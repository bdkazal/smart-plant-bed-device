#pragma once

#include <Arduino.h>

String getApiBaseUrl();
String getDeviceUuid();
String getDeviceApiKey();

void printDeviceIdentity();