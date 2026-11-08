#pragma once
// clangd: ignore unused-includes
// ============================================================
// Tópicos MQTT do projeto
// ============================================================
// Padrão: casa/<categoria>/<dispositivo>/<ação>
// O plugin MQTT do RabbitMQ traduz '/' -> '.' ao cruzar pra AMQP,
// então "casa/led/comando" vira a routing key "casa.led.comando".
// ============================================================

// --- LED ---
constexpr const char* TOPICO_LED_COMANDO = "casa/led/comando";  // recebe "ligar"/"desligar"
constexpr const char* TOPICO_LED_STATUS  = "casa/led/status";   // publica LED_ON/LED_OFF/alive

// --- Sensor DHT22 ---
constexpr const char* TOPICO_SENSOR_DHT  = "casa/sensor/dht";   // publica {"temp":..,"hum":..}

// --- Adicione novos tópicos abaixo ---
// constexpr const char* TOPICO_SENSOR_LUZ = "casa/sensor/luz";