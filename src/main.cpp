#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>

#include "secrets.h"
#include "topicos.h"

// ============================================================
// CONFIGURAÇÃO DO SENSOR
// ============================================================
// Comente a linha abaixo quando o DHT22 fisico estiver conectado.
// Enquanto estiver comentada, o firmware le o sensor real.
// #define SIMULAR_SENSOR

#define DHTPIN  5
#define DHTTYPE 22
DHT dht(DHTPIN, DHTTYPE);

// ============================================================
// CREDENCIAIS (do secrets.h)
// ============================================================
const char* ssid          = WIFI_SSID;
const char* password      = WIFI_PASSWORD;
const char* mqtt_user     = MQTT_USER;
const char* mqtt_password = MQTT_PASSWORD;
const char* mqtt_server   = MQTT_SERVER;

const int mqtt_port = 1883;

// ============================================================
// HARDWARE
// ============================================================
const int pinoLed = 2;

WiFiClient espClient;
PubSubClient client(espClient);

// ============================================================
// Wi-Fi
// ============================================================
void setup_wifi() {
  WiFi.begin(ssid, password);
  Serial.print("Conectando Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi conectado");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

// ============================================================
// Callback MQTT (comandos do LED)
// ============================================================
void callback(char* topic, uint8_t* payload, unsigned int length) {
  String mensagem = "";
  for (unsigned int i = 0; i < length; i++) mensagem += (char)payload[i];

  Serial.printf("Comando recebido: %s\n", mensagem.c_str());

  if (mensagem == "ligar") {
    digitalWrite(pinoLed, HIGH);
    client.publish(TOPICO_LED_STATUS, "LED_ON");
  } else if (mensagem == "desligar") {
    digitalWrite(pinoLed, LOW);
    client.publish(TOPICO_LED_STATUS, "LED_OFF");
  } else {
    client.publish(TOPICO_LED_STATUS, "comando_invalido");
  }
}

// ============================================================
// Reconexão MQTT
// ============================================================
void reconnect() {
  while (!client.connected()) {
    String clientId = "esp32-" + WiFi.macAddress();
    clientId.replace(":", "");

    Serial.print("Tentando conexao MQTT...");
    if (client.connect(clientId.c_str(), mqtt_user, mqtt_password)) {
      Serial.println("conectado");
      client.subscribe(TOPICO_LED_COMANDO);
      client.publish(TOPICO_LED_STATUS, "online");
    } else {
      Serial.printf("falhou, rc=%d. Tentando em 5s\n", client.state());
      delay(5000);
    }
  }
}

// ============================================================
// Leitura + publicação do sensor
// ============================================================
void publicarSensor() {
  float temperatura, umidade;

#ifdef SIMULAR_SENSOR
  // ---- MODO SIMULADO (sem hardware) ----
  temperatura = 24.0 + (random(0, 100) / 100.0);  // 24.00 .. 24.99
  umidade     = 60.0 + (random(0, 100) / 100.0);  // 60.00 .. 60.99
  Serial.println("[SIMULADO] usando valores fake do sensor");
#else
  // ---- MODO REAL (com DHT22) ----
  // DHT22 exige ~2s entre leituras; garante estabilidade
  delay(2000);

  temperatura = dht.readTemperature();
  umidade     = dht.readHumidity();

  // Retry: até 3 tentativas com 2s entre elas
  int tentativas = 0;
  while ((isnan(temperatura) || isnan(umidade)) && tentativas < 3) {
    Serial.printf("  Leitura falhou (tentativa %d/3). Retentando em 2s...\n", tentativas + 1);
    delay(2000);
    temperatura = dht.readTemperature();
    umidade     = dht.readHumidity();
    tentativas++;
  }

  if (isnan(temperatura) || isnan(umidade)) {
    Serial.println("=== FALHA PERSISTENTE NA LEITURA DO DHT22 ===");
    Serial.println("Verifique fisicamente:");
    Serial.println("  1. VCC do sensor no 3.3V (NAO 5V)");
    Serial.println("  2. DATA no GPIO 4 (pino D4 do ESP32)");
    Serial.println("  3. GND no GND");
    Serial.println("  4. Resistor 10k entre VCC e DATA (se DHT22 'cru')");
    Serial.println("  5. Sensor é realmente DHT22 (nao DHT11)?");
    Serial.println("  6. Conexao firme na protoboard?");
    return;
  }
#endif

  // ---- Publica JSON ----
  char payload[100];
  snprintf(payload, sizeof(payload), "{\"temp\":%.1f,\"hum\":%.1f}", temperatura, umidade);
  client.publish(TOPICO_SENSOR_DHT, payload);

  Serial.printf("Sensor publicado: %s\n", payload);
}

// ============================================================
// Setup
// ============================================================
void setup() {
  Serial.begin(115200);
  delay(1000);  // dá tempo do Serial abrir
  Serial.println("\n=== ESP32 IoT + RabbitMQ ===");

  pinMode(pinoLed, OUTPUT);
  digitalWrite(pinoLed, LOW);

#ifndef SIMULAR_SENSOR
  dht.begin();
  delay(2000);  // DHT22 precisa estabilizar após power-on
  Serial.println("DHT22 inicializado");
#else
  Serial.println("Modo SIMULADO — sensor nao sera lido");
  randomSeed(analogRead(0));
#endif

  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

// ============================================================
// Loop
// ============================================================
void loop() {
  if (!client.connected()) reconnect();
  client.loop();

  // Heartbeat a cada 10s
  static unsigned long ultimoHeartbeat = 0;
  if (millis() - ultimoHeartbeat > 10000) {
    ultimoHeartbeat = millis();
    client.publish(TOPICO_LED_STATUS, "alive");
  }

  // Sensor a cada 30s
  static unsigned long ultimoSensor = 0;
  if (millis() - ultimoSensor > 30000) {
    ultimoSensor = millis();
    publicarSensor();
  }
}