#!/usr/bin/env python3
import json
from datetime import datetime, timezone, timedelta
WIB = timezone(timedelta(hours=7))
from influxdb import InfluxDBClient
import paho.mqtt.client as mqtt

# ==========================
# KONFIGURASI
# ==========================

# MQTT broker
MQTT_HOST = "206.237.97.19"
MQTT_PORT = 1883
MQTT_USER = "urbansolv"
MQTT_PASS = "letsgosolv"
MQTT_TOPIC = "power_monitoring/+/data/telemetry"

# InfluxDB
INFLUX_HOST = "127.0.0.1"
INFLUX_PORT = 8086
INFLUX_DB   = "power_logger"

# Kalau True: pakai timestamp dari JSON
# Kalau False: pakai waktu server Raspberry (lebih “realtime” ke waktu terima)
USE_DEVICE_TIMESTAMP = False

# ==========================
# SETUP INFLUX
# ==========================

influx = InfluxDBClient(host=INFLUX_HOST, port=INFLUX_PORT)
influx.switch_database(INFLUX_DB)

# ==========================
# CALLBACK MQTT
# ==========================

def on_connect(client, userdata, flags, rc):
    print("MQTT connected with result code", rc)
    client.subscribe(MQTT_TOPIC)
    print("Subscribed to", MQTT_TOPIC)

def on_message(client, userdata, msg):
    try:
        topic = msg.topic
        data = json.loads(msg.payload.decode("utf-8"))

        parts = topic.split("/")
        measurement = parts[0]
        device = parts[1]

        # ----- PILIH TIMESTAMP -----
        if USE_DEVICE_TIMESTAMP and "timestamp" in data:
            # device timestamp → dikonversi ke WIB
            ts = datetime.fromtimestamp(int(data["timestamp"]), WIB).isoformat()
        else:
            # waktu lokal Raspberry Pi (WIB)
            ts = datetime.now(WIB).isoformat()

        # ambil field yang ada (aman kalau ada yang hilang)
        fields = {}
        for key in ["voltage", "current", "power", "power_factor", "energy"]:
            if key in data:
                try:
                    fields[key] = float(data[key])
                except ValueError:
                    pass

        if not fields:
            print("No numeric fields in payload, skip:", payload_str)
            return

        point = [{
            "measurement": measurement,
            "tags": {
                "device": device
            },
            "time": ts,
            "fields": fields
        }]

        influx.write_points(point)
        print("Wrote point:", point)

    except Exception as e:
        print("Error processing message:", e)
        print("Raw payload:", msg.payload)

# ==========================
# MAIN
# ==========================

client = mqtt.Client()
client.username_pw_set(MQTT_USER, MQTT_PASS)
client.on_connect = on_connect
client.on_message = on_message

client.connect(MQTT_HOST, MQTT_PORT, 60)

print("Starting MQTT loop...")
client.loop_forever()

