#include "ValveController.h"

#include <Arduino.h>

#include "ApiClient.h"
#include "ValveDriver.h"

// Valve / watering runtime state.
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
    setValveDriverOpen(true);

    Serial.println();
    Serial.println("VALVE: OPEN");
    Serial.println("Watering state: watering");
}

void closeFakeValve()
{
    valveOpen = false;
    wateringActive = false;
    setValveDriverOpen(false);

    Serial.println();
    Serial.println("VALVE: CLOSED");
    Serial.println("Watering state: idle");
}

void startWateringCommand(int commandId, int durationSeconds)
{
    if (wateringActive)
    {
        Serial.println("Already watering. Ignoring new valve_on command.");
        sendCommandAck(commandId, "failed", "Device is already watering");
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

    // Report the new runtime state once after the valve state changes.
    sendDeviceStateSync(0);

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

    // Report the final runtime state once after command handling finishes.
    sendDeviceStateSync(commandId);

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

    if (now - wateringStartedAt < wateringDurationMs)
    {
        return;
    }

    Serial.println();
    Serial.println("Watering duration completed.");

    int completedCommandId = activeCommandId;

    closeFakeValve();

    if (completedCommandId > 0)
    {
        bool executed = sendCommandAck(completedCommandId, "executed");

        if (!executed)
        {
            Serial.println("Warning: failed to send executed ack. Laravel timeout may mark this command failed.");
        }

        // Report the final runtime state once after the command finishes.
        sendDeviceStateSync(completedCommandId);
    }

    activeCommandId = 0;
    wateringStartedAt = 0;
    wateringDurationMs = 0;
}
