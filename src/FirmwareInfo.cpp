#include "FirmwareInfo.h"

#include <Arduino.h>

static const char FIRMWARE_VERSION[] = "v0.1.0";
static const char DEVICE_TYPE[] = "plant_bed_controller";
static const char HARDWARE_MODEL[] = "esp32-d0wd-v3-devkit";

String getFirmwareVersion()
{
    return String(FIRMWARE_VERSION);
}

String getDeviceType()
{
    return String(DEVICE_TYPE);
}

String getHardwareModel()
{
    return String(HARDWARE_MODEL);
}

void printFirmwareInfo()
{
    Serial.println();
    Serial.println("Firmware info loaded.");
    Serial.print("Firmware version: ");
    Serial.println(getFirmwareVersion());
    Serial.print("Device type: ");
    Serial.println(getDeviceType());
    Serial.print("Hardware model: ");
    Serial.println(getHardwareModel());
}