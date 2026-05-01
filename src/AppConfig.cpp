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

    Serial.print("Schedule count: ");
    Serial.println(deviceConfig.scheduleCount);

    for (int i = 0; i < deviceConfig.scheduleCount; i++)
    {
        Serial.print("Schedule ");
        Serial.print(i + 1);
        Serial.print(" | ID: ");
        Serial.print(deviceConfig.schedules[i].id);
        Serial.print(" | Enabled: ");
        Serial.print(deviceConfig.schedules[i].isEnabled ? "yes" : "no");
        Serial.print(" | Day: ");
        Serial.print(deviceConfig.schedules[i].dayOfWeek);
        Serial.print(" | Time: ");
        Serial.print(deviceConfig.schedules[i].timeOfDay);
        Serial.print(" | Duration seconds: ");
        Serial.println(deviceConfig.schedules[i].durationSeconds);
    }
}

void resetSchedules()
{
    deviceConfig.scheduleCount = 0;

    for (int i = 0; i < MAX_WATERING_SCHEDULES; i++)
    {
        deviceConfig.schedules[i] = WateringScheduleConfig();
    }
}

bool applyConfigObject(JsonObject config)
{
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

    resetSchedules();

    JsonArray schedules = config["schedules"].as<JsonArray>();

    if (!schedules.isNull())
    {
        for (JsonObject schedule : schedules)
        {
            if (deviceConfig.scheduleCount >= MAX_WATERING_SCHEDULES)
            {
                Serial.println("Warning: maximum cached schedules reached. Extra schedules ignored.");
                break;
            }

            int index = deviceConfig.scheduleCount;

            deviceConfig.schedules[index].id = schedule["id"] | 0;
            deviceConfig.schedules[index].isEnabled = schedule["is_enabled"] | false;
            deviceConfig.schedules[index].dayOfWeek = schedule["day_of_week"] | 0;
            deviceConfig.schedules[index].timeOfDay = schedule["time_of_day"] | "";
            deviceConfig.schedules[index].durationSeconds = schedule["duration_seconds"] | 0;

            deviceConfig.scheduleCount++;
        }
    }

    deviceConfig.hasLoadedConfig = true;

    printDeviceConfig();

    return true;
}

bool parseConfigResponse(const String &response)
{
    JsonDocument doc;

    DeserializationError error = deserializeJson(doc, response);

    if (error)
    {
        Serial.print("Failed to parse config response JSON: ");
        Serial.println(error.c_str());
        return false;
    }

    JsonObject config = doc["config"];

    return applyConfigObject(config);
}

bool parseConfigObjectJson(const String &configJson)
{
    if (configJson.length() == 0)
    {
        Serial.println("No cached config JSON to parse.");
        return false;
    }

    JsonDocument doc;

    DeserializationError error = deserializeJson(doc, configJson);

    if (error)
    {
        Serial.print("Failed to parse cached config JSON: ");
        Serial.println(error.c_str());
        return false;
    }

    return applyConfigObject(doc.as<JsonObject>());
}

String extractConfigJsonFromResponse(const String &response)
{
    JsonDocument doc;

    DeserializationError error = deserializeJson(doc, response);

    if (error)
    {
        Serial.print("Failed to parse config response for cache extraction: ");
        Serial.println(error.c_str());
        return "";
    }

    JsonObject config = doc["config"];

    if (config.isNull())
    {
        Serial.println("Cannot extract cache config: config object missing.");
        return "";
    }

    String configJson;
    serializeJson(config, configJson);

    return configJson;
}
