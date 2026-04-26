#include "ValveController.h"

#include <Arduino.h>

#include "ApiClient.h"

// Fake valve / watering runtime state.
// Later this will control a real GPIO relay/MOSFET.
bool valveOpen = false;
bool wateringActive = false;
int activeCommandId = 0;
unsigned long wateringStartedAt = 0;
unsigned long wateringDurationMs = 0;

bool isValveOpen()
{
    return valveOpen;
}

bool isWateringActive()
{
    return wateringActive;
}

int getActiveCommandId()
{
    return activeCommandId;
}

void openFakeValve()
{
    valveOpen = true;
    wateringActive = true;

    Serial.println();
    Serial.println("FAKE VALVE: OPEN");
    Serial.println("Watering state: watering");

    sendDeviceStateSync();
}

void closeFakeValve()
{
    valveOpen = false;
    wateringActive = false;

    Serial.println();
    Serial.println("FAKE VALVE: CLOSED");
    Serial.println("Watering state: idle");

    sendDeviceStateSync();
}
void startWateringCommand(int commandId, int durationSeconds)
{
    if (wateringActive)
    {
        Serial.println("Already watering. Ignoring new valve_on for now.");
        return;
    }

    if (durationSeconds <= 0)
    {
        Serial.println("Invalid duration. Sending failed ack.");
        sendCommandAck(commandId, "failed", "Invalid duration_seconds");
        return;
    }

    activeCommandId = commandId;
    wateringStartedAt = millis();
    wateringDurationMs = (unsigned long)durationSeconds * 1000UL;

    openFakeValve();

    bool acknowledged = sendCommandAck(commandId, "acknowledged");

    if (!acknowledged)
    {
        Serial.println("Warning: failed to send acknowledged ack. Local watering still started.");
    }

    Serial.print("Watering duration seconds: ");
    Serial.println(durationSeconds);
}

void stopWateringCommand(int commandId)
{
    int interruptedCommandId = activeCommandId;

    closeFakeValve();

    if (interruptedCommandId > 0 && interruptedCommandId != commandId)
    {
        Serial.println();
        Serial.print("Closing interrupted valve_on command: ");
        Serial.println(interruptedCommandId);

        bool previousExecuted = sendCommandAck(interruptedCommandId, "executed");

        if (!previousExecuted)
        {
            Serial.println("Warning: failed to mark interrupted valve_on command as executed.");
        }
    }

    bool acknowledged = sendCommandAck(commandId, "acknowledged");

    if (!acknowledged)
    {
        Serial.println("Warning: failed to send acknowledged ack for valve_off.");
    }

    bool executed = sendCommandAck(commandId, "executed");

    if (!executed)
    {
        Serial.println("Warning: failed to send executed ack for valve_off.");
    }

    activeCommandId = 0;
    wateringStartedAt = 0;
    wateringDurationMs = 0;
}

void updateWateringState()
{
    if (!wateringActive)
    {
        return;
    }

    unsigned long now = millis();

    if (now - wateringStartedAt >= wateringDurationMs)
    {
        Serial.println();
        Serial.println("Watering duration completed.");

        closeFakeValve();

        if (activeCommandId > 0)
        {
            bool executed = sendCommandAck(activeCommandId, "executed");

            if (!executed)
            {
                Serial.println("Warning: failed to send executed ack.");
            }
        }

        activeCommandId = 0;
        wateringStartedAt = 0;
        wateringDurationMs = 0;
    }
}