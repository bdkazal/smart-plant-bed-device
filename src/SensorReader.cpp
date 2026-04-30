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

bool isSoilMoistureSensorAvailable(int rawValue)
{
    // With the 100k pulldown resistor:
    //   disconnected sensor raw = around 0
    //   wet soil raw            = around 995–1021
    //   dry soil raw            = around 1382–1535
    //   air raw                 = around 2100–2222
    //
    // So low raw values below SOIL_DISCONNECTED_RAW_MAX mean
    // the sensor signal is unavailable, not "100% wet".
    return rawValue >= SOIL_DISCONNECTED_RAW_MAX;
}

int convertSoilRawToPercent(int rawValue)
{
    // Capacitive sensor behavior:
    //   higher raw = drier
    //   lower raw  = wetter
    //
    // Calibration after adding 100k pulldown:
    //   SOIL_DRY_RAW -> 0%
    //   SOIL_WET_RAW -> 100%
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

    reading.soilMoistureRaw = soilRaw;
    reading.hasSoilMoisture = isSoilMoistureSensorAvailable(soilRaw);

    if (reading.hasSoilMoisture)
    {
        reading.soilMoisturePercent = convertSoilRawToPercent(soilRaw);
    }

    float temperature = readTemperatureC();
    float humidity = readHumidityPercent();

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