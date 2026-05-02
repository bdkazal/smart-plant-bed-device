#include "LocalAutomation.h"

#include <Arduino.h>

#include "ApiClient.h"
#include "AppConfig.h"
#include "ValveController.h"

unsigned long lastLocalAutoWateringAt = 0;

bool isLocalAutoModeEnabled()
{
    return deviceConfig.wateringMode == "auto";
}

bool hasUsableLocalAutoConfig()
{
    return deviceConfig.hasLoadedConfig &&
           deviceConfig.soilMoistureThreshold > 0 &&
           deviceConfig.maxWateringDurationSeconds > 0;
}

bool localAutoCooldownPassed()
{
    if (lastLocalAutoWateringAt == 0)
    {
        return true;
    }

    unsigned long cooldownMs = (unsigned long)deviceConfig.cooldownMinutes * 60UL * 1000UL;

    return millis() - lastLocalAutoWateringAt >= cooldownMs;
}

void beginLocalAutomation()
{
    lastLocalAutoWateringAt = 0;

    Serial.println();
    Serial.println("Local automation initialized.");
    Serial.println("Local auto watering is fallback-only when Laravel is not reachable.");
}

void updateLocalAutomation(const SensorReading &reading)
{
    if (isServerRecentlyReachable())
    {
        return;
    }

    if (!hasUsableLocalAutoConfig())
    {
        Serial.println("Local auto skipped: no usable cached config.");
        return;
    }

    if (!isLocalAutoModeEnabled())
    {
        return;
    }

    if (!reading.hasSoilMoisture)
    {
        Serial.println("Local auto skipped: soil moisture reading unavailable.");
        return;
    }

    if (isWateringActive())
    {
        return;
    }

    if (!localAutoCooldownPassed())
    {
        return;
    }

    if (reading.soilMoisturePercent > deviceConfig.soilMoistureThreshold)
    {
        return;
    }

    Serial.println();
    Serial.println("Local fallback auto watering triggered.");
    Serial.print("Soil moisture %: ");
    Serial.println(reading.soilMoisturePercent);
    Serial.print("Threshold %: ");
    Serial.println(deviceConfig.soilMoistureThreshold);
    Serial.print("Duration seconds: ");
    Serial.println(deviceConfig.maxWateringDurationSeconds);

    startLocalWatering(deviceConfig.maxWateringDurationSeconds);

    lastLocalAutoWateringAt = millis();
}
