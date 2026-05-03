#include "DisplayButton.h"

#include <Arduino.h>

#include "DisplayManager.h"
#include "PinConfig.h"

static const unsigned long DISPLAY_BUTTON_DEBOUNCE_MS = 50;

bool lastDisplayButtonReading = HIGH;
bool stableDisplayButtonState = HIGH;
unsigned long lastDisplayButtonChangeAt = 0;

void beginDisplayButton()
{
    pinMode(DISPLAY_WAKE_BUTTON_PIN, INPUT_PULLUP);

    lastDisplayButtonReading = digitalRead(DISPLAY_WAKE_BUTTON_PIN);
    stableDisplayButtonState = lastDisplayButtonReading;
    lastDisplayButtonChangeAt = millis();

    Serial.println();
    Serial.println("Display wake button initialized.");
    Serial.print("Display wake button GPIO: ");
    Serial.println(DISPLAY_WAKE_BUTTON_PIN);
    Serial.println("Button mode: INPUT_PULLUP");
}

void handleDisplayButtonPress()
{
    Serial.println();
    Serial.println("Display wake button pressed.");

    displayShowCriticalIfNeeded();
    displayShowCurrentStatus(OLED_WAKE_BUTTON_SHOW_MS);
}

void updateDisplayButton()
{
    bool currentReading = digitalRead(DISPLAY_WAKE_BUTTON_PIN);
    unsigned long now = millis();

    if (currentReading != lastDisplayButtonReading)
    {
        lastDisplayButtonChangeAt = now;
        lastDisplayButtonReading = currentReading;
    }

    if (now - lastDisplayButtonChangeAt < DISPLAY_BUTTON_DEBOUNCE_MS)
    {
        return;
    }

    if (currentReading == stableDisplayButtonState)
    {
        return;
    }

    stableDisplayButtonState = currentReading;

    // Button pressed because we use INPUT_PULLUP.
    if (stableDisplayButtonState == LOW)
    {
        handleDisplayButtonPress();
    }
}
