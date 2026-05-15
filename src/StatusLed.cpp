#include "StatusLed.h"

#include <Arduino.h>

#include "PinConfig.h"

static const unsigned long WIFI_BLINK_INTERVAL_MS = 2000;

bool wifiLedBlinkState = false;
unsigned long lastWifiBlinkAt = 0;

int ledOnLevel(bool activeLow)
{
    return activeLow ? LOW : HIGH;
}

int ledOffLevel(bool activeLow)
{
    return activeLow ? HIGH : LOW;
}

bool isPinEnabled(int pin)
{
    return pin >= 0;
}

void safePinMode(int pin, uint8_t mode)
{
    if (!isPinEnabled(pin))
    {
        return;
    }

    pinMode(pin, mode);
}

void safeDigitalWrite(int pin, int value)
{
    if (!isPinEnabled(pin))
    {
        return;
    }

    digitalWrite(pin, value);
}

void writeWifiLed(bool on)
{
    safeDigitalWrite(
        WIFI_STATUS_LED_PIN,
        on ? ledOnLevel(WIFI_STATUS_LED_ACTIVE_LOW) : ledOffLevel(WIFI_STATUS_LED_ACTIVE_LOW));
}

void writeWateringLed(bool on)
{
    safeDigitalWrite(
        WATERING_STATUS_LED_PIN,
        on ? ledOnLevel(WATERING_STATUS_LED_ACTIVE_LOW) : ledOffLevel(WATERING_STATUS_LED_ACTIVE_LOW));
}

void beginStatusLed()
{
    safePinMode(WIFI_STATUS_LED_PIN, OUTPUT);
    safePinMode(WATERING_STATUS_LED_PIN, OUTPUT);

    wifiLedBlinkState = false;
    lastWifiBlinkAt = 0;

    writeWifiLed(false);
    writeWateringLed(false);

    Serial.println();
    Serial.println("Status LEDs initialized.");
    Serial.print("Wi-Fi status LED GPIO: ");
    if (isPinEnabled(WIFI_STATUS_LED_PIN))
    {
        Serial.println(WIFI_STATUS_LED_PIN);
    }
    else
    {
        Serial.println("disabled");
    }
    Serial.print("Wi-Fi status LED active mode: ");
    Serial.println(WIFI_STATUS_LED_ACTIVE_LOW ? "ACTIVE LOW" : "ACTIVE HIGH");
    Serial.print("Watering status LED GPIO: ");
    if (isPinEnabled(WATERING_STATUS_LED_PIN))
    {
        Serial.println(WATERING_STATUS_LED_PIN);
    }
    else
    {
        Serial.println("disabled - mirrors valve output physically");
    }
    Serial.print("Watering status LED active mode: ");
    Serial.println(WATERING_STATUS_LED_ACTIVE_LOW ? "ACTIVE LOW" : "ACTIVE HIGH");
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

void setWateringStatusLed(bool watering)
{
    writeWateringLed(watering);
}
