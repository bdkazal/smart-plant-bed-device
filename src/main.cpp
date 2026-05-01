#include <Arduino.h>

#include "WiFiMan.h"
#include "ApiClient.h"
#include "ValveController.h"
#include "SensorReader.h"
#include "DeviceStorage.h"
#include "SetupPortal.h"
#include "WifiReset.h"
#include "DeviceIdentity.h"
#include "FirmwareInfo.h"
#include "ValveDriver.h"
#include "StatusLed.h"
#include "ManualButton.h"
#include "AppConfig.h"
#include "TimeSync.h"

// Backend contract timing
const unsigned long HEARTBEAT_INTERVAL_MS = 15000;
const unsigned long COMMAND_POLL_INTERVAL_MS = 5000;
const unsigned long READING_INTERVAL_MS = 30000;
const unsigned long CONFIG_REFRESH_INTERVAL_MS = 60000;

unsigned long lastHeartbeatAt = 0;
unsigned long lastCommandPollAt = 0;
unsigned long lastReadingAt = 0;
unsigned long lastConfigRefreshAt = 0;

void loadCachedLaravelConfigIfAvailable()
{
  String cachedConfigJson = loadCachedConfigJson();

  if (cachedConfigJson.length() == 0)
  {
    Serial.println("No cached Laravel config found.");
    return;
  }

  Serial.println();
  Serial.println("Loading cached Laravel config from flash...");

  bool parsed = parseConfigObjectJson(cachedConfigJson);

  if (!parsed)
  {
    Serial.println("Cached Laravel config exists but could not be parsed.");
    return;
  }

  Serial.println("Cached Laravel config loaded.");
}

void runOnlineStartupTasks()
{
  fetchConfig();
  syncTimeFromNtp(deviceConfig.timezone);

  sendHeartbeat();
  sendDeviceStateSync(0);
  pollCommands();

  SensorReading reading = readSensors();
  sendSensorReading(reading);

  unsigned long now = millis();

  lastHeartbeatAt = now;
  lastCommandPollAt = now;
  lastReadingAt = now;
  lastConfigRefreshAt = now;
}

void setup()
{
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("Smart Plant Bed ESP32 starting...");

  beginDeviceStorage();

  beginValveDriver();
  beginStatusLed();
  beginManualButton();
  beginSensorReader();
  beginTimeSync();

  printDeviceIdentity();
  printFirmwareInfo();

  checkWifiResetOnBoot();

  StoredDeviceConfig storedConfig = loadStoredDeviceConfig();

  loadCachedLaravelConfigIfAvailable();

  if (!storedConfig.hasWifiCredentials)
  {
    Serial.println("No Wi-Fi saved. Starting setup portal.");
    startSetupPortal();
    return;
  }

  connectToWiFiUsingConfig(storedConfig);

  if (isWiFiConnected())
  {
    setWifiStatusLedConnected();
    runOnlineStartupTasks();
  }
}

void loop()
{
  if (isSetupPortalActive())
  {
    updateWifiStatusLedDisconnected();
    handleSetupPortal();
    return;
  }

  updateManualButton();

  if (!isWiFiConnected())
  {
    updateWifiStatusLedDisconnected();

    Serial.println("Wi-Fi disconnected. Reconnecting...");
    connectToWiFi();

    if (isWiFiConnected())
    {
      setWifiStatusLedConnected();

      Serial.println("Reconnected. Running online startup tasks...");
      runOnlineStartupTasks();
    }
  }
  else
  {
    setWifiStatusLedConnected();
  }

  updateWateringState();

  unsigned long now = millis();

  if (now - lastHeartbeatAt >= HEARTBEAT_INTERVAL_MS)
  {
    sendHeartbeat();
    sendDeviceStateSync(0);

    Serial.print("Laravel reachable recently: ");
    Serial.println(isServerRecentlyReachable() ? "yes" : "no");

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

  if (now - lastConfigRefreshAt >= CONFIG_REFRESH_INTERVAL_MS)
  {
    Serial.println();
    Serial.println("Refreshing device config...");
    fetchConfig();
    lastConfigRefreshAt = now;
  }
}
