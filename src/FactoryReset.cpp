#include "FactoryReset.h"

#include <Arduino.h>

#include "DeviceStorage.h"

// Most ESP32 dev boards use GPIO 0 as the BOOT button.
// Button pressed = LOW.
static const int FACTORY_RESET_BUTTON_PIN = 0;
static const unsigned long FACTORY_RESET_HOLD_MS = 3000;

void checkFactoryResetOnBoot()
{
    pinMode(FACTORY_RESET_BUTTON_PIN, INPUT_PULLUP);

    Serial.println();
    Serial.println("Checking factory reset button...");
    Serial.println("Hold BOOT button during startup to clear saved Wi-Fi.");

    if (digitalRead(FACTORY_RESET_BUTTON_PIN) == HIGH)
    {
        Serial.println("Factory reset button not pressed.");
        return;
    }

    Serial.println("Factory reset button pressed. Keep holding...");

    unsigned long startedAt = millis();

    while (millis() - startedAt < FACTORY_RESET_HOLD_MS)
    {
        if (digitalRead(FACTORY_RESET_BUTTON_PIN) == HIGH)
        {
            Serial.println("Factory reset cancelled. Button released too early.");
            return;
        }

        Serial.print(".");
        delay(250);
    }

    Serial.println();
    Serial.println("Factory reset confirmed.");
    clearStoredDeviceConfig();

    Serial.println("Saved Wi-Fi cleared. Restarting...");
    delay(1000);

    ESP.restart();
}