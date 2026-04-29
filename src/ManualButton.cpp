#include "ManualButton.h"

#include <Arduino.h>

#include "AppConfig.h"
#include "PinConfig.h"
#include "ValveController.h"

static const unsigned long BUTTON_DEBOUNCE_MS = 50;

bool lastButtonReading = HIGH;
bool stableButtonState = HIGH;
unsigned long lastButtonChangeAt = 0;

void beginManualButton()
{
    pinMode(MANUAL_WATER_BUTTON_PIN, INPUT_PULLUP);

    lastButtonReading = digitalRead(MANUAL_WATER_BUTTON_PIN);
    stableButtonState = lastButtonReading;
    lastButtonChangeAt = millis();

    Serial.println();
    Serial.println("Manual watering button initialized.");
    Serial.print("Manual button GPIO: ");
    Serial.println(MANUAL_WATER_BUTTON_PIN);
    Serial.println("Button mode: INPUT_PULLUP");
}

int getLocalManualDurationSeconds()
{
    if (deviceConfig.localManualDurationSeconds > 0)
    {
        return deviceConfig.localManualDurationSeconds;
    }

    // Safe fallback if config has not loaded yet.
    return 30;
}

void handleManualButtonPress()
{
    Serial.println();
    Serial.println("Manual watering button pressed.");

    if (isWateringActive())
    {
        stopLocalWatering();
        return;
    }

    int durationSeconds = getLocalManualDurationSeconds();

    startLocalWatering(durationSeconds);
}

void updateManualButton()
{
    bool currentReading = digitalRead(MANUAL_WATER_BUTTON_PIN);
    unsigned long now = millis();

    if (currentReading != lastButtonReading)
    {
        lastButtonChangeAt = now;
        lastButtonReading = currentReading;
    }

    if (now - lastButtonChangeAt < BUTTON_DEBOUNCE_MS)
    {
        return;
    }

    if (currentReading == stableButtonState)
    {
        return;
    }

    stableButtonState = currentReading;

    // Button pressed because we use INPUT_PULLUP.
    if (stableButtonState == LOW)
    {
        handleManualButtonPress();
    }
}