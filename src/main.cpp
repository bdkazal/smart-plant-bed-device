#include <Arduino.h>

#include "WiFiMan.h"
#include "ApiClient.h"
#include "ValveController.h"
#include "SensorReader.h"
#include "DeviceStorage.h"

// Backend contract timing
const unsigned long HEARTBEAT_INTERVAL_MS = 15000;
const unsigned long COMMAND_POLL_INTERVAL_MS = 5000;
const unsigned long READING_INTERVAL_MS = 30000;

unsigned long lastHeartbeatAt = 0;
unsigned long lastCommandPollAt = 0;
unsigned long lastReadingAt = 0;

void setup()
{
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("Smart Plant Bed ESP32 starting...");

  beginDeviceStorage();

  connectToWiFi();

  if (isWiFiConnected())
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
      sendDeviceStateSync(0);
      pollCommands();

      SensorReading reading = readSensors();
      sendSensorReading(reading);

      unsigned long now = millis();

      lastHeartbeatAt = now;
      lastCommandPollAt = now;
      lastReadingAt = now;
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