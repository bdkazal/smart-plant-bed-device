#include "WiFiMan.h"

#include <Arduino.h>
#include <WiFi.h>

#include "secrets.h"
#include "DeviceStorage.h"

static const unsigned long WIFI_RECONNECT_INTERVAL_MS = 15000;
static const unsigned long WIFI_RECONNECT_STATUS_MS = 5000;

unsigned long lastReconnectAttemptAt = 0;
unsigned long lastReconnectStatusAt = 0;
bool reconnectInProgress = false;

bool isWiFiConnected()
{
    return WiFi.status() == WL_CONNECTED;
}

void printWiFiConnectedInfo()
{
    Serial.println("Wi-Fi connected successfully.");
    Serial.print("ESP32 IP address: ");
    Serial.println(WiFi.localIP());
    Serial.print("Signal strength RSSI: ");
    Serial.println(WiFi.RSSI());
}

bool connectWithCredentials(const String &ssid, const String &password)
{
    if (ssid.length() == 0)
    {
        Serial.println("Cannot connect to Wi-Fi: SSID is empty.");
        return false;
    }

    Serial.println();
    Serial.println("Connecting to Wi-Fi...");
    Serial.print("SSID: ");
    Serial.println(ssid);

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), password.c_str());

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
        printWiFiConnectedInfo();
        reconnectInProgress = false;
        return true;
    }

    Serial.println("Wi-Fi connection failed.");
    reconnectInProgress = false;
    return false;
}

bool connectToWiFiUsingConfig(const StoredDeviceConfig &storedConfig)
{
    if (storedConfig.hasWifiCredentials)
    {
        Serial.println("Trying stored Wi-Fi credentials...");

        bool connected = connectWithCredentials(
            storedConfig.wifiSsid,
            storedConfig.wifiPassword);

        if (connected)
        {
            return true;
        }

        Serial.println("Stored Wi-Fi failed. Falling back to development secrets.");
    }
    else
    {
        Serial.println("No stored Wi-Fi. Using development secrets.");
    }

    return connectWithCredentials(WIFI_SSID, WIFI_PASSWORD);
}

void connectToWiFi()
{
    StoredDeviceConfig storedConfig = loadStoredDeviceConfig();
    connectToWiFiUsingConfig(storedConfig);
}

void startNonBlockingReconnect()
{
    StoredDeviceConfig storedConfig = loadStoredDeviceConfig();

    String ssid = WIFI_SSID;
    String password = WIFI_PASSWORD;

    if (storedConfig.hasWifiCredentials)
    {
        ssid = storedConfig.wifiSsid;
        password = storedConfig.wifiPassword;
    }

    if (ssid.length() == 0)
    {
        Serial.println("Non-blocking Wi-Fi reconnect skipped: SSID is empty.");
        return;
    }

    Serial.println();
    Serial.println("Starting non-blocking Wi-Fi reconnect...");
    Serial.print("SSID: ");
    Serial.println(ssid);

    WiFi.mode(WIFI_STA);
    WiFi.disconnect(false, false);
    delay(10);
    WiFi.begin(ssid.c_str(), password.c_str());

    reconnectInProgress = true;
    lastReconnectStatusAt = millis();
}

void updateWiFiReconnect()
{
    unsigned long now = millis();

    if (isWiFiConnected())
    {
        if (reconnectInProgress)
        {
            Serial.println();
            Serial.println("Non-blocking Wi-Fi reconnect completed.");
            printWiFiConnectedInfo();
        }

        reconnectInProgress = false;
        return;
    }

    if (reconnectInProgress)
    {
        if (now - lastReconnectStatusAt >= WIFI_RECONNECT_STATUS_MS)
        {
            Serial.println("Wi-Fi reconnect still in progress. Local controls remain active.");
            lastReconnectStatusAt = now;
        }

        // If an attempt has not connected within the retry interval, start a fresh attempt.
        if (now - lastReconnectAttemptAt < WIFI_RECONNECT_INTERVAL_MS)
        {
            return;
        }
    }

    if (now - lastReconnectAttemptAt < WIFI_RECONNECT_INTERVAL_MS)
    {
        return;
    }

    lastReconnectAttemptAt = now;
    startNonBlockingReconnect();
}