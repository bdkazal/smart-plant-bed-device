#include "ValveDriver.h"

#include <Arduino.h>

#include "PinConfig.h"

bool valveDriverOpen = false;

int valveOnLevel()
{
    return VALVE_ACTIVE_LOW ? LOW : HIGH;
}

int valveOffLevel()
{
    return VALVE_ACTIVE_LOW ? HIGH : LOW;
}

void beginValveDriver()
{
    pinMode(VALVE_CONTROL_PIN, OUTPUT);
    digitalWrite(VALVE_CONTROL_PIN, valveOffLevel());
    valveDriverOpen = false;

    Serial.println();
    Serial.println("Valve driver initialized.");
    Serial.print("Valve control GPIO: ");
    Serial.println(VALVE_CONTROL_PIN);
    Serial.print("Valve active mode: ");
    Serial.println(VALVE_ACTIVE_LOW ? "ACTIVE LOW" : "ACTIVE HIGH");
    Serial.println("Valve output state: OFF");
}

void setValveDriverOpen(bool open)
{
    valveDriverOpen = open;

    digitalWrite(
        VALVE_CONTROL_PIN,
        valveDriverOpen ? valveOnLevel() : valveOffLevel()
    );

    Serial.print("Valve driver GPIO state: ");
    Serial.println(valveDriverOpen ? "ON" : "OFF");
}
