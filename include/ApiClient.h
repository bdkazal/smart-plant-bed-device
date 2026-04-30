#pragma once

#include <Arduino.h>

#include "SensorReader.h"

void fetchConfig();
void sendHeartbeat();

bool sendCommandAck(int commandId, const String &status, const String &message = "");

void pollCommands();

bool sendDeviceStateSync(int lastCompletedCommandId = 0);

bool sendSensorReading(const SensorReading &reading);

bool isServerRecentlyReachable();
unsigned long getLastServerSuccessAt();