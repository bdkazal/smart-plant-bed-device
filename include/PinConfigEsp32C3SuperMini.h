#pragma once

#include <Arduino.h>

// Smart Plant Bed ESP32-C3 Super Mini pin map.
// V1 migration profile for the small ESP32-C3 Super Mini board.

// Valve / LR7843 MOSFET input.
// This same physical GPIO also drives the watering indicator LED through 330R.
// Wiring:
//   GPIO5 -> LR7843 PWM/input
//   GPIO5 -> 330R resistor -> LED anode
//   LED cathode -> GND
//
// HIGH = valve ON + LED ON
// LOW  = valve OFF + LED OFF
static const int VALVE_CONTROL_PIN = 5;
static const bool VALVE_ACTIVE_LOW = false;

// Wi-Fi status LED.
// Wiring:
//   GPIO6 -> 330R resistor -> LED anode
//   LED cathode -> GND
static const int WIFI_STATUS_LED_PIN = 6;
static const bool WIFI_STATUS_LED_ACTIVE_LOW = false;

// ESP32-C3 Super Mini V1 does not use a separate watering status LED pin.
// The watering indicator LED physically mirrors VALVE_CONTROL_PIN.
// -1 means disabled / not connected.
static const int WATERING_STATUS_LED_PIN = -1;
static const bool WATERING_STATUS_LED_ACTIVE_LOW = false;

// External Wi-Fi reset button for C3.
// Do not use the onboard BOOT/GPIO9 button because GPIO9 is used as I2C SCL.
// Wiring:
//   GPIO7 ---- button ---- GND
//
// Internal pull-up is used:
//   not pressed = HIGH
//   pressed     = LOW
static const int WIFI_RESET_BUTTON_PIN = 7;

// Soil moisture analog input.
// Recalibrate after migration because ESP32-C3 ADC readings can differ
// from the classic ESP32 DevKit readings.
static const int SOIL_MOISTURE_PIN = 1;

// Keep the current calibration first, then re-test and update after C3 migration.
static const int SOIL_DRY_RAW = 1550;
static const int SOIL_WET_RAW = 1000;
static const int SOIL_DISCONNECTED_RAW_MAX = 800;
static const int SOIL_CRITICAL_PERCENT = 15;

// Buttons use internal pull-up:
//   not pressed = HIGH
//   pressed     = LOW
static const int MANUAL_WATER_BUTTON_PIN = 3;
static const int DISPLAY_WAKE_BUTTON_PIN = 4;

// DHT11 temperature/humidity sensor data.
static const int DHT_SENSOR_PIN = 2;

// I2C bus for SSD1306 OLED + RTC.
// The tested C3 Super Mini pinout labels:
//   GPIO8 = SDA
//   GPIO9 = SCL
//
// GPIO9 can be boot-related on many ESP32-C3 boards, so test boot/reboot
// carefully after connecting OLED and RTC. Do not use GPIO9 for Wi-Fi reset.
static const int OLED_I2C_SDA_PIN = 8;
static const int OLED_I2C_SCL_PIN = 9;
static const int OLED_I2C_ADDRESS = 0x3C;
static const int OLED_SCREEN_WIDTH = 128;
static const int OLED_SCREEN_HEIGHT = 64;
static const int OLED_RESET_PIN = -1;

// Display behavior.
static const unsigned long OLED_BOOT_SHOW_MS = 12000;
static const unsigned long OLED_STATUS_SHOW_MS = 10000;
static const unsigned long OLED_WAKE_BUTTON_SHOW_MS = 30000;
static const unsigned long OLED_WATERING_SHOW_MS = 10000;
