#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>

// --- Headers locais do projeto ---
#include "secrets.h"      // Credenciais (Wi-Fi, MQTT)
#include "topicos.h"      // Nomes dos tópicos MQTT
#include "sensor_dht.h"   // Lógica do sensor DHT22

// ============================================================
// CONFIGURAÇÃO DE HARDWARE
// ============================================================
const int PINO_LED = 2;

// Pino e tipo do sensor (usado pelo construtor do SensorDHT)
#define DHTPIN  5
#define DHTTYPE DHT22

// ============================================================
// CREDENCIAIS E REDE (Vindas do secrets.h)
// ============================================================
const char* ssid          = WIFI_SSID;
const char* password      = WIFI_PASSWORD;
const char* mqtt_user     = MQTT_USER;
const char* mqtt_password = MQTT_PASSWORD;
const char* mqtt_server   = MQTT_SERVER;
const int   mqtt_port     = 1883;

// ============================================================
// INSTÂNCIAS GLOBAIS
// ============================================================
WiFiClient   espClient;
PubSubClient client(espClient);
SensorDHT    sensor(DHTPIN, DHTTYPE);

// ============================================================
// CONEXÃO WI-FI
// ============================================================
void setup_wifi() {
  Serial.print("Conectando Wi-Fi");
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("\nWiFi conectado");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

// ============================================================
// CALLBACK MQTT (Recebe comandos do LED)
// ============================================================
void callback(char* topic, uint8_t* payload, unsigned int length) {
  String mensagem = "";
  for (unsigned int i = 0; i < length; i++) {
    mensagem += (char)payload[i];
  }

  Serial.printf("Comando recebido [%s]: %s\n", topic, mensagem.c_str());

  if (mensagem == "ligar") {
    digitalWrite(PINO_LED, HIGH);
    client.publish(TOPICO_LED_STATUS, "LED_ON");
  } 
  else if (mensagem == "desligar") {
    digitalWrite(PINO_LED, LOW);
    client.publish(TOPICO_LED_STATUS, "LED_OFF");
  } 
  else {
    client.publish(TOPICO_LED_STATUS, "comando_invalido");
  }
}

// ============================================================
// RECONEXÃO MQTT (Com backoff em caso de falha)
// ============================================================
void reconnect() {
  while (!client.connected()) {
    // Gera um ID único baseado no MAC address
    String clientId = "esp32-" + WiFi.macAddress();
    clientId.replace(":", "");

    Serial.print("Tentando conexao MQTT...");
    
    if (client.connect(clientId.c_str(), mqtt_user, mqtt_password)) {
      Serial.println("conectado");
      client.subscribe(TOPICO_LED_COMANDO);
      client.publish(TOPICO_LED_STATUS, "online");
    } 
    else {
      Serial.printf("falhou, rc=%d. Tentando em 5s\n", client.state());
      delay(5000);
    }
  }
}

// ============================================================
// PUBLICAÇÃO DO SENSOR
// ============================================================
void publicarSensor() {
  float temperatura, umidade;

  // O método ler() do SensorDHT já faz o delay de 2s e verifica erros
  if (!sensor.ler(temperatura, umidade)) {
    Serial.println("Sem leitura válida do sensor. Pulando publicação.");
    return;
  }

  // Monta o JSON
  char payload[100];
  snprintf(payload, sizeof(payload), "{\"temp\":%.1f,\"hum\":%.1f}", temperatura, umidade);

  // Publica
  if (client.publish(TOPICO_SENSOR_DHT, payload)) {
    Serial.printf("Sensor publicado: %s\n", payload);
  } else {
    Serial.println("Falha ao publicar no MQTT (buffer cheio?)");
  }
}

// ============================================================
// SETUP
// ============================================================
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=== ESP32 IoT + RabbitMQ ===");

  // 1. Configura Hardware
  pinMode(PINO_LED, OUTPUT);
  digitalWrite(PINO_LED, LOW);
  
  // 2. Inicializa Sensor (o método iniciar() já faz o delay necessário)
  sensor.iniciar();

  // 3. Inicializa Redes
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

// ============================================================
// LOOP PRINCIPAL
// ============================================================
void loop() {
  // 1. Garante que o MQTT está vivo
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // 2. Heartbeat (a cada 10 segundos)
  static unsigned long ultimoHeartbeat = 0;
  if (millis() - ultimoHeartbeat > 10000) {
    ultimoHeartbeat = millis();
    client.publish(TOPICO_LED_STATUS, "alive");
  }

  // 3. Telemetria (a cada 30 segundos)
  static unsigned long ultimoSensor = 0;
  if (millis() - ultimoSensor > 30000) {
    ultimoSensor = millis();
    publicarSensor();
  }
}