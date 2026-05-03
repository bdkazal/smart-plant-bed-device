# Biztola Plant Buddy Device Firmware

ESP32 firmware for **Biztola Plant Buddy**, a smart plant-bed / irrigation controller connected to a Laravel IoT backend.

The firmware is currently built and tested on a classic ESP32 DevKit-style board using:

- ESP32-D0WD-V3 development board
- VS Code
- PlatformIO
- Arduino framework

The device connects to Laravel, sends heartbeat/state/readings, polls commands, controls a valve/pump output, reads soil moisture, reads DHT11 temperature/humidity, displays local status on an SSD1306 OLED, and supports offline fallback watering.

> Current pin map is for the ESP32 DevKit prototype. ESP32-C3 Zero / Super Mini will need a separate pin remap because several current pins are not available the same way on ESP32-C3.

---

## Product Name

```text
Brand:   Biztola
Device:  Plant Buddy
Full:    Biztola Plant Buddy
```

Recommended public description:

```text
Biztola Plant Buddy — Smart Plant Bed Controller
```

---

## Current V1 Features

### Connectivity

- Wi-Fi connection
- Stored Wi-Fi credentials using ESP32 flash/NVS
- Wi-Fi setup hotspot portal
- Wi-Fi reset using BOOT button
- Laravel device identity using UUID + `X-DEVICE-KEY`
- Laravel config fetch
- Periodic config refresh
- Cached config stored in flash only when config changes
- Server reachability tracking based on recent successful API requests

### Backend communication

- Heartbeat upload
- Device state sync
- Command polling
- Command acknowledgement
- Sensor reading upload
- Laravel manual watering
- Laravel auto watering
- Laravel schedule command support

### Watering control

- `valve_on` command handling
- `valve_off` command handling
- Timed watering
- Valve/pump output GPIO driver
- Local physical manual watering button
- Physical button can stop local, dashboard, auto, or scheduled watering
- Watering status LED: OFF when idle, ON when watering

### Sensors

- Capacitive soil moisture sensor reading
- Soil moisture disconnected detection using 100k pulldown behavior
- Sends `soil_moisture: null` when sensor is unavailable
- DHT11 temperature/humidity reading
- DHT11 is display/reporting only; watering logic does not depend on it
- Soil sensor is optional; missing sensor is shown as `N/A`, not treated as a permanent OLED warning

### Offline behavior

- Cached Laravel config is loaded on boot
- Offline local auto fallback
- Offline local schedule fallback
- NTP time sync when online
- Local schedule fallback uses synced time
- Offline local watering skips noisy state sync when Laravel is not recently reachable

### OLED display

- 0.96 inch SSD1306 128x64 I2C OLED support
- Biztola monochrome 64x64 boot logo splash for 2.5 seconds
- Boot status screen
- Main status screen
- Watering active screen that stays awake during watering
- Watering done screen
- Critical dry-soil screen that stays awake
- Display wake button wakes OLED for 30 seconds
- Simple 1px square border around normal status screens

### Status LEDs

- Wi-Fi status LED
  - solid ON when connected
  - blinking when disconnected/setup mode
- Watering status LED
  - OFF when idle
  - ON when watering

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

Current ESP32 DevKit prototype pin map:

| Purpose                               | ESP32 GPIO   | Notes                                                     |
| ------------------------------------- | -----------: | --------------------------------------------------------- |
| Valve / relay / MOSFET output         | GPIO26       | Active HIGH by default                                    |
| Wi-Fi status LED                      | GPIO27       | Solid ON when connected, blinking when disconnected/setup |
| Watering status LED                   | GPIO14       | OFF when idle, ON when watering                           |
| Manual watering button                | GPIO25       | Button connects GPIO25 to GND, uses `INPUT_PULLUP`        |
| OLED wake/status button               | GPIO33       | Button connects GPIO33 to GND, uses `INPUT_PULLUP`        |
| Capacitive soil moisture analog input | GPIO34       | ADC1 input-only pin                                       |
| DHT11 data                            | GPIO32       | Temperature/humidity display/reading                      |
| OLED SDA                              | GPIO21       | I2C SDA                                                   |
| OLED SCL                              | GPIO22       | I2C SCL                                                   |
| Wi-Fi reset button                    | GPIO0 / BOOT | Hold during startup to clear saved Wi-Fi                  |

---

## Current Wiring

### Valve / Pump Output Test LED

For safe testing, GPIO26 can control an LED instead of a real valve/pump.

```text
GPIO26 ---- 330Ω resistor ---- LED long leg/anode
LED short leg/cathode -------- GND
```

When watering starts, the LED turns ON. When watering stops or completes, the LED turns OFF.

### Future Real Pump / Solenoid Output

Do not connect a pump or solenoid directly to ESP32 GPIO.

Use a MOSFET/relay driver:

```text
ESP32 GPIO26 -> MOSFET gate/input driver
12V supply   -> pump/solenoid positive
pump/solenoid negative -> MOSFET drain/output
MOSFET source -> GND
ESP32 GND and 12V GND common
flyback diode across inductive load
```

Planned prototype direction:

```text
12V pump or 12V normally-closed solenoid
AO3400 MOSFET controller for small V1 load
LM2596 buck converter for ESP32/sensors/OLED logic power
12V 3A adapter recommended over 12V 2A for startup headroom
```

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

### Watering Status LED

```text
GPIO14 ---- 220Ω to 1kΩ resistor ---- LED long leg/anode
LED short leg/cathode ---------------- GND
```

Behavior:

```text
Idle      = OFF
Watering  = ON
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
offline fallback watering
```

### OLED Wake / Status Button

```text
GPIO33 ---- button ---- GND
```

The firmware uses the ESP32 internal pull-up resistor:

```text
not pressed = HIGH
pressed     = LOW
```

Behavior:

```text
Press button:
  OLED wakes
  shows current status
  stays on for 30 seconds
  sleeps again
```

### Capacitive Soil Moisture Sensor

```text
Sensor VCC  -> ESP32 3V3
Sensor GND  -> ESP32 GND
Sensor AOUT -> ESP32 GPIO34
```

Do not send 5V analog output into the ESP32 ADC pin.

Recommended optional pulldown for unplug detection:

```text
GPIO34 / sensor AOUT -> 100kΩ resistor -> GND
```

This helps disconnected sensor read near `0`, so firmware can send:

```json
{
  "soil_moisture": null
}
```

### DHT11 Sensor

```text
DHT11 VCC  -> ESP32 3V3
DHT11 GND  -> ESP32 GND
DHT11 DATA -> ESP32 GPIO32
```

DHT11 is used for display/readings only. No watering decision should depend on it in V1.

### OLED Display

0.96 inch SSD1306 I2C OLED:

```text
OLED VCC -> ESP32 3V3
OLED GND -> ESP32 GND
OLED SDA -> ESP32 GPIO21
OLED SCL -> ESP32 GPIO22
```

Current configured OLED address:

```cpp
static const int OLED_I2C_ADDRESS = 0x3C;
```

If your module scans as `0x3D`, update `include/PinConfig.h`.

---

## Soil Moisture Calibration

Current tested values after adding the 100k pulldown resistor:

```text
Disconnected sensor raw: 0
Air raw: around 2100–2222
Dry soil raw: around 1382–1535
Wet soil raw: around 995–1021
```

Current calibration:

```cpp
static const int SOIL_DRY_RAW = 1550;
static const int SOIL_WET_RAW = 1000;
static const int SOIL_DISCONNECTED_RAW_MAX = 800;
static const int SOIL_CRITICAL_PERCENT = 15;
```

The firmware maps:

```text
dry soil raw -> 0%
wet soil raw -> 100%
```

Important design note:

```text
Sensor missing/unplugged -> soil_moisture:null
Very dry soil            -> low percentage, not null
```

This distinction matters because the soil moisture sensor is optional, but when installed it must still detect very dry soil.

---

## OLED Display Screens

### Boot logo

On power-up, the display shows the Biztola logo for 2.5 seconds:

```text
64x64 monochrome bitmap
centered on 128x64 screen
no border on splash screen
```

### Main status screen

Normal online:

```text
      Plant Bed
SCHEDULE          IDLE
SOIL 42%            OK
T 29C            H 61%
```

Server offline / local fallback:

```text
    Offline Mode
SCHEDULE          IDLE
SOIL 42%            OK
T 29C            H 61%
```

Wi-Fi offline:

```text
    WIFI OFFLINE
SCHEDULE          IDLE
SOIL 42%            OK
T 29C            H 61%
```

### Watering screen

```text
      WATERING
SCHEDULE      Watering
SOIL 18%           LOW
TIME            60 sec
```

The watering screen stays awake while watering is active.

### Watering done screen

```text
   Watering DONE
VALVE          CLOSED
SOIL 24%           LOW
   Returning idle
```

Then the OLED sleeps after the configured visible time.

### Critical dry-soil screen

```text
    * DRY SOIL *
SOIL 12%           DRY
LIMIT              15%
SCHEDULE          IDLE
```

This screen stays awake until moisture rises above the critical level.

### Optional soil sensor behavior

If soil sensor is missing/unplugged, the normal screen shows:

```text
SOIL --            N/A
```

It does not keep the OLED awake as a warning because soil sensor support is optional in the product design.

---

## Normal Boot Flow

On startup, the ESP32 does:

```text
1. Start Serial Monitor
2. Start device storage
3. Initialize valve driver
4. Initialize status LEDs
5. Initialize manual watering button
6. Initialize display wake button
7. Initialize sensor reader
8. Initialize time sync
9. Initialize local automation
10. Initialize OLED display
11. Show Biztola boot logo for 2.5 seconds
12. Show boot status
13. Print device identity
14. Print firmware info
15. Check BOOT button for Wi-Fi reset
16. Load stored Wi-Fi credentials
17. Load cached Laravel config if available
18. If Wi-Fi exists:
    - connect to Wi-Fi
    - set Wi-Fi LED solid ON
    - fetch Laravel config
    - sync NTP time
    - send heartbeat
    - sync current state
    - poll commands
    - upload sensor reading
    - show current OLED status
19. If Wi-Fi does not exist:
    - start setup hotspot
    - blink Wi-Fi LED
    - wait for user Wi-Fi setup
```

Safe default on boot:

```text
valve closed
watering idle
watering status LED off
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
SCHEDULE_CHECK_INTERVAL_MS = 5000

OFFLINE_HEARTBEAT_INTERVAL_MS = 30000
OFFLINE_COMMAND_POLL_INTERVAL_MS = 30000
OFFLINE_CONFIG_REFRESH_INTERVAL_MS = 120000
```

Meaning:

```text
Heartbeat every 15 seconds while Laravel recently reachable
Command polling every 5 seconds while Laravel recently reachable
Sensor reading every 30 seconds
Config refresh every 60 seconds while Laravel recently reachable
Local schedule check every 5 seconds
Offline API retry intervals are slower to avoid noisy failed requests
```

Config refresh updates RAM values and saves cached config only when the received config actually changed.

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
2. Turns watering status LED ON
3. Shows OLED watering screen
4. Syncs state as watering/open
5. Sends acknowledged ack
6. Waits for duration
7. Closes valve output
8. Turns watering status LED OFF
9. Shows OLED watering done screen
10. Sends executed ack
11. Syncs final idle/closed state
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
2. Turns watering status LED OFF
3. Shows OLED watering done screen
4. If a valve_on command was active, marks it executed
5. Acknowledges valve_off
6. Marks valve_off executed
7. Syncs final idle/closed state
```

### Physical Button Stop

If watering was started by Laravel and the physical button is pressed:

```text
1. ESP32 closes valve output
2. Turns watering status LED OFF
3. Sends executed ack for the active Laravel command
4. Syncs final idle/closed state
```

If watering was started locally by the button:

```text
1. ESP32 closes valve output
2. Turns watering status LED OFF
3. Syncs idle/closed state if Laravel is recently reachable
4. No Laravel command ack is sent because there is no Laravel command ID
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

If soil sensor is missing:

```json
{
  "soil_moisture": null
}
```

Laravel decides online auto watering based on:

```text
watering_mode = auto
soil_moisture is not null
soil_moisture <= soil_moisture_threshold
no active watering command exists
cooldown has passed
```

When Laravel is not reachable, the ESP32 can use cached config for local fallback automation.

---

## Offline Auto / Schedule Fallback

The device loads cached Laravel config from flash on boot and can continue limited operation when Laravel is unavailable.

### Offline auto fallback

Runs locally when:

```text
Laravel is not recently reachable
watering_mode = auto
soil sensor is available
soil moisture <= threshold
cooldown allows watering
valve is not already watering
```

### Offline schedule fallback

Runs locally when:

```text
Laravel is not recently reachable
watering_mode = schedule
NTP/system time is ready
cached schedules contain an enabled schedule for current day/time
valve is not already watering
```

The schedule fallback prevents repeated triggering of the same schedule within the same day/time key.

---

## Current Firmware File Structure

```text
include/
  ApiClient.h
  AppConfig.h
  BootLogo.h
  CommandHandler.h
  DeviceIdentity.h
  DeviceStorage.h
  DisplayButton.h
  DisplayManager.h
  FirmwareInfo.h
  LocalAutomation.h
  ManualButton.h
  PinConfig.h
  SensorReader.h
  SetupPortal.h
  StatusLed.h
  TimeSync.h
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
  DisplayButton.cpp
  DisplayManager.cpp
  FirmwareInfo.cpp
  LocalAutomation.cpp
  ManualButton.cpp
  SensorReader.cpp
  SetupPortal.cpp
  StatusLed.cpp
  TimeSync.cpp
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

Pull latest code:

```bash
git pull
```

---

## Safe Hardware Testing Order

Before connecting a real pump/solenoid valve:

```text
1. ESP32 only
2. External LED on GPIO26
3. Watering status LED on GPIO14
4. MOSFET/relay module with no water load
5. MOSFET/relay switching test
6. Low-voltage dummy load
7. Real pump/valve with proper external power
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

Recommended V1 hardware direction:

```text
ESP32 DevKit for current prototype stability
ESP32-C3 Zero / Super Mini later after pin remap
LM2596 buck converter for ESP32/sensors/OLED
12V 3A adapter preferred
AO3400 MOSFET controller acceptable for small/short-duty V1 loads
DHT11 acceptable for display-only temp/humidity
capacitive soil moisture sensor with 100k pulldown
0.96 inch SSD1306 I2C OLED
DS1307/DS3231 RTC optional later for better offline-after-power-loss timekeeping
```

---

## Review Notes Before V1 Hardware Prototype

Current firmware architecture is clean enough for V1:

- pin values are centralized in `PinConfig.h`
- backend communication is isolated in `ApiClient.cpp`
- sensor logic is isolated in `SensorReader.cpp`
- watering runtime logic is isolated in `ValveController.cpp`
- offline fallback logic is isolated in `LocalAutomation.cpp`
- OLED behavior is isolated in `DisplayManager.cpp`
- Wi-Fi credentials and cached config are stored using `Preferences`

Important remaining risks before real hardware:

1. ESP32-C3 migration needs a dedicated pin profile.
2. Real pump/solenoid switching needs proper MOSFET, flyback diode, common ground, and power headroom.
3. Offline schedule after complete power loss still depends on time becoming available again; RTC support would improve this.
4. DHT11 is acceptable for display but should not control watering.
5. Sensor calibration should be re-tested after final wiring/enclosure because analog readings can shift.

---

## V1 Status

Working:

```text
Wi-Fi setup portal
Wi-Fi reset
Laravel config fetch
Cached Laravel config
Periodic config refresh
Heartbeat
State sync
Command polling
Manual dashboard watering
Laravel auto watering from real soil moisture reading
Offline local auto fallback
Offline local schedule fallback
Physical button watering
Physical button stop
Valve LED/output
Wi-Fi status LED
Watering status LED
Display wake button
OLED boot logo
OLED status/watering/done/critical screens
Capacitive soil moisture sensor
Soil sensor disconnected/null behavior
DHT11 temperature/humidity display/readings
NTP time sync
```

Next planned:

```text
ESP32-C3 Zero / Super Mini pin profile
real MOSFET/pump/solenoid hardware test
optional RTC support for offline time after power loss
production enclosure/wiring cleanup
OTA firmware update
production factory provisioning
```
