#include "SensorReader.h"

#include <Arduino.h>

SensorReading readSensors()
{
    SensorReading reading;

    // Fake values for API testing.
    // Later we will replace these with real sensor reads.
    reading.soilMoisturePercent = random(30, 71);
    reading.temperatureC = random(240, 321) / 10.0;
    reading.humidityPercent = random(450, 801) / 10.0;

    Serial.println();
    Serial.println("Fake sensor reading:");
    Serial.print("Soil moisture %: ");
    Serial.println(reading.soilMoisturePercent);
    Serial.print("Temperature C: ");
    Serial.println(reading.temperatureC);
    Serial.print("Humidity %: ");
    Serial.println(reading.humidityPercent);

    return reading;
}