#pragma once

#include <Arduino.h>

// Smart Plant Bed ESP32 pin map.
// Keep all pin choices centralized here so hardware wiring changes do not
// spread across the firmware.

// GPIO26 is a generally safe output pin on most ESP32 dev boards.
// For now we use it for LED/GPIO valve simulation.
static const int VALVE_CONTROL_PIN = 26;

// Most relay modules are either active HIGH or active LOW.
// For LED/GPIO simulation, active HIGH is easiest:
//   HIGH = valve/LED ON
//   LOW  = valve/LED OFF
static const bool VALVE_ACTIVE_LOW = false;
