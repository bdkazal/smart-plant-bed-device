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

int convertSoilRawToPercent(int rawValue)
{
    int percent = map(rawValue, SOIL_DRY_RAW, SOIL_WET_RAW, 0, 100);

    return clampPercent(percent);
}

float readTemperatureC()
{
    float temperature = dht.readTemperature();

    if (isnan(temperature))
    {
        Serial.println("Warning: failed to read DHT11 temperature.");
        return NAN;
    }

    return temperature;
}

float readHumidityPercent()
{
    float humidity = dht.readHumidity();

    if (isnan(humidity))
    {
        Serial.println("Warning: failed to read DHT11 humidity.");
        return NAN;
    }

    return humidity;
}

SensorReading readSensors()
{
    SensorReading reading;

    int soilRaw = readSoilMoistureRaw();

    reading.soilMoisturePercent = convertSoilRawToPercent(soilRaw);

    float temperature = readTemperatureC();
    float humidity = readHumidityPercent();

    // If DHT11 fails, keep values as 0 for now.
    // Later we can improve ApiClient to send null values.
    reading.temperatureC = isnan(temperature) ? 0.0 : temperature;
    reading.humidityPercent = isnan(humidity) ? 0.0 : humidity;

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