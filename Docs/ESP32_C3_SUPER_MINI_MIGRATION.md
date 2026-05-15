# ESP32-C3 Super Mini Migration Guide

This document plans the migration of the Biztola Plant Buddy firmware from the current classic ESP32 DevKit prototype to an ESP32-C3 Super Mini board.

The goal is to keep the existing ESP32 DevKit firmware working while adding a separate ESP32-C3 Super Mini board profile.

Do **not** replace the working DevKit pin map directly.

---

## Current board

Current working prototype:

```text
Board: ESP32 DevKit / ESP32-D0WD-V3 style board
Framework: Arduino
Build system: PlatformIO
```

Current firmware pin values are centralized in:

```text
include/PinConfig.h
```

This is good because the ESP32-C3 migration should mostly be a pin-profile and PlatformIO environment change, not a full firmware rewrite.

---

## Why migration needs a separate pin profile

The current ESP32 DevKit pin map uses pins that do not exist on the ESP32-C3 Super Mini or are not exposed in the same way.

Current DevKit pin map:

| Function | Current ESP32 DevKit pin | ESP32-C3 Super Mini issue |
| --- | ---: | --- |
| Valve / MOSFET output | GPIO26 | Not available on C3 Super Mini |
| Wi-Fi status LED | GPIO27 | Not available on C3 Super Mini |
| Watering status LED | GPIO14 | Different C3 pin plan; separate LED not needed |
| Soil moisture ADC | GPIO34 | Not available on C3 Super Mini |
| Manual watering button | GPIO25 | Not available on C3 Super Mini |
| OLED wake/status button | GPIO33 | Not available on C3 Super Mini |
| DHT11 data | GPIO32 | Not available on C3 Super Mini |
| I2C SDA | GPIO21 | C3 board has dedicated exposed SDA label on GPIO8 |
| I2C SCL | GPIO22 | C3 board has dedicated exposed SCL label on GPIO9 |

Therefore, the C3 version needs its own pin map.

---

## ESP32-C3 Super Mini exposed pins

The tested board exposes these useful GPIO pins:

```text
GPIO0
GPIO1
GPIO2
GPIO3
GPIO4
GPIO5
GPIO6
GPIO7
GPIO8
GPIO9
GPIO10
GPIO20
GPIO21
```

Observed board pinout notes:

```text
GPIO0-GPIO4 are ADC capable on the shown board pinout.
GPIO8 is labeled SDA.
GPIO9 is labeled SCL.
GPIO20 is labeled RX.
GPIO21 is labeled TX.
```

Keep GPIO20/GPIO21 free for serial/debug unless there is a strong reason to use them.

Reserve GPIO0 for now because small ESP boards often use it for boot/flash behavior.

---

## Recommended ESP32-C3 Super Mini V1 pin map

| Function | ESP32-C3 Super Mini pin | Notes |
| --- | ---: | --- |
| Valve / LR7843 MOSFET input | GPIO5 | Active HIGH. Also drives watering indicator LED physically. |
| Watering indicator LED | GPIO5 | Same physical pin as valve output, through 330Ω resistor. No separate firmware control. |
| Wi-Fi status LED | GPIO6 | Through 330Ω resistor. |
| Soil moisture ADC | GPIO1 | ADC input. Recalibrate after migration. |
| DHT11 data | GPIO2 | Digital input/data pin. |
| Manual watering button | GPIO3 | Button to GND. Use `INPUT_PULLUP`. |
| OLED wake/status button | GPIO4 | Button to GND. Use `INPUT_PULLUP`. |
| I2C SDA | GPIO8 | OLED + RTC shared I2C bus. |
| I2C SCL | GPIO9 | OLED + RTC shared I2C bus. Test boot carefully. |
| Spare | GPIO7 | Future use. |
| Spare | GPIO10 | Future use. |
| UART RX | GPIO20 | Keep free for debug/serial. |
| UART TX | GPIO21 | Keep free for debug/serial. |
| Reserved | GPIO0 | Avoid for V1 unless needed. |

---

## Valve and watering LED design

The ESP32-C3 version should not use a separate watering status LED GPIO.

Use one shared physical pin:

```text
GPIO5 -> LR7843 PWM/input
GPIO5 -> 330Ω resistor -> LED anode
LED cathode -> GND
```

Behavior:

```text
GPIO5 HIGH = valve ON + watering LED ON
GPIO5 LOW  = valve OFF + watering LED OFF
```

Firmware should not separately control a watering LED on C3.

Recommended C3 firmware value:

```cpp
static const int WATERING_STATUS_LED_PIN = -1;
```

Meaning:

```text
No separate watering LED pin exists.
The indicator LED physically mirrors the valve output.
```

### LED resistor rule

For ESP32/ESP32-C3 visible indicator LEDs on 3.3V GPIO, use:

```text
330Ω default
220Ω only if more brightness is required
```

Do not use 1kΩ for the Plant Buddy operation/status LED design.

---

## Hardware wiring map

### Valve / LR7843 MOSFET module

```text
ESP32-C3 GPIO5 -> LR7843 PWM/input
ESP32-C3 GND   -> LR7843 signal GND

12V+           -> valve +
valve -        -> LR7843 switched output / load -
12V-           -> LR7843 power GND / supply -
```

Ground rule:

```text
ESP32-C3 GND and 12V supply GND need a common reference.
Valve load current must stay in the valve/MOSFET power path, not through the ESP32 ground wire.
```

Use flyback protection for the solenoid valve unless the module/load wiring already provides suitable protection.

Flyback diode orientation:

```text
Diode cathode/stripe -> valve + / 12V+
Diode anode          -> valve - / MOSFET switched side
```

### Wi-Fi status LED

```text
GPIO6 -> 330Ω resistor -> LED anode
LED cathode -> GND
```

### Soil moisture sensor

```text
Sensor VCC  -> 3V3
Sensor GND  -> GND
Sensor AOUT -> GPIO1
```

Recommended unplug-detection pulldown:

```text
GPIO1 / sensor AOUT -> 100kΩ resistor -> GND
```

Recalibration is required after moving to C3 because ADC readings may shift.

### DHT11

```text
DHT11 VCC  -> 3V3
DHT11 GND  -> GND
DHT11 DATA -> GPIO2
```

DHT11 remains display/reporting only. Watering decisions must not depend on DHT11.

### Buttons

Manual watering button:

```text
GPIO3 -> button -> GND
```

OLED wake/status button:

```text
GPIO4 -> button -> GND
```

Firmware uses internal pull-up:

```text
not pressed = HIGH
pressed     = LOW
```

### OLED + RTC I2C bus

```text
GPIO8 -> SDA
GPIO9 -> SCL
```

OLED:

```text
OLED VCC -> 3V3
OLED GND -> GND
OLED SDA -> GPIO8
OLED SCL -> GPIO9
```

RTC:

```text
RTC SDA -> GPIO8
RTC SCL -> GPIO9
RTC GND -> GND
```

Important DS1307 warning:

```text
Many DS1307 modules require 5V and may pull SDA/SCL up to 5V.
ESP32-C3 GPIO is 3.3V logic and is not 5V tolerant.
Use a 3.3V-safe RTC module or I2C level shifter if the DS1307 module pulls I2C lines to 5V.
```

GPIO9 caution:

```text
GPIO9 can be boot-related on many ESP32-C3 boards.
Because this board labels GPIO8/GPIO9 as SDA/SCL, start with them, but test boot/reboot carefully after connecting OLED and RTC.
```

---

## Firmware profile plan

Create separate pin profiles:

```text
include/PinConfigEsp32Devkit.h
include/PinConfigEsp32C3SuperMini.h
```

Then make `include/PinConfig.h` select the correct profile:

```cpp
#pragma once

#if defined(BOARD_ESP32_C3_SUPER_MINI)
  #include "PinConfigEsp32C3SuperMini.h"
#else
  #include "PinConfigEsp32Devkit.h"
#endif
```

### DevKit profile

Move the current working `include/PinConfig.h` pin values into:

```text
include/PinConfigEsp32Devkit.h
```

Do not change the DevKit pin values during the migration.

### ESP32-C3 Super Mini profile

Recommended first C3 profile:

```cpp
#pragma once

#include <Arduino.h>

// ESP32-C3 Super Mini pin map.
// Valve output also drives physical watering indicator LED through 330Ω.
static const int VALVE_CONTROL_PIN = 5;
static const bool VALVE_ACTIVE_LOW = false;

static const int WIFI_STATUS_LED_PIN = 6;
static const bool WIFI_STATUS_LED_ACTIVE_LOW = false;

// No separate watering LED pin on C3.
// The LED is physically wired to VALVE_CONTROL_PIN.
static const int WATERING_STATUS_LED_PIN = -1;
static const bool WATERING_STATUS_LED_ACTIVE_LOW = false;

static const int SOIL_MOISTURE_PIN = 1;

static const int MANUAL_WATER_BUTTON_PIN = 3;
static const int DISPLAY_WAKE_BUTTON_PIN = 4;

static const int DHT_SENSOR_PIN = 2;

static const int OLED_I2C_SDA_PIN = 8;
static const int OLED_I2C_SCL_PIN = 9;
static const int OLED_I2C_ADDRESS = 0x3C;
static const int OLED_SCREEN_WIDTH = 128;
static const int OLED_SCREEN_HEIGHT = 64;
static const int OLED_RESET_PIN = -1;

static const unsigned long OLED_BOOT_SHOW_MS = 12000;
static const unsigned long OLED_STATUS_SHOW_MS = 10000;
static const unsigned long OLED_WAKE_BUTTON_SHOW_MS = 30000;
static const unsigned long OLED_WATERING_SHOW_MS = 10000;

// Keep current calibration first, then recalibrate after hardware migration.
static const int SOIL_DRY_RAW = 1550;
static const int SOIL_WET_RAW = 1000;
static const int SOIL_DISCONNECTED_RAW_MAX = 800;
static const int SOIL_CRITICAL_PERCENT = 15;
```

---

## PlatformIO plan

Keep existing DevKit environment.

Add a new ESP32-C3 environment:

```ini
[env:esp32c3_supermini]
platform = espressif32
board = esp32-c3-devkitm-1
framework = arduino

monitor_speed = 115200
upload_speed = 115200

build_flags =
    -D BOARD_ESP32_C3_SUPER_MINI

lib_deps =
    bblanchon/ArduinoJson
    adafruit/DHT sensor library
    adafruit/Adafruit Unified Sensor
    adafruit/Adafruit GFX Library
    adafruit/Adafruit SSD1306
    adafruit/RTClib
```

Build DevKit:

```bash
pio run -e esp32dev
```

Build C3:

```bash
pio run -e esp32c3_supermini
```

Upload C3:

```bash
pio run -e esp32c3_supermini --target upload
```

Monitor:

```bash
pio device monitor
```

If upload fails, use the board boot mode:

```text
Hold BOOT
Tap/reset or plug USB
Start upload
Release BOOT after upload begins
```

Exact behavior can vary by ESP32-C3 Super Mini board revision.

---

## Required firmware safety update

Any class that controls a status LED must support disabled pins.

If a pin value is `-1`, the class should do nothing.

Example pattern:

```cpp
void StatusLed::begin()
{
    if (_pin < 0) {
        return;
    }

    pinMode(_pin, OUTPUT);
    off();
}

void StatusLed::on()
{
    if (_pin < 0) {
        return;
    }

    digitalWrite(_pin, _activeLow ? LOW : HIGH);
}

void StatusLed::off()
{
    if (_pin < 0) {
        return;
    }

    digitalWrite(_pin, _activeLow ? HIGH : LOW);
}
```

This is required because the ESP32-C3 version does not use a separate watering status LED GPIO.

---

## Migration test order

Do not connect every component at once.

Use this test order:

```text
1. Flash ESP32-C3 Super Mini with no external hardware connected.
2. Confirm Serial Monitor works.
3. Confirm boot is reliable after several resets.
4. Confirm Wi-Fi connects.
5. Test GPIO5 with only an LED + 330Ω resistor.
6. Connect GPIO5 to LR7843 input, no valve connected yet.
7. Test LR7843 switching with multimeter or dummy load.
8. Connect valve using external 12V supply and common ground.
9. Test manual valve ON/OFF behavior.
10. Connect OLED only on GPIO8/GPIO9.
11. Reboot several times and confirm I2C does not disturb boot.
12. Add RTC to the same I2C bus.
13. Confirm time sync / RTC behavior.
14. Add soil moisture sensor on GPIO1.
15. Recalibrate soil raw values.
16. Add DHT11 on GPIO2.
17. Add manual button on GPIO3.
18. Add OLED wake/status button on GPIO4.
19. Add Wi-Fi LED on GPIO6.
20. Run full online + offline watering tests.
```

---

## Recalibration checklist

After moving to ESP32-C3, re-test soil moisture values:

```text
Disconnected sensor raw
Air raw
Dry soil raw
Wet soil raw
```

Then update:

```cpp
SOIL_DRY_RAW
SOIL_WET_RAW
SOIL_DISCONNECTED_RAW_MAX
```

Do not assume classic ESP32 ADC values will match ESP32-C3 ADC values.

---

## Acceptance checklist

The ESP32-C3 migration is acceptable only when all items pass:

```text
[ ] DevKit build still compiles.
[ ] C3 build compiles.
[ ] C3 flashes successfully.
[ ] Serial Monitor works.
[ ] Wi-Fi setup portal works.
[ ] Wi-Fi reconnect works.
[ ] GPIO5 valve output works.
[ ] GPIO5 watering LED mirrors valve state through 330Ω.
[ ] LR7843 module switches reliably.
[ ] Real valve opens/closes with external 12V supply.
[ ] Wi-Fi LED works on GPIO6.
[ ] Soil moisture reads on GPIO1.
[ ] Soil sensor disconnected/null behavior works.
[ ] DHT11 reads on GPIO2.
[ ] Manual button works on GPIO3.
[ ] OLED wake/status button works on GPIO4.
[ ] OLED works on GPIO8/GPIO9.
[ ] RTC works on GPIO8/GPIO9 without 5V I2C risk.
[ ] Laravel config fetch works.
[ ] Heartbeat works.
[ ] Reading upload works.
[ ] Command polling works.
[ ] valve_on and valve_off commands work.
[ ] Physical button can stop watering.
[ ] Offline local auto fallback works.
[ ] Offline schedule fallback works.
[ ] OLED screens work.
[ ] Device remains responsive while Laravel is offline.
```

---

## Final recommendation

Start with firmware profile support before rewiring the real device.

Order:

```text
1. Add C3 pin profile and PlatformIO environment.
2. Confirm C3 firmware builds and boots with no hardware.
3. Move hardware one component at a time.
4. Recalibrate soil sensor.
5. Retest online/offline watering behavior.
```

Do not remove the ESP32 DevKit build until the ESP32-C3 version has passed all acceptance tests.
