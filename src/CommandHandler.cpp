#include "CommandHandler.h"

#include <ArduinoJson.h>

#include "ApiClient.h"
#include "ValveController.h"

void parseCommandResponse(const String &response)
{
    JsonDocument doc;

    DeserializationError error = deserializeJson(doc, response);

    if (error)
    {
        Serial.print("Failed to parse command JSON: ");
        Serial.println(error.c_str());
        return;
    }

    if (doc["command"].isNull())
    {
        Serial.println("No pending command.");
        return;
    }

    JsonObject command = doc["command"];

    int commandId = command["id"] | 0;
    String commandType = command["command_type"] | "";
    String status = command["status"] | "";

    Serial.println();
    Serial.println("Pending command found:");
    Serial.print("Command ID: ");
    Serial.println(commandId);
    Serial.print("Command type: ");
    Serial.println(commandType);
    Serial.print("Status: ");
    Serial.println(status);

    if (commandType == "valve_on")
    {
        int durationSeconds = command["payload"]["duration_seconds"] | 0;

        Serial.print("Duration seconds: ");
        Serial.println(durationSeconds);

        startWateringCommand(commandId, durationSeconds);
    }
    else if (commandType == "valve_off")
    {
        stopWateringCommand(commandId);
    }
    else
    {
        Serial.println("Unknown command type. Sending failed ack.");
        sendCommandAck(commandId, "failed", "Unknown command type");
    }
}