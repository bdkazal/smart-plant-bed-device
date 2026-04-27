#include "WiFiMan.h"

#include <Arduino.h>
#include <WiFi.h>

#include "secrets.h"
#include "DeviceStorage.h"

bool isWiFiConnected()
{
    return WiFi.status() == WL_CONNECTED;
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
        Serial.println("Wi-Fi connected successfully.");
        Serial.print("ESP32 IP address: ");
        Serial.println(WiFi.localIP());
        Serial.print("Signal strength RSSI: ");
        Serial.println(WiFi.RSSI());
        return true;
    }

    Serial.println("Wi-Fi connection failed.");
    return false;
}

void connectToWiFi()
{
    StoredDeviceConfig storedConfig = loadStoredDeviceConfig();

    if (storedConfig.hasWifiCredentials)
    {
        Serial.println("Trying stored Wi-Fi credentials...");

        bool connected = connectWithCredentials(
            storedConfig.wifiSsid,
            storedConfig.wifiPassword
        );

        if (connected)
        {
            return;
        }

        Serial.println("Stored Wi-Fi failed. Falling back to development secrets.");
    }
    else
    {
        Serial.println("No stored Wi-Fi. Using development secrets.");
    }

    connectWithCredentials(WIFI_SSID, WIFI_PASSWORD);
}