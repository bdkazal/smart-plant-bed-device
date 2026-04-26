#pragma once

#include <Arduino.h>

void fetchConfig();
void sendHeartbeat();
bool sendCommandAck(int commandId, const String &status, const String &message = "");
void pollCommands();

// Temporary bridge.
// This function still lives in main.cpp for now.
// Later we will move it into CommandHandler.
void parseCommandResponse(const String &response);