#include "WiFiMan.h"

#include <Arduino.h>
#include <WiFi.h>

#include "secrets.h"

bool isWiFiConnected()
{
    return WiFi.status() == WL_CONNECTED;
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