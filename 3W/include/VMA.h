#ifndef VMA_H
#define VMA_H

#include <Arduino.h>
#include <BME280I2C.h>
#include <Wire.h>

void printBME280Data(Stream* client, BME280I2C& bme);

#endif