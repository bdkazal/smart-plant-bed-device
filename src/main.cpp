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
#include "LocalAutomation.h"

// Backend contract timing
const unsigned long HEARTBEAT_INTERVAL_MS = 15000;
const unsigned long COMMAND_POLL_INTERVAL_MS = 5000;
const unsigned long READING_INTERVAL_MS = 30000;
const unsigned long CONFIG_REFRESH_INTERVAL_MS = 60000;

// Slower retry timing when Wi-Fi is connected but Laravel is not reachable.
// This keeps the device responsive without spamming failed API requests.
const unsigned long OFFLINE_HEARTBEAT_INTERVAL_MS = 30000;
const unsigned long OFFLINE_COMMAND_POLL_INTERVAL_MS = 30000;
const unsigned long OFFLINE_CONFIG_REFRESH_INTERVAL_MS = 120000;

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

unsigned long getHeartbeatIntervalMs()
{
  return isServerRecentlyReachable() ? HEARTBEAT_INTERVAL_MS : OFFLINE_HEARTBEAT_INTERVAL_MS;
}

unsigned long getCommandPollIntervalMs()
{
  return isServerRecentlyReachable() ? COMMAND_POLL_INTERVAL_MS : OFFLINE_COMMAND_POLL_INTERVAL_MS;
}

unsigned long getConfigRefreshIntervalMs()
{
  return isServerRecentlyReachable() ? CONFIG_REFRESH_INTERVAL_MS : OFFLINE_CONFIG_REFRESH_INTERVAL_MS;
}

void handleSensorReadingCycle()
{
  SensorReading reading = readSensors();

  if (isServerRecentlyReachable())
  {
    sendSensorReading(reading);
  }
  else
  {
    Serial.println("Laravel not reachable. Sensor reading kept local for fallback automation.");
  }

  updateLocalAutomation(reading);
}

void runOnlineStartupTasks()
{
  fetchConfig();
  syncTimeFromNtp(deviceConfig.timezone);

  sendHeartbeat();
  sendDeviceStateSync(0);
  pollCommands();

  handleSensorReadingCycle();

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
  beginLocalAutomation();

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

  if (now - lastHeartbeatAt >= getHeartbeatIntervalMs())
  {
    sendHeartbeat();
    sendDeviceStateSync(0);

    Serial.print("Laravel reachable recently: ");
    Serial.println(isServerRecentlyReachable() ? "yes" : "no");

    lastHeartbeatAt = now;
  }

  if (now - lastCommandPollAt >= getCommandPollIntervalMs())
  {
    pollCommands();
    lastCommandPollAt = now;
  }

  if (now - lastReadingAt >= READING_INTERVAL_MS)
  {
    handleSensorReadingCycle();
    lastReadingAt = now;
  }

  if (now - lastConfigRefreshAt >= getConfigRefreshIntervalMs())
  {
    Serial.println();
    Serial.println("Refreshing device config...");
    fetchConfig();
    lastConfigRefreshAt = now;
  }
}
