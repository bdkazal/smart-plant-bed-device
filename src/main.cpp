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
#include "DisplayButton.h"
#include "AppConfig.h"
#include "TimeSync.h"
#include "LocalAutomation.h"
#include "DisplayManager.h"
#include "PinConfig.h"

// Backend contract timing
const unsigned long HEARTBEAT_INTERVAL_MS = 15000;
const unsigned long COMMAND_POLL_INTERVAL_MS = 5000;
const unsigned long READING_INTERVAL_MS = 30000;
const unsigned long CONFIG_REFRESH_INTERVAL_MS = 60000;
const unsigned long SCHEDULE_CHECK_INTERVAL_MS = 5000;

// Slower retry timing when Wi-Fi is connected but Laravel is not reachable.
// This keeps the device responsive without spamming failed API requests.
const unsigned long OFFLINE_HEARTBEAT_INTERVAL_MS = 30000;
const unsigned long OFFLINE_COMMAND_POLL_INTERVAL_MS = 30000;
const unsigned long OFFLINE_CONFIG_REFRESH_INTERVAL_MS = 120000;

unsigned long lastHeartbeatAt = 0;
unsigned long lastCommandPollAt = 0;
unsigned long lastReadingAt = 0;
unsigned long lastConfigRefreshAt = 0;
unsigned long lastScheduleCheckAt = 0;

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
  displaySetLatestSensorReading(reading);

  if (isServerRecentlyReachable())
  {
    sendSensorReading(reading);
  }
  else
  {
    Serial.println("Laravel not reachable. Sensor reading kept local for fallback automation.");
  }

  updateLocalAutomation(reading);
  displayShowCriticalIfNeeded();
}

void runOnlineStartupTasks()
{
  fetchConfig();
  syncTimeFromNtp(deviceConfig.timezone);

  sendHeartbeat();
  sendDeviceStateSync(0);
  pollCommands();

  handleSensorReadingCycle();
  updateLocalScheduleFallback();
  displayShowCurrentStatus(OLED_STATUS_SHOW_MS);

  unsigned long now = millis();

  lastHeartbeatAt = now;
  lastCommandPollAt = now;
  lastReadingAt = now;
  lastConfigRefreshAt = now;
  lastScheduleCheckAt = now;
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
  beginDisplayButton();
  beginSensorReader();
  beginTimeSync();
  beginLocalAutomation();
  beginDisplayManager();

  displayShowBootLogo(2500);
  delay(2500);

  displayShowBootStatus("Starting device", "Loading config", "Please wait");

  printDeviceIdentity();
  printFirmwareInfo();

  checkWifiResetOnBoot();

  StoredDeviceConfig storedConfig = loadStoredDeviceConfig();

  loadCachedLaravelConfigIfAvailable();

  if (!storedConfig.hasWifiCredentials)
  {
    Serial.println("No Wi-Fi saved. Starting setup portal.");
    displayShowBootStatus("WiFi setup", "Portal active", "Connect phone");
    startSetupPortal();
    return;
  }

  displayShowBootStatus("Connecting WiFi", storedConfig.wifiSsid, "");
  connectToWiFiUsingConfig(storedConfig);

  if (isWiFiConnected())
  {
    setWifiStatusLedConnected();
    displayShowBootStatus("WiFi connected", "Fetching config", "Syncing time");
    runOnlineStartupTasks();
  }
  else
  {
    displayShowBootStatus("WiFi failed", "Using cached cfg", "Offline mode");
  }
}

void loop()
{
  if (isSetupPortalActive())
  {
    updateWifiStatusLedDisconnected();
    handleSetupPortal();
    updateDisplayButton();
    updateDisplayManager();
    return;
  }

  updateManualButton();
  updateDisplayButton();

  if (!isWiFiConnected())
  {
    updateWifiStatusLedDisconnected();

    Serial.println("Wi-Fi disconnected. Reconnecting...");
    connectToWiFi();

    if (isWiFiConnected())
    {
      setWifiStatusLedConnected();

      Serial.println("Reconnected. Running online startup tasks...");
      displayShowBootStatus("WiFi reconnected", "Running sync", "");
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

  if (now - lastScheduleCheckAt >= SCHEDULE_CHECK_INTERVAL_MS)
  {
    updateLocalScheduleFallback();
    lastScheduleCheckAt = now;
  }

  if (now - lastConfigRefreshAt >= getConfigRefreshIntervalMs())
  {
    Serial.println();
    Serial.println("Refreshing device config...");
    fetchConfig();
    lastConfigRefreshAt = now;
  }

  updateDisplayManager();
}
