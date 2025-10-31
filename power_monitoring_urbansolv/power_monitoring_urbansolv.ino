#include <WiFi.h>
#include <PubSubClient.h>
#include <PZEM004Tv30.h>

// ====== KONFIGURASI WIFI DAN MQTT ======
const char* ssid = "WIFI-RUCKUS-STP";
const char* password = "STP2024!";
const char* mqtt_server = "broker.hivemq.com";  // broker publik
const int mqtt_port = 1883;
const char* mqtt_client_id = "Power_Monitoring_Urbansolv";

// ====== TOPIK MQTT ======
const char* topic_voltage = "power_monitoring/data/voltage";
const char* topic_current = "power_monitoring/data/current";
const char* topic_power   = "power_monitoring/data/power";
const char* topic_energy  = "power_monitoring/data/energy";

// ====== OBJEK ======
WiFiClient espClient;
PubSubClient client(espClient);
PZEM004Tv30 pzem(&Serial2, 25, 26); // RX=25, TX=26

// ====== FUNGSI ======
void setup_wifi() {
  Serial.println();
  Serial.print("Menghubungkan ke ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi tersambung!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Menghubungkan ke MQTT...");
    if (client.connect(mqtt_client_id)) {
      Serial.println("Terhubung ke broker MQTT!");
    } else {
      Serial.print("Gagal, rc=");
      Serial.print(client.state());
      Serial.println(" mencoba lagi 5 detik...");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(9600);
  Serial2.begin(9600, SERIAL_8N1, 25, 26); // UART2 dengan pin 25/26 RX 26 TX25

  Serial.println("Inisialisasi Sensor...");
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  Serial.println("Sistem Monitoring Daya");
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // === Baca data dari sensor ===
  float voltage = pzem.voltage();
  float current = pzem.current();
  float power   = pzem.power();
  float energy  = pzem.energy();

  if (isnan(voltage)) {
    Serial.println("Gagal membaca data dari sensor PZEM!");
  } else {
    // Cetak ke Serial Monitor
    Serial.println("===== DATA DAYA =====");
    Serial.print("Tegangan : "); Serial.print(voltage); Serial.println(" V");
    Serial.print("Arus     : "); Serial.print(current); Serial.println(" A");
    Serial.print("Daya     : "); Serial.print(power); Serial.println(" W");
    Serial.print("Energi   : "); Serial.print(energy); Serial.println(" Wh");
    Serial.println("======================");

    // Konversi ke string
    char buffer[16];
    dtostrf(voltage, 6, 2, buffer);
    client.publish(topic_voltage, buffer);

    dtostrf(current, 6, 2, buffer);
    client.publish(topic_current, buffer);

    dtostrf(power, 6, 2, buffer);
    client.publish(topic_power, buffer);

    dtostrf(energy, 6, 2, buffer);
    client.publish(topic_energy, buffer);
  }

  delay(1000); // kirim setiap 1 detik
}
