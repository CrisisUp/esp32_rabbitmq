#include <Arduino.h>
#include <stdint.h>
#include <WiFi.h>
#include <PubSubClient.h>

// --- Configurações de Rede ---
const char* ssid = "TIM GOMES-2G";
const char* password = "Tyll89na";

// --- Configurações do RabbitMQ (Broker MQTT) ---
const char* mqtt_server = "192.168.1.9";
const int   mqtt_port   = 1883;
const char* mqtt_user   = "admin";
const char* mqtt_password = "admin123";

// --- Tópicos ---
const char* topico_comando = "casa/led/comando";
const char* topico_status  = "casa/led/status";

// --- Hardware ---
const int pinoLed = 2;

WiFiClient espClient;
PubSubClient client(espClient);

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Conectando a ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi conectado");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

// Função chamada quando uma mensagem chega no tópico inscrito
void callback(char* topic, uint8_t* payload, unsigned int length) {
  Serial.print("Mensagem recebida no topico: ");
  Serial.println(topic);

  String mensagem = "";
  for (int i = 0; i < length; i++) {
    mensagem += (char)payload[i];
  }
  Serial.print("Conteudo: ");
  Serial.println(mensagem);

  if (mensagem == "ligar") {
    digitalWrite(pinoLed, HIGH);
    client.publish(topico_status, "LED_ON");
    Serial.println("LED LIGADO (status publicado)");
  } else if (mensagem == "desligar") {
    digitalWrite(pinoLed, LOW);
    client.publish(topico_status, "LED_OFF");
    Serial.println("LED DESLIGADO (status publicado)");
  } else {
    client.publish(topico_status, "comando_invalido");
    Serial.println("Comando invalido");
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Tentando conexao MQTT...");
    String clientId = "esp32-" + WiFi.macAddress();
    clientId.replace(":", "");

    if (client.connect(clientId.c_str(), mqtt_user, mqtt_password)) {
      Serial.println("conectado");
      client.subscribe(topico_comando);
      client.publish(topico_status, "online");   // avisa que entrou
    } else {
      Serial.print("falhou, rc=");
      Serial.print(client.state());
      Serial.println(" tentando novamente em 5 segundos");
      delay(5000);
    }
  }
}

void setup() {
  pinMode(pinoLed, OUTPUT);
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Heartbeat a cada 10s
  static unsigned long ultimoEnvio = 0;
  if (millis() - ultimoEnvio > 10000) {
    ultimoEnvio = millis();
    client.publish(topico_status, "alive");
    Serial.println("Heartbeat enviado: alive");
  }
}