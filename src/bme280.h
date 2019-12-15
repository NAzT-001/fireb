#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

#define SEALEVELPRESSURE_HPA (1013.25)

Adafruit_BME280 bme;
double bmeTemp, bmeHumid, press, alt;

void readBME280()
{
  bmeTemp = bme.readTemperature();
  bmeHumid = bme.readHumidity();
  press = bme.readPressure() / 100.0F;
  alt = bme.readAltitude(SEALEVELPRESSURE_HPA);

  if (bmeTemp <= 0 && bmeTemp >= 100)
  {
    bmeTemp = 0;
  }

  if (bmeHumid <= 0 && bmeHumid >= 100)
  {
    bmeHumid = 0;
  }
}
