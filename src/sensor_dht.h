#pragma once
#include <Arduino.h>
#include <DHT.h>

class SensorDHT {
public:
  SensorDHT(uint8_t pino, uint8_t tipo);

  void iniciar();
  bool ler(float& temperatura, float& umidade);

private:
  DHT _dht;
};