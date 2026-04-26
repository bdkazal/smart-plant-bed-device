#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "secrets.h"

// Backend contract timing
const unsigned long HEARTBEAT_INTERVAL_MS = 15000;
const unsigned long COMMAND_POLL_INTERVAL_MS = 5000;

unsigned long lastHeartbeatAt = 0;
unsigned long lastCommandPollAt = 0;

// Fake valve / watering runtime state.
// Later this will control a real GPIO relay/MOSFET.
bool valveOpen = false;
bool wateringActive = false;
int activeCommandId = 0;
unsigned long wateringStartedAt = 0;
unsigned long wateringDurationMs = 0;

struct DeviceConfig
{
  String deviceName;
  String timezone;
  String wateringMode;
  int soilMoistureThreshold = 0;
  int maxWateringDurationSeconds = 0;
  int cooldownMinutes = 0;
  int localManualDurationSeconds = 0;
};

DeviceConfig deviceConfig;

bool isWiFiConnected()
{
  return WiFi.status() == WL_CONNECTED;
}

void addDeviceHeaders(HTTPClient &http)
{
  http.addHeader("Content-Type", "application/json");
  http.addHeader("X-DEVICE-KEY", DEVICE_API_KEY);
}

void connectToWiFi()
{
  Serial.println();
  Serial.println("Connecting to Wi-Fi...");
  Serial.print("SSID: ");
  Serial.println(WIFI_SSID);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int attempts = 0;

  while (!isWiFiConnected() && attempts < 30)
  {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  Serial.println();

  if (isWiFiConnected())
  {
    Serial.println("Wi-Fi connected successfully.");
    Serial.print("ESP32 IP address: ");
    Serial.println(WiFi.localIP());
    Serial.print("Signal strength RSSI: ");
    Serial.println(WiFi.RSSI());
  }
  else
  {
    Serial.println("Wi-Fi connection failed.");
  }
}

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

void parseConfigResponse(const String &response)
{
  JsonDocument doc;

  DeserializationError error = deserializeJson(doc, response);

  if (error)
  {
    Serial.print("Failed to parse config JSON: ");
    Serial.println(error.c_str());
    return;
  }

  JsonObject config = doc["config"];

  if (config.isNull())
  {
    Serial.println("Config JSON does not contain a config object.");
    return;
  }

  deviceConfig.deviceName = config["device_name"] | "";
  deviceConfig.timezone = config["timezone"] | "";
  deviceConfig.wateringMode = config["watering_mode"] | "";
  deviceConfig.soilMoistureThreshold = config["soil_moisture_threshold"] | 0;
  deviceConfig.maxWateringDurationSeconds = config["max_watering_duration_seconds"] | 0;
  deviceConfig.cooldownMinutes = config["cooldown_minutes"] | 0;
  deviceConfig.localManualDurationSeconds = config["local_manual_duration_seconds"] | 0;

  printDeviceConfig();
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
    parseConfigResponse(response);
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

bool sendCommandAck(int commandId, const String &status, const String &message = "")
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

void openFakeValve()
{
  valveOpen = true;
  wateringActive = true;

  Serial.println();
  Serial.println("FAKE VALVE: OPEN");
  Serial.println("Watering state: watering");
}

void closeFakeValve()
{
  valveOpen = false;
  wateringActive = false;

  Serial.println();
  Serial.println("FAKE VALVE: CLOSED");
  Serial.println("Watering state: idle");
}

void startWateringCommand(int commandId, int durationSeconds)
{
  if (wateringActive)
  {
    Serial.println("Already watering. Ignoring new valve_on for now.");
    return;
  }

  if (durationSeconds <= 0)
  {
    Serial.println("Invalid duration. Sending failed ack.");
    sendCommandAck(commandId, "failed", "Invalid duration_seconds");
    return;
  }

  activeCommandId = commandId;
  wateringStartedAt = millis();
  wateringDurationMs = (unsigned long)durationSeconds * 1000UL;

  openFakeValve();

  bool acknowledged = sendCommandAck(commandId, "acknowledged");

  if (!acknowledged)
  {
    Serial.println("Warning: failed to send acknowledged ack. Local watering still started.");
  }

  Serial.print("Watering duration seconds: ");
  Serial.println(durationSeconds);
}

void stopWateringCommand(int commandId)
{
  closeFakeValve();

  bool acknowledged = sendCommandAck(commandId, "acknowledged");

  if (!acknowledged)
  {
    Serial.println("Warning: failed to send acknowledged ack for valve_off.");
  }

  bool executed = sendCommandAck(commandId, "executed");

  if (!executed)
  {
    Serial.println("Warning: failed to send executed ack for valve_off.");
  }

  activeCommandId = 0;
  wateringStartedAt = 0;
  wateringDurationMs = 0;
}

void parseCommandResponse(const String &response)
{
  JsonDocument doc;

  DeserializationError error = deserializeJson(doc, response);

  if (error)
  {
    Serial.print("Failed to parse command JSON: ");
    Serial.println(error.c_str());
    return;
  }

  if (doc["command"].isNull())
  {
    Serial.println("No pending command.");
    return;
  }

  JsonObject command = doc["command"];

  int commandId = command["id"] | 0;
  String commandType = command["command_type"] | "";
  String status = command["status"] | "";

  Serial.println();
  Serial.println("Pending command found:");
  Serial.print("Command ID: ");
  Serial.println(commandId);
  Serial.print("Command type: ");
  Serial.println(commandType);
  Serial.print("Status: ");
  Serial.println(status);

  if (commandType == "valve_on")
  {
    int durationSeconds = command["payload"]["duration_seconds"] | 0;

    Serial.print("Duration seconds: ");
    Serial.println(durationSeconds);

    startWateringCommand(commandId, durationSeconds);
  }
  else if (commandType == "valve_off")
  {
    stopWateringCommand(commandId);
  }
  else
  {
    Serial.println("Unknown command type. Sending failed ack.");
    sendCommandAck(commandId, "failed", "Unknown command type");
  }
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

void updateWateringState()
{
  if (!wateringActive)
  {
    return;
  }

  unsigned long now = millis();

  if (now - wateringStartedAt >= wateringDurationMs)
  {
    Serial.println();
    Serial.println("Watering duration completed.");

    closeFakeValve();

    if (activeCommandId > 0)
    {
      bool executed = sendCommandAck(activeCommandId, "executed");

      if (!executed)
      {
        Serial.println("Warning: failed to send executed ack.");
      }
    }

    activeCommandId = 0;
    wateringStartedAt = 0;
    wateringDurationMs = 0;
  }
}

void setup()
{
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("Smart Plant Bed ESP32 starting...");

  connectToWiFi();

  if (isWiFiConnected())
  {
    fetchConfig();
    sendHeartbeat();
    pollCommands();

    lastHeartbeatAt = millis();
    lastCommandPollAt = millis();
  }
}

void loop()
{
  if (!isWiFiConnected())
  {
    Serial.println("Wi-Fi disconnected. Reconnecting...");
    connectToWiFi();

    if (isWiFiConnected())
    {
      Serial.println("Reconnected. Fetching config again...");
      fetchConfig();
      sendHeartbeat();
      pollCommands();

      lastHeartbeatAt = millis();
      lastCommandPollAt = millis();
    }
  }

  updateWateringState();

  unsigned long now = millis();

  if (now - lastHeartbeatAt >= HEARTBEAT_INTERVAL_MS)
  {
    sendHeartbeat();
    lastHeartbeatAt = now;
  }

  if (now - lastCommandPollAt >= COMMAND_POLL_INTERVAL_MS)
  {
    pollCommands();
    lastCommandPollAt = now;
  }
}