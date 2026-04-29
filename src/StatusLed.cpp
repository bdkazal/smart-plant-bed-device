#include "StatusLed.h"

#include <Arduino.h>

#include "PinConfig.h"

static const unsigned long WIFI_BLINK_INTERVAL_MS = 2000;

bool wifiLedBlinkState = false;
unsigned long lastWifiBlinkAt = 0;

int wifiLedOnLevel()
{
    return WIFI_STATUS_LED_ACTIVE_LOW ? LOW : HIGH;
}

int wifiLedOffLevel()
{
    return WIFI_STATUS_LED_ACTIVE_LOW ? HIGH : LOW;
}

void writeWifiLed(bool on)
{
    digitalWrite(
        WIFI_STATUS_LED_PIN,
        on ? wifiLedOnLevel() : wifiLedOffLevel());
}

void beginStatusLed()
{
    pinMode(WIFI_STATUS_LED_PIN, OUTPUT);

    wifiLedBlinkState = false;
    lastWifiBlinkAt = 0;

    writeWifiLed(false);

    Serial.println();
    Serial.println("Status LED initialized.");
    Serial.print("Wi-Fi status LED GPIO: ");
    Serial.println(WIFI_STATUS_LED_PIN);
    Serial.print("Wi-Fi status LED active mode: ");
    Serial.println(WIFI_STATUS_LED_ACTIVE_LOW ? "ACTIVE LOW" : "ACTIVE HIGH");
}

void setWifiStatusLedConnected()
{
    wifiLedBlinkState = true;
    writeWifiLed(true);
}

void updateWifiStatusLedDisconnected()
{
    unsigned long now = millis();

    if (now - lastWifiBlinkAt < WIFI_BLINK_INTERVAL_MS)
    {
        return;
    }

    lastWifiBlinkAt = now;
    wifiLedBlinkState = !wifiLedBlinkState;

    writeWifiLed(wifiLedBlinkState);
}