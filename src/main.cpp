#include <Arduino.h>

#include "WiFiMan.h"
#include "ApiClient.h"
#include "ValveController.h"
#include "SensorReader.h"
#include "DeviceStorage.h"
#include "SetupPortal.h"
#include "WifiReset.h"
#include "DeviceIdentity.h"

// Backend contract timing
const unsigned long HEARTBEAT_INTERVAL_MS = 15000;
const unsigned long COMMAND_POLL_INTERVAL_MS = 5000;
const unsigned long READING_INTERVAL_MS = 30000;

unsigned long lastHeartbeatAt = 0;
unsigned long lastCommandPollAt = 0;
unsigned long lastReadingAt = 0;

void runOnlineStartupTasks()
{
  fetchConfig();

  sendHeartbeat();
  sendDeviceStateSync(0);
  pollCommands();

  SensorReading reading = readSensors();
  sendSensorReading(reading);

  unsigned long now = millis();

  lastHeartbeatAt = now;
  lastCommandPollAt = now;
  lastReadingAt = now;
}

void setup()
{
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("Smart Plant Bed ESP32 starting...");

  beginDeviceStorage();

  printDeviceIdentity();

  checkWifiResetOnBoot();

  StoredDeviceConfig storedConfig = loadStoredDeviceConfig();

  if (!storedConfig.hasWifiCredentials)
  {
    Serial.println("No Wi-Fi saved. Starting setup portal.");
    startSetupPortal();
    return;
  }

  connectToWiFiUsingConfig(storedConfig);
  if (isWiFiConnected())
  {
    runOnlineStartupTasks();
  }
}

void loop()
{
  if (isSetupPortalActive())
  {
    handleSetupPortal();
    return;
  }

  if (!isWiFiConnected())
  {
    Serial.println("Wi-Fi disconnected. Reconnecting...");
    connectToWiFi();

    if (isWiFiConnected())
    {
      Serial.println("Reconnected. Running online startup tasks...");
      runOnlineStartupTasks();
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

  if (now - lastReadingAt >= READING_INTERVAL_MS)
  {
    SensorReading reading = readSensors();
    sendSensorReading(reading);
    lastReadingAt = now;
  }
}