#include "ApiClient.h"

#include <HTTPClient.h>

#include "secrets.h"
#include "WiFiMan.h"
#include "AppConfig.h"
#include "CommandHandler.h"
#include "ValveController.h"

void addDeviceHeaders(HTTPClient &http)
{
  http.addHeader("Content-Type", "application/json");
  http.addHeader("X-DEVICE-KEY", DEVICE_API_KEY);
}

void fetchConfig()
{
  if (!isWiFiConnected())
  {
    Serial.println("Cannot fetch config: Wi-Fi is not connected.");
    return;
  }

  String url = String(API_BASE_URL) + "/api/device/config?device_uuid=" + DEVICE_UUID;

  HTTPClient http;

  Serial.println();
  Serial.println("Fetching device config...");
  Serial.print("URL: ");
  Serial.println(url);

  http.begin(url);
  http.addHeader("X-DEVICE-KEY", DEVICE_API_KEY);

  int statusCode = http.GET();
  String response = http.getString();

  Serial.print("HTTP status: ");
  Serial.println(statusCode);

  Serial.print("Response: ");
  Serial.println(response);

  if (statusCode == 200)
  {
    bool parsed = parseConfigResponse(response);

    if (!parsed)
    {
      Serial.println("Config response received but parsing failed.");
    }
  }
  else
  {
    Serial.println("Config fetch failed. Keeping previous/default config.");
  }

  http.end();
}

void sendHeartbeat()
{
  if (!isWiFiConnected())
  {
    Serial.println("Cannot send heartbeat: Wi-Fi is not connected.");
    return;
  }

  String url = String(API_BASE_URL) + "/api/device/heartbeat";

  String body = "{";
  body += "\"device_uuid\":\"";
  body += DEVICE_UUID;
  body += "\"";
  body += "}";

  HTTPClient http;

  Serial.println();
  Serial.println("Sending heartbeat...");
  Serial.print("URL: ");
  Serial.println(url);
  Serial.print("Body: ");
  Serial.println(body);

  http.begin(url);
  addDeviceHeaders(http);

  int statusCode = http.POST(body);
  String response = http.getString();

  Serial.print("HTTP status: ");
  Serial.println(statusCode);

  Serial.print("Response: ");
  Serial.println(response);

  http.end();
}

bool sendCommandAck(int commandId, const String &status, const String &message)
{
  if (!isWiFiConnected())
  {
    Serial.println("Cannot send command ack: Wi-Fi is not connected.");
    return false;
  }

  String url = String(API_BASE_URL) + "/api/device/commands/" + String(commandId) + "/ack";

  String body = "{";
  body += "\"device_uuid\":\"";
  body += DEVICE_UUID;
  body += "\",";
  body += "\"status\":\"";
  body += status;
  body += "\"";

  if (message.length() > 0)
  {
    body += ",\"message\":\"";
    body += message;
    body += "\"";
  }

  body += "}";

  HTTPClient http;

  Serial.println();
  Serial.println("Sending command ack...");
  Serial.print("URL: ");
  Serial.println(url);
  Serial.print("Body: ");
  Serial.println(body);

  http.begin(url);
  addDeviceHeaders(http);

  int statusCode = http.POST(body);
  String response = http.getString();

  Serial.print("HTTP status: ");
  Serial.println(statusCode);
  Serial.print("Response: ");
  Serial.println(response);

  http.end();

  return statusCode >= 200 && statusCode < 300;
}

void pollCommands()
{
  if (!isWiFiConnected())
  {
    Serial.println("Cannot poll commands: Wi-Fi is not connected.");
    return;
  }

  String url = String(API_BASE_URL) + "/api/device/commands?device_uuid=" + DEVICE_UUID;

  HTTPClient http;

  Serial.println();
  Serial.println("Polling commands...");
  Serial.print("URL: ");
  Serial.println(url);

  http.begin(url);
  http.addHeader("X-DEVICE-KEY", DEVICE_API_KEY);

  int statusCode = http.GET();
  String response = http.getString();

  Serial.print("HTTP status: ");
  Serial.println(statusCode);

  Serial.print("Response: ");
  Serial.println(response);

  if (statusCode == 200)
  {
    parseCommandResponse(response);
  }
  else
  {
    Serial.println("Command poll failed. Will retry later.");
  }

  http.end();
}

bool sendDeviceStateSync(int lastCompletedCommandId)
{
  if (!isWiFiConnected())
  {
    Serial.println("Cannot sync device state: Wi-Fi is not connected.");
    return false;
  }

  String operationState = isWateringActive() ? "watering" : "idle";
  String valveState = isValveOpen() ? "open" : "closed";
  String wateringState = isWateringActive() ? "watering" : "idle";

  String url = String(API_BASE_URL) + "/api/device/state";

  String body = "{";
  body += "\"device_uuid\":\"";
  body += DEVICE_UUID;
  body += "\",";
  body += "\"device_type\":\"plant_bed_controller\",";
  body += "\"firmware_version\":\"v0.1.0\",";
  body += "\"operation_state\":\"";
  body += operationState;
  body += "\",";
  body += "\"valve_state\":\"";
  body += valveState;
  body += "\",";
  body += "\"watering_state\":\"";
  body += wateringState;
  body += "\"";

  if (lastCompletedCommandId > 0)
  {
    body += ",\"last_completed_command_id\":";
    body += String(lastCompletedCommandId);
  }

  body += "}";

  HTTPClient http;

  Serial.println();
  Serial.println("Syncing device state...");
  Serial.print("URL: ");
  Serial.println(url);
  Serial.print("Body: ");
  Serial.println(body);

  http.begin(url);
  addDeviceHeaders(http);

  int statusCode = http.POST(body);
  String response = http.getString();

  Serial.print("HTTP status: ");
  Serial.println(statusCode);
  Serial.print("Response: ");
  Serial.println(response);

  http.end();

  return statusCode >= 200 && statusCode < 300;
}

bool sendSensorReading(const SensorReading &reading)
{
  if (!isWiFiConnected())
  {
    Serial.println("Cannot send sensor reading: Wi-Fi is not connected.");
    return false;
  }

  String url = String(API_BASE_URL) + "/api/device/readings";

  String body = "{";
  body += "\"device_uuid\":\"";
  body += DEVICE_UUID;
  body += "\",";
  body += "\"soil_moisture_percent\":";
  body += String(reading.soilMoisturePercent);
  body += ",";
  body += "\"water_level_percent\":";
  body += String(reading.waterLevelPercent);
  body += ",";
  body += "\"temperature_c\":";
  body += String(reading.temperatureC, 1);
  body += ",";
  body += "\"humidity_percent\":";
  body += String(reading.humidityPercent, 1);
  body += "}";

  HTTPClient http;

  Serial.println();
  Serial.println("Sending sensor reading...");
  Serial.print("URL: ");
  Serial.println(url);
  Serial.print("Body: ");
  Serial.println(body);

  http.begin(url);
  addDeviceHeaders(http);

  int statusCode = http.POST(body);
  String response = http.getString();

  Serial.print("HTTP status: ");
  Serial.println(statusCode);
  Serial.print("Response: ");
  Serial.println(response);

  http.end();

  return statusCode >= 200 && statusCode < 300;
}