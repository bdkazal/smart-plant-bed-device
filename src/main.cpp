#include <Arduino.h>
#include <ArduinoJson.h>

#include "AppConfig.h"
#include "WiFiMan.h"
#include "ApiClient.h"

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
  int interruptedCommandId = activeCommandId;

  closeFakeValve();

  if (interruptedCommandId > 0 && interruptedCommandId != commandId)
  {
    Serial.println();
    Serial.print("Closing interrupted valve_on command: ");
    Serial.println(interruptedCommandId);

    bool previousExecuted = sendCommandAck(interruptedCommandId, "executed");

    if (!previousExecuted)
    {
      Serial.println("Warning: failed to mark interrupted valve_on command as executed.");
    }
  }

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