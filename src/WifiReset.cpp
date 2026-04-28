#include "WifiReset.h"

#include <Arduino.h>

#include "DeviceStorage.h"

// Most ESP32 dev boards use GPIO 0 as the BOOT button.
// Button pressed = LOW.
static const int WIFI_RESET_BUTTON_PIN = 0;
static const unsigned long WIFI_RESET_HOLD_MS = 3000;

void checkWifiResetOnBoot()
{
    pinMode(WIFI_RESET_BUTTON_PIN, INPUT_PULLUP);

    Serial.println();
    Serial.println("Checking Wi-Fi reset button...");
    Serial.println("Hold BOOT button during startup to clear saved Wi-Fi.");

    if (digitalRead(WIFI_RESET_BUTTON_PIN) == HIGH)
    {
        Serial.println("Wi-Fi reset button not pressed.");
        return;
    }

    Serial.println("Wi-Fi reset button pressed. Keep holding...");

    unsigned long startedAt = millis();

    while (millis() - startedAt < WIFI_RESET_HOLD_MS)
    {
        if (digitalRead(WIFI_RESET_BUTTON_PIN) == HIGH)
        {
            Serial.println("Wi-Fi reset cancelled. Button released too early.");
            return;
        }

        Serial.print(".");
        delay(250);
    }

    Serial.println();
    Serial.println("Wi-Fi reset confirmed.");
    clearStoredDeviceConfig();

    Serial.println("Saved Wi-Fi cleared. Restarting...");
    delay(1000);

    ESP.restart();
}