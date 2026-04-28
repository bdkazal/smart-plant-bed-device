#pragma once

#include <Arduino.h>

String getFirmwareVersion();
String getDeviceType();
String getHardwareModel();

void printFirmwareInfo();