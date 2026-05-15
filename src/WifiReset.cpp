#include "WifiReset.h"

#include <Arduino.h>

#include "DeviceStorage.h"
#include "PinConfig.h"

// Button pressed = LOW.
// Pin is selected by board profile:
// - ESP32 DevKit: GPIO0 / onboard BOOT button
// - ESP32-C3 Super Mini: GPIO7 external button, because GPIO9 is used for I2C SCL
static const unsigned long WIFI_RESET_HOLD_MS = 3000;

void checkWifiResetOnBoot()
{
    pinMode(WIFI_RESET_BUTTON_PIN, INPUT_PULLUP);

    Serial.println();
    Serial.println("Checking Wi-Fi reset button...");
    Serial.print("Wi-Fi reset GPIO: ");
    Serial.println(WIFI_RESET_BUTTON_PIN);
    Serial.println("Hold Wi-Fi reset button during startup to clear saved Wi-Fi.");

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
