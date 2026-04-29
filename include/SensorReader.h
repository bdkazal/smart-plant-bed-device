#pragma once

struct SensorReading
{
    int soilMoisturePercent;
    int waterLevelPercent;
    float temperatureC;
    float humidityPercent;
};

void beginSensorReader();
SensorReading readSensors();