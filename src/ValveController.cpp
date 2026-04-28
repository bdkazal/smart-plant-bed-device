#include "ValveController.h"

#include <Arduino.h>

#include "ApiClient.h"
#include "DeviceStorage.h"

// Fake valve / watering runtime state.
// Later this will control a real GPIO relay/MOSFET.
bool valveOpen = false;
bool wateringActive = false;
int activeCommandId = 0;
int pendingCompletedCommandId = 0;
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

void rememberPendingCompletedCommand(int commandId)
{
    if (commandId <= 0)
    {
        return;
    }

    pendingCompletedCommandId = commandId;

    Serial.println();
    Serial.print("Remembered pending completed command for reconnect sync: ");
    Serial.println(pendingCompletedCommandId);

    savePendingCompletedCommandId(commandId);
}

bool hasPendingCompletedCommand()
{
    return pendingCompletedCommandId > 0;
}

int getPendingCompletedCommandId()
{
    return pendingCompletedCommandId;
}

void clearPendingCompletedCommand()
{
    if (pendingCompletedCommandId > 0)
    {
        Serial.print("Clearing pending completed command from RAM: ");
        Serial.println(pendingCompletedCommandId);
    }

    pendingCompletedCommandId = 0;

    clearPendingCompletedCommandId();
}

void loadPendingCompletedCommandFromStorage()
{
    pendingCompletedCommandId = loadPendingCompletedCommandId();

    if (pendingCompletedCommandId > 0)
    {
        Serial.print("Pending completed command restored into RAM: ");
        Serial.println(pendingCompletedCommandId);
    }
}

bool syncPendingCompletedCommandIfNeeded()
{
    if (!hasPendingCompletedCommand())
    {
        return true;
    }

    Serial.println();
    Serial.print("Syncing pending completed command after reconnect: ");
    Serial.println(pendingCompletedCommandId);

    bool synced = sendDeviceStateSync(pendingCompletedCommandId);

    if (synced)
    {
        clearPendingCompletedCommand();
        return true;
    }

    Serial.println("Pending completed command sync failed. Will retry later.");
    return false;
}

void openFakeValve()
{
    valveOpen = true;
    wateringActive = true;

    Serial.println();
    Serial.println("FAKE VALVE: OPEN");
    Serial.println("Watering state: watering");

    sendDeviceStateSync(0);
}

void closeFakeValve()
{
    valveOpen = false;
    wateringActive = false;

    Serial.println();
    Serial.println("FAKE VALVE: CLOSED");
    Serial.println("Watering state: idle");

    sendDeviceStateSync(0);
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

        if (previousExecuted)
        {
            bool synced = sendDeviceStateSync(interruptedCommandId);

            if (!synced)
            {
                rememberPendingCompletedCommand(interruptedCommandId);
            }
        }
        else
        {
            Serial.println("Warning: failed to mark interrupted valve_on command as executed.");
            rememberPendingCompletedCommand(interruptedCommandId);
        }
    }

    bool acknowledged = sendCommandAck(commandId, "acknowledged");

    if (!acknowledged)
    {
        Serial.println("Warning: failed to send acknowledged ack for valve_off.");
    }

    bool executed = sendCommandAck(commandId, "executed");

    if (executed)
    {
        bool synced = sendDeviceStateSync(commandId);

        if (!synced)
        {
            rememberPendingCompletedCommand(commandId);
        }
    }
    else
    {
        Serial.println("Warning: failed to send executed ack for valve_off.");
        rememberPendingCompletedCommand(commandId);
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

            if (executed)
            {
                bool synced = sendDeviceStateSync(activeCommandId);

                if (!synced)
                {
                    rememberPendingCompletedCommand(activeCommandId);
                }
            }
            else
            {
                Serial.println("Warning: failed to send executed ack.");
                rememberPendingCompletedCommand(activeCommandId);
            }
        }

        activeCommandId = 0;
        wateringStartedAt = 0;
        wateringDurationMs = 0;
    }
}