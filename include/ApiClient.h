#pragma once

#include <Arduino.h>

void fetchConfig();
void sendHeartbeat();

bool sendCommandAck(int commandId, const String &status, const String &message = "");

void pollCommands();

bool sendDeviceStateSync(int lastCompletedCommandId = 0);