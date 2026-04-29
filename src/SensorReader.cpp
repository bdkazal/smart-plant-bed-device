#include "SensorReader.h"

#include <Arduino.h>

#include "PinConfig.h"

int clampPercent(int value)
{
    if (value < 0)
    {
        return 0;
    }

    if (value > 100)
    {
        return 100;
    }

    return value;
}

int readSoilMoistureRaw()
{
    // Take multiple samples to reduce noisy ADC readings.
    const int sampleCount = 10;
    long total = 0;

    for (int i = 0; i < sampleCount; i++)
    {
        total += analogRead(SOIL_MOISTURE_PIN);
        delay(5);
    }

    return total / sampleCount;
}

int convertSoilRawToPercent(int rawValue)
{
    // Capacitive sensors usually read:
    //   higher raw = drier
    //   lower raw  = wetter
    //
    // So we map:
    //   SOIL_DRY_RAW -> 0%
    //   SOIL_WET_RAW -> 100%
    int percent = map(rawValue, SOIL_DRY_RAW, SOIL_WET_RAW, 0, 100);

    return clampPercent(percent);
}

SensorReading readSensors()
{
    SensorReading reading;

    int soilRaw = readSoilMoistureRaw();

    reading.soilMoisturePercent = convertSoilRawToPercent(soilRaw);

    // Temperature/humidity are still fake for now.
    // Later we will replace them with DHT/SHT sensor readings.
    reading.temperatureC = random(240, 321) / 10.0;
    reading.humidityPercent = random(450, 801) / 10.0;

    Serial.println();
    Serial.println("Sensor reading:");
    Serial.print("Soil moisture raw: ");
    Serial.println(soilRaw);
    Serial.print("Soil moisture %: ");
    Serial.println(reading.soilMoisturePercent);
    Serial.print("Temperature C: ");
    Serial.println(reading.temperatureC);
    Serial.print("Humidity %: ");
    Serial.println(reading.humidityPercent);

    return reading;
}