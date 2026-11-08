#include "sensor_dht.h"

SensorDHT::SensorDHT(uint8_t pino, uint8_t tipo)
  : _dht(pino, tipo) {}

void SensorDHT::iniciar() {
  _dht.begin();
  delay(2000);
  Serial.println("Sensor DHT inicializado");
}

bool SensorDHT::ler(float& temperatura, float& umidade) {
  delay(2000);

  temperatura = _dht.readTemperature();
  umidade     = _dht.readHumidity();

  if (isnan(temperatura) || isnan(umidade)) {
    Serial.println("Leitura do DHT falhou");
    return false;
  }
  return true;
}