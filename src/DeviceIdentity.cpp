#include "DeviceIdentity.h"

#include <Arduino.h>

#include "secrets.h"

String getApiBaseUrl()
{
    return String(API_BASE_URL);
}

String getDeviceUuid()
{
    return String(DEVICE_UUID);
}

String getDeviceApiKey()
{
    return String(DEVICE_API_KEY);
}

void printDeviceIdentity()
{
    Serial.println();
    Serial.println("Device identity loaded.");
    Serial.print("API base URL: ");
    Serial.println(getApiBaseUrl());
    Serial.print("Device UUID: ");
    Serial.println(getDeviceUuid());
    Serial.println("Device API key: [hidden]");
}