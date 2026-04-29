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

// GPIO27 is used for Wi-Fi status LED.
// Connected = solid ON.
// Not connected / setup mode = blinking.
static const int WIFI_STATUS_LED_PIN = 27;
static const bool WIFI_STATUS_LED_ACTIVE_LOW = false;

// GPIO34 is input-only and belongs to ADC1.
// ADC1 pins are preferred for analog sensors while Wi-Fi is active.
static const int SOIL_MOISTURE_PIN = 34;

// Temporary calibration values.
// You MUST adjust these after checking your real sensor readings.
//
// Usually:
//   dry soil / air  = higher raw value
//   wet soil / water = lower raw value
//
// These defaults are only a starting point.
static const int SOIL_DRY_RAW = 3200;
static const int SOIL_WET_RAW = 1300;