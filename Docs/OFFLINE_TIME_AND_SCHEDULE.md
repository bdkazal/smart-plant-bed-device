# Offline Time and Schedule Behavior

This document records the confirmed offline schedule behavior for Biztola Plant Buddy firmware.

## Confirmed status

Offline schedule fallback is working after both:

```text
Laravel server is stopped/offline
ESP32 is power-cycled while Laravel remains offline
```

Test result:

```text
The device restored time from DS1307 RTC, loaded cached Laravel config from flash, and triggered the scheduled watering at the expected local time.
```

## Time source priority

Firmware time priority:

```text
1. NTP time when available
2. Laravel server_time_utc from /api/device/config
3. DS1307 RTC UTC backup time
4. No valid time: local schedule fallback disabled
```

The RTC is not the authority when the server/internet is available. It is the backup clock for offline reboot.

## RTC storage rule

The DS1307 stores UTC time, not local wall-clock time.

Reason:

```text
UTC is stable across countries and timezone changes.
Laravel remains the source of truth for the device timezone.
The firmware uses timezone_offset_minutes from Laravel to convert UTC into local display/schedule time.
```

Example for Bangladesh:

```text
RTC UTC:          17:10:06
Local Asia/Dhaka: 23:10:06
```

Expected boot log:

```text
Initializing DS1307 RTC...
DS1307 RTC ready UTC: 2026-5-12 17:10:06
System time loaded from RTC UTC: 2026-5-12 17:10:06
Time source: RTC backup.
RTC restored local time: 2026-05-12 23:10:06
```

## Laravel config time fields

The Laravel config response should include:

```json
{
  "server_time_utc": "2026-05-12T16:46:13+00:00",
  "server_time_local": "2026-05-12 22:46:13",
  "server_time": "2026-05-12 22:46:13",
  "config": {
    "timezone": "Asia/Dhaka",
    "timezone_offset_minutes": 360
  }
}
```

The firmware prefers `server_time_utc`. The local fields are useful for logs/backward compatibility.

## Offline schedule fallback rules

The device can run cached schedules locally when:

```text
Laravel is not recently reachable
watering_mode = schedule
cached config has enabled schedules
device time is ready from NTP, Laravel UTC, or RTC
watering is not already active
```

The schedule match window is one minute:

```text
Schedule 23:10:00 can trigger from 23:10:00 to 23:10:59
```

The firmware prevents repeating the same schedule during the same day/time key.

## Test procedure

### Server-off test

```text
1. Start Laravel.
2. Boot the ESP32 and let it fetch config.
3. Create a schedule 2-3 minutes ahead.
4. Wait until firmware logs the schedule in Parsed device config.
5. Stop Laravel with Ctrl+C.
6. Wait for the scheduled minute.
7. Confirm valve/watering LED turns ON and then OFF after duration.
```

Expected log:

```text
Local fallback schedule watering triggered.
Scheduled time: HH:MM:00
Current time: HH:MM:xx
Duration seconds: ...
```

### Server-off plus power-loss test

```text
1. Keep Laravel stopped.
2. Power-cycle or reset ESP32.
3. Confirm RTC restored local time in Serial Monitor.
4. Wait for a cached schedule time.
5. Confirm watering triggers on time.
```

## OLED behavior

The display button cycles pages:

```text
Press 1: main status page
Press 2: schedule/time page
Press 3: main status page
Press 4: schedule/time page
```

Schedule page:

```text
      Schedule
ENABLED          YES
TIME           23:10
NEXT        Sun 00:06
```

If time is not available:

```text
      Schedule
ENABLED          YES
TIME        NOT SET
NEXT             --
```

The dry-soil page may stay awake as a warning, but the display button is allowed to temporarily override it and show the requested status/schedule page.

## Important design notes

- Do not hardcode Bangladesh time in firmware.
- Do not store local time in RTC.
- Do not run local schedule fallback without valid time.
- Keep API calls short so physical controls stay responsive while Laravel is offline.
- Keep cached config writes change-based to reduce flash wear.
- The soil sensor is optional, but if installed and reading critical dry soil, the warning page may stay awake.

## Future improvement

For better global timezone and DST support, Laravel can later send precomputed schedule runs:

```json
{
  "next_schedule_runs": [
    {
      "schedule_id": 9,
      "run_at_utc": "2026-05-12T18:06:00Z",
      "run_at_local": "2026-05-13 00:06:00",
      "duration_seconds": 30
    }
  ]
}
```

Then firmware can compare UTC epoch values instead of local day/time strings.
