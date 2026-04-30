# Smart Plant Bed Device Firmware

ESP32 firmware for the Smart Plant Bed Laravel IoT platform.

This firmware is built with:

- ESP32-D0WD-V3 development board
- VS Code
- PlatformIO
- Arduino framework

The device connects to the Laravel backend, sends heartbeat/state/readings, polls commands, controls a valve output, reads soil moisture, and supports local physical-button watering.

---

## Current V1 Features

- Wi-Fi connection
- Stored Wi-Fi credentials using ESP32 flash/NVS
- Wi-Fi setup hotspot portal
- Wi-Fi reset using BOOT button
- Laravel device identity using UUID + `X-DEVICE-KEY`
- Laravel config fetch
- Periodic config refresh
- Heartbeat upload
- Device state sync
- Command polling
- Command acknowledgement
- `valve_on` command handling
- `valve_off` command handling
- Timed watering
- Real capacitive soil moisture sensor reading
- Auto watering through Laravel backend
- Valve output GPIO driver
- Wi-Fi status LED
- Local physical manual watering button
- Physical button can stop local, dashboard, or auto watering

---

## Backend API Contract

The device communicates with Laravel using:

```http
GET  /api/device/config
POST /api/device/readings
GET  /api/device/commands
POST /api/device/commands/{id}/ack
POST /api/device/heartbeat
POST /api/device/state
```

Device authentication uses:

```http
X-DEVICE-KEY: <device_api_key>
```

The device identifies itself using:

```json
{
  "device_uuid": "..."
}
```

---

## Hardware Pin Map

| Purpose                               |   ESP32 GPIO | Notes                                                     |
| ------------------------------------- | -----------: | --------------------------------------------------------- |
| Valve / relay output test LED         |       GPIO26 | Active HIGH by default                                    |
| Wi-Fi status LED                      |       GPIO27 | Solid ON when connected, blinking when disconnected/setup |
| Manual watering button                |       GPIO25 | Button connects GPIO25 to GND, uses `INPUT_PULLUP`        |
| Capacitive soil moisture analog input |       GPIO34 | ADC1 input-only pin                                       |
| Wi-Fi reset button                    | GPIO0 / BOOT | Hold during startup to clear saved Wi-Fi                  |

---

## Current Wiring

### Valve Test LED

For safe testing, GPIO26 currently controls an LED instead of a real valve.

```text
GPIO26 ---- 330Ω resistor ---- LED long leg/anode
LED short leg/cathode -------- GND
```

When watering starts, the LED turns ON.

When watering stops or completes, the LED turns OFF.

### Wi-Fi Status LED

```text
GPIO27 ---- 330Ω resistor ---- LED long leg/anode
LED short leg/cathode -------- GND
```

Behavior:

```text
Wi-Fi connected     = solid ON
Wi-Fi disconnected  = blinking
Setup portal active = blinking
```

### Manual Watering Button

```text
GPIO25 ---- button ---- GND
```

The firmware uses the ESP32 internal pull-up resistor:

```text
not pressed = HIGH
pressed     = LOW
```

Button behavior:

```text
If idle:
  press button -> start local watering

If watering:
  press button -> stop watering
```

The physical button can stop:

```text
local button watering
dashboard manual watering
Laravel auto watering
scheduled watering command
```

### Capacitive Soil Moisture Sensor

```text
Sensor VCC  -> ESP32 3V3
Sensor GND  -> ESP32 GND
Sensor AOUT -> ESP32 GPIO34
```

Do not send 5V analog output into the ESP32 ADC pin.

---

## Soil Moisture Calibration

Current tested values:

```text
Air raw: around 3076
Dry soil raw: around 1832–2041
Wet soil raw: around 1238–1270
```

Current calibration:

```cpp
static const int SOIL_DRY_RAW = 2050;
static const int SOIL_WET_RAW = 1240;
```

The firmware maps:

```text
dry soil raw -> 0%
wet soil raw -> 100%
```

Air may also show `0%`, which is expected because calibration is based on soil values, not air.

---

## Normal Boot Flow

On startup, the ESP32 does:

```text
1. Start Serial Monitor
2. Start device storage
3. Initialize valve driver
4. Initialize status LED
5. Initialize manual button
6. Print device identity
7. Print firmware info
8. Check BOOT button for Wi-Fi reset
9. Load stored Wi-Fi credentials
10. If Wi-Fi exists:
    - connect to Wi-Fi
    - set Wi-Fi LED solid ON
    - fetch Laravel config
    - send heartbeat
    - sync current state
    - poll commands
    - upload sensor reading
11. If Wi-Fi does not exist:
    - start setup hotspot
    - blink Wi-Fi LED
    - wait for user Wi-Fi setup
```

Safe default on boot:

```text
valve closed
watering idle
```

---

## Wi-Fi Setup Portal

If no Wi-Fi credentials are saved, the ESP32 starts a setup hotspot:

```text
SSID: PlantBed-Setup
Password: plantbed123
Setup URL: http://192.168.4.1
```

Setup steps:

```text
1. Connect phone/laptop to PlantBed-Setup
2. Open browser and visit http://192.168.4.1
3. Select home Wi-Fi from the list
4. Enter Wi-Fi password
5. Submit
6. ESP32 tests the credentials
7. If correct:
   - saves Wi-Fi credentials to flash
   - restarts
   - connects to home Wi-Fi
8. If wrong:
   - does not save credentials
   - shows an error on the setup page
```

The setup page only asks for Wi-Fi credentials.

It does not ask for:

```text
device_uuid
device_api_key
Laravel API URL
```

Those are device identity/provisioning values, not customer Wi-Fi setup values.

---

## Wi-Fi Reset

To clear saved Wi-Fi credentials and return to setup mode:

```text
1. Hold the ESP32 BOOT button
2. While holding BOOT, press/release EN or RST
3. Keep holding BOOT for about 3 seconds
4. Release BOOT
```

Expected result:

```text
Wi-Fi reset button pressed. Keep holding...
Wi-Fi reset confirmed.
Stored device config cleared.
Saved Wi-Fi cleared. Restarting...
```

This is a Wi-Fi reset only.

It does not:

```text
unclaim the device in Laravel
erase device UUID
erase device API key
erase ownership
```

---

## Runtime Intervals

Current timing:

```cpp
HEARTBEAT_INTERVAL_MS = 15000
COMMAND_POLL_INTERVAL_MS = 5000
READING_INTERVAL_MS = 30000
CONFIG_REFRESH_INTERVAL_MS = 60000
```

Meaning:

```text
Heartbeat every 15 seconds
Command polling every 5 seconds
Sensor reading every 30 seconds
Config refresh every 60 seconds
```

Config refresh updates RAM values only. It does not write to flash.

---

## Command Behavior

### valve_on

When Laravel sends:

```json
{
  "command_type": "valve_on",
  "payload": {
    "duration_seconds": 60
  }
}
```

ESP32:

```text
1. Opens valve output
2. Syncs state as watering/open
3. Sends acknowledged ack
4. Waits for duration
5. Closes valve output
6. Sends executed ack
7. Syncs final idle/closed state
```

### valve_off

When Laravel sends:

```json
{
  "command_type": "valve_off"
}
```

ESP32:

```text
1. Closes valve output immediately
2. If a valve_on command was active, marks it executed
3. Acknowledges valve_off
4. Marks valve_off executed
5. Syncs final idle/closed state
```

### Physical Button Stop

If watering was started by Laravel and the physical button is pressed:

```text
1. ESP32 closes valve output
2. Sends executed ack for the active Laravel command
3. Syncs final idle/closed state
```

If watering was started locally by the button:

```text
1. ESP32 closes valve output
2. Syncs idle/closed state
3. No Laravel command ack is sent because there is no Laravel command ID
```

---

## Laravel Auto Watering

The ESP32 sends readings to Laravel:

```json
{
  "device_uuid": "...",
  "temperature": 28.4,
  "humidity": 65,
  "soil_moisture": 30
}
```

Laravel decides auto watering based on:

```text
watering_mode = auto
soil_moisture <= soil_moisture_threshold
no active watering command exists
cooldown has passed
```

The ESP32 does not currently make the auto-watering decision locally. Laravel does.

---

## Current Firmware File Structure

```text
include/
  ApiClient.h
  AppConfig.h
  CommandHandler.h
  DeviceIdentity.h
  DeviceStorage.h
  FirmwareInfo.h
  ManualButton.h
  PinConfig.h
  SensorReader.h
  SetupPortal.h
  StatusLed.h
  ValveController.h
  ValveDriver.h
  WifiReset.h
  WiFiMan.h
  secrets.example.h
  secrets.h              # local only, ignored by Git

src/
  ApiClient.cpp
  AppConfig.cpp
  CommandHandler.cpp
  DeviceIdentity.cpp
  DeviceStorage.cpp
  FirmwareInfo.cpp
  ManualButton.cpp
  SensorReader.cpp
  SetupPortal.cpp
  StatusLed.cpp
  ValveController.cpp
  ValveDriver.cpp
  WifiReset.cpp
  WiFiMan.cpp
  main.cpp
```

---

## Important Local File

Create this file locally:

```text
include/secrets.h
```

Use `include/secrets.example.h` as the template.

Example:

```cpp
#pragma once

static const char WIFI_SSID[] = "YOUR_WIFI_NAME";
static const char WIFI_PASSWORD[] = "YOUR_WIFI_PASSWORD";

static const char API_BASE_URL[] = "http://YOUR_LARAVEL_SERVER_IP:8000";

static const char DEVICE_UUID[] = "YOUR_DEVICE_UUID";
static const char DEVICE_API_KEY[] = "YOUR_DEVICE_API_KEY";
```

Do not commit:

```text
include/secrets.h
```

---

## Useful PlatformIO Commands

Build:

```bash
pio run
```

Upload:

```bash
pio run --target upload
```

Serial Monitor:

```bash
pio device monitor
```

Clean build:

```bash
pio run --target clean
pio run
```

---

## Safe Hardware Testing Order

Before connecting a real pump/solenoid valve:

```text
1. ESP32 only
2. External LED on GPIO26
3. Relay module with no load
4. Relay clicking test
5. Low-voltage dummy load
6. Real pump/valve with proper external power
```

Do not power a pump or solenoid directly from an ESP32 GPIO pin.

The ESP32 GPIO should only control:

```text
relay module
MOSFET driver
transistor driver circuit
```

Real water hardware needs:

```text
correct external power supply
common ground where required
flyback diode/protection for inductive loads
safe enclosure
water/electric separation
```

---

## V1 Status

Working:

```text
Wi-Fi setup portal
Wi-Fi reset
Laravel config fetch
Periodic config refresh
Heartbeat
State sync
Command polling
Manual dashboard watering
Auto watering from real soil moisture reading
Physical button watering
Physical button stop
Valve LED output
Wi-Fi status LED
Capacitive soil moisture sensor
```

Not yet implemented:

```text
real relay/pump/solenoid hardware
water level sensor
temperature/humidity real sensor
local offline schedule fallback
OTA firmware update
production factory provisioning
```
