#include "SensorReader.h"

#include <Arduino.h>
#include <DHT.h>

#include "PinConfig.h"

static const int DHT_TYPE = DHT11;

DHT dht(DHT_SENSOR_PIN, DHT_TYPE);

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

void beginSensorReader()
{
    dht.begin();

    Serial.println();
    Serial.println("Sensor reader initialized.");
    Serial.print("Soil moisture ADC GPIO: ");
    Serial.println(SOIL_MOISTURE_PIN);
    Serial.print("DHT11 data GPIO: ");
    Serial.println(DHT_SENSOR_PIN);
}

int readSoilMoistureRaw()
{
    const int sampleCount = 10;
    long total = 0;

    for (int i = 0; i < sampleCount; i++)
    {
        total += analogRead(SOIL_MOISTURE_PIN);
        delay(5);
    }

    return total / sampleCount;
}

bool isSoilSensorAvailable(int rawValue)
{
    // If the raw value is close to your air/disconnected range,
    // do not treat it as valid soil moisture.
    return rawValue < SOIL_SENSOR_UNAVAILABLE_RAW;
}

int convertSoilRawToPercent(int rawValue)
{
    int percent = map(rawValue, SOIL_DRY_RAW, SOIL_WET_RAW, 0, 100);

    return clampPercent(percent);
}

SensorReading readSensors()
{
    SensorReading reading;

    int soilRaw = readSoilMoistureRaw();

    reading.soilMoistureRaw = soilRaw;
    reading.hasSoilMoisture = isSoilSensorAvailable(soilRaw);

    if (reading.hasSoilMoisture)
    {
        reading.soilMoisturePercent = convertSoilRawToPercent(soilRaw);
    }

    float temperature = dht.readTemperature();
    float humidity = dht.readHumidity();

    reading.hasTemperature = !isnan(temperature);
    reading.hasHumidity = !isnan(humidity);

    if (reading.hasTemperature)
    {
        reading.temperatureC = temperature;
    }

    if (reading.hasHumidity)
    {
        reading.humidityPercent = humidity;
    }

    Serial.println();
    Serial.println("Sensor reading:");
    Serial.print("Soil moisture raw: ");
    Serial.println(soilRaw);

    if (reading.hasSoilMoisture)
    {
        Serial.print("Soil moisture %: ");
        Serial.println(reading.soilMoisturePercent);
    }
    else
    {
        Serial.println("Soil moisture: unavailable");
    }

    if (reading.hasTemperature)
    {
        Serial.print("Temperature C: ");
        Serial.println(reading.temperatureC);
    }
    else
    {
        Serial.println("Temperature C: unavailable");
    }

    if (reading.hasHumidity)
    {
        Serial.print("Humidity %: ");
        Serial.println(reading.humidityPercent);
    }
    else
    {
        Serial.println("Humidity %: unavailable");
    }

    return reading;
}