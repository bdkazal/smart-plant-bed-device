#include "AppConfig.h"
#include <ArduinoJson.h>

DeviceConfig deviceConfig;

void printDeviceConfig()
{
    Serial.println();
    Serial.println("Parsed device config:");
    Serial.print("Device name: ");
    Serial.println(deviceConfig.deviceName);
    Serial.print("Timezone: ");
    Serial.println(deviceConfig.timezone);
    Serial.print("Watering mode: ");
    Serial.println(deviceConfig.wateringMode);
    Serial.print("Soil moisture threshold: ");
    Serial.println(deviceConfig.soilMoistureThreshold);
    Serial.print("Max watering duration seconds: ");
    Serial.println(deviceConfig.maxWateringDurationSeconds);
    Serial.print("Cooldown minutes: ");
    Serial.println(deviceConfig.cooldownMinutes);
    Serial.print("Local manual duration seconds: ");
    Serial.println(deviceConfig.localManualDurationSeconds);
}

bool parseConfigResponse(const String &response)
{
    JsonDocument doc;

    DeserializationError error = deserializeJson(doc, response);

    if (error)
    {
        Serial.print("Failed to parse config JSON: ");
        Serial.println(error.c_str());
        return false;
    }

    JsonObject config = doc["config"];

    if (config.isNull())
    {
        Serial.println("Config JSON does not contain a config object.");
        return false;
    }

    deviceConfig.deviceName = config["device_name"] | "";
    deviceConfig.timezone = config["timezone"] | "";
    deviceConfig.wateringMode = config["watering_mode"] | "";
    deviceConfig.soilMoistureThreshold = config["soil_moisture_threshold"] | 0;
    deviceConfig.maxWateringDurationSeconds = config["max_watering_duration_seconds"] | 0;
    deviceConfig.cooldownMinutes = config["cooldown_minutes"] | 0;
    deviceConfig.localManualDurationSeconds = config["local_manual_duration_seconds"] | 0;

    printDeviceConfig();

    return true;
}