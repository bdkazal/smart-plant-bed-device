#include <Arduino.h>
#include <ArduinoJson.h>

#include "AppConfig.h"
#include "WiFiMan.h"
#include "ApiClient.h"
#include "ValveController.h"

// Backend contract timing
const unsigned long HEARTBEAT_INTERVAL_MS = 15000;
const unsigned long COMMAND_POLL_INTERVAL_MS = 5000;

unsigned long lastHeartbeatAt = 0;
unsigned long lastCommandPollAt = 0;

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