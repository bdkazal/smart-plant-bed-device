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

// Calibration from your capacitive soil moisture sensor test:
//
// Without Resistor:
// Air raw: around 3076
// Dry soil raw: around 1832–2041
// Wet soil raw: around 1238–1270
//
// Calibration after adding 100k pulldown resistor:
//
// Disconnected sensor raw: 0
// Air raw: around 2100–2222
// Dry soil raw: around 1382–1535
// Wet soil raw: around 995–1021
//
// We calibrate using soil values, not air value.
static const int SOIL_DRY_RAW = 1550;
static const int SOIL_WET_RAW = 1000;

// If raw is below this threshold, treat soil sensor as disconnected
// and send soil_moisture:null to Laravel.
static const int SOIL_DISCONNECTED_RAW_MAX = 800;

// GPIO25 is used for local manual watering button.
// Wiring:
//   GPIO25 ---- button ---- GND
//
// Internal pull-up is used:
//   not pressed = HIGH
//   pressed     = LOW
static const int MANUAL_WATER_BUTTON_PIN = 25;

// GPIO32 is used for DHT11 temperature/humidity sensor data.
static const int DHT_SENSOR_PIN = 32;