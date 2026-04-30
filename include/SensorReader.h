#pragma once

struct SensorReading
{
    bool hasSoilMoisture = false;
    int soilMoisturePercent = 0;
    int soilMoistureRaw = 0;

    bool hasTemperature = false;
    float temperatureC = 0.0;

    bool hasHumidity = false;
    float humidityPercent = 0.0;

    int waterLevelPercent = 0;
};

void beginSensorReader();
SensorReading readSensors();