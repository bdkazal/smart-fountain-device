# Biztola Smart Fountain Device Firmware

ESP32 firmware for the **Biztola Smart Fountain** product.

This repository contains the device-side runtime for the Smart Fountain. The backend lives in:

```text
bdkazal/biztola-iot-platform
```

The Smart Fountain is a **persistent state device**. Outputs stay in their last applied state until another command, schedule, local button, safety rule, or recovery flow changes them.

## Documentation

Full documentation is kept in [`docs/`](docs/).

| File | Purpose |
| --- | --- |
| [`docs/SMART_FOUNTAIN_DEVICE.md`](docs/SMART_FOUNTAIN_DEVICE.md) | Full device/product overview and current firmware status. |
| [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) | Firmware module architecture and responsibility boundaries. |
| [`docs/SETUP_AND_FLASHING.md`](docs/SETUP_AND_FLASHING.md) | Setup, secrets, build, upload, and monitor workflow. |
| [`docs/HARDWARE_PINS.md`](docs/HARDWARE_PINS.md) | GPIO pin map and hardware assumptions. |
| [`docs/STATUS_INDICATORS.md`](docs/STATUS_INDICATORS.md) | Network/server and water-safety LED indicator behavior. |
| [`docs/API_AND_STATE_FLOW.md`](docs/API_AND_STATE_FLOW.md) | Laravel config, commands, ACK, state sync, and payload rules. |
| [`docs/OFFLINE_RECOVERY_AND_SAFETY.md`](docs/OFFLINE_RECOVERY_AND_SAFETY.md) | Offline mode, local controls, recovery sync, config cache, and water safety. |
| [`docs/DAILY_TIMELINE_OPERATION.md`](docs/DAILY_TIMELINE_OPERATION.md) | Online Laravel schedule vs offline cached timeline behavior. |
| [`docs/DAILY_TIMELINE_TESTING.md`](docs/DAILY_TIMELINE_TESTING.md) | Real-device schedule/timeline test guide. |
| [`docs/TESTING_CHECKLIST.md`](docs/TESTING_CHECKLIST.md) | Firmware smoke-test checklist. |
| [`docs/MQTT_COMMANDS.md`](docs/MQTT_COMMANDS.md) | MQTT command subscriber setup and behavior. |

Note: the Laravel repository uses uppercase `Docs/`. This firmware repository uses lowercase `docs/`.

## Current stable status

The firmware is stable after the runtime refactor chain through PR #15.

Verified on real ESP32 hardware before the latest controller GPIO move:

```text
[x] PlatformIO ESP32 project builds
[x] Wi-Fi stored credentials boot path works
[x] GET /api/device/config works
[x] server_time_utc sync works
[x] DS3231/HW-111 RTC sync/update path works
[x] compact config cache loads before Wi-Fi/API
[x] compact config cache saves only when changed
[x] config outputs parse into firmware state
[x] daily_timeline ranges parse into firmware state
[x] POST /api/device/state works
[x] GET /api/device/commands works
[x] ACK acknowledged/executed works
[x] output_set commands work
[x] scene_apply command handling exists
[x] local pump button works
[x] local COB button works
[x] local button changes sync to Laravel
[x] API offline mode works
[x] local controls work while API is offline
[x] API recovery probe works
[x] final queued local state syncs after Laravel recovers
[x] network/server status LED works on old GPIO23 assignment
[x] water safety status LED works on old GPIO13 assignment
```

Focused behavior still needing real-device verification:

```text
[ ] local pump button works after moving to GPIO19
[ ] local COB button works after moving to GPIO18
[ ] Wi-Fi reset/setup button works from GPIO23
[ ] network/server status LED works after moving to GPIO17
[ ] water safety status LED works after moving to GPIO16
[ ] Laravel online daily timeline creates scene_apply at range start time
[ ] ESP32 applies scheduled scene_apply command and syncs actual state
[ ] ESP32 offline cached timeline applies range outputs at real boundary time
[ ] water-low safety still protects pump during scheduled/offline timeline apply
[ ] MQTT command subscriber connects and processes dashboard commands
[ ] REST command polling still works as fallback
```

## Current controller GPIO layout

```text
Fountain/RJ45/external-load side:
GPIO25 = pump MOSFET control
GPIO26 = COB MOSFET control
GPIO27 = RGB / NeoPixel data
GPIO32 = float switch input

Controller-box local side:
GPIO19 = pump local button
GPIO18 = COB local button
GPIO23 = Wi-Fi reset/setup button
GPIO17 = network/server status LED
GPIO16 = water safety status LED
GPIO21 = RTC SDA
GPIO22 = RTC SCL
```

## Quick setup

Copy secrets:

```bash
cp include/DeviceSecrets.example.h include/DeviceSecrets.h
```

Edit `include/DeviceSecrets.h`:

```cpp
#define WIFI_SSID "YOUR_WIFI_NAME"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"
#define API_BASE_URL "http://192.168.0.xxx:8000"
#define DEVICE_UUID "YOUR_SMART_FOUNTAIN_DEVICE_UUID"
#define DEVICE_API_KEY "YOUR_SMART_FOUNTAIN_DEVICE_API_KEY"
```

Do **not** use `127.0.0.1` for `API_BASE_URL` from ESP32. Use the Mac/Laravel server LAN IP.

Run Laravel on the LAN:

```bash
php artisan serve --host=0.0.0.0 --port=8000
```

## Build, upload, monitor

```bash
pio run
pio run -t upload
pio device monitor -b 115200
```

The monitor speed is also configured in `platformio.ini`.

## Expected normal boot

```text
Biztola Smart Fountain ESP32 starting...
Device storage initialized.
Network/server status LED ready: GPIO 17 active_HIGH
Water safety status LED ready: GPIO 16 active_HIGH
All outputs forced OFF: safe boot hardware default
Output state untrusted: safe boot OFF is hardware-only until cached or fresh config loads
Loading cached Laravel config from flash. bytes=...
Cached Laravel config loaded.
Trying stored Wi-Fi credentials...
Wi-Fi connected. IP: ...
GET /api/device/config
Config HTTP status: 200
server_time_utc: ...
DeviceClock synced from Laravel UTC.
Daily timeline enabled: yes
Timeline range count: 3
POST /api/device/state
State HTTP status: 200
State synced.
Cloud mode: ONLINE - Laravel reachable, dashboard/API control active.
```

## Core behavior summary

### Online mode

Laravel is reachable and controls live dashboard/API behavior.

```text
Dashboard command -> Laravel pending command -> ESP32 poll -> ACK acknowledged -> apply locally -> ACK executed -> POST actual state
```

With MQTT enabled, dashboard commands can also arrive through the MQTT command subscriber. REST polling remains enabled as fallback.

### Offline/API-unavailable mode

Firmware keeps local control and safety active.

```text
API failures -> server-offline mode -> local buttons still work -> local state queued -> recovery probe -> final actual state syncs to Laravel
```

### Status indicators

```text
Network LED:
fast blink = Wi-Fi disconnected or setup portal active
slow blink = Wi-Fi connected but Laravel/API offline
solid ON   = Wi-Fi connected and Laravel/API online

Water safety LED:
OFF      = water OK
solid ON = water low / pump protected
```

See:

```text
docs/STATUS_INDICATORS.md
docs/HARDWARE_PINS.md
```

### Daily timeline

There are two separate timeline paths:

```text
Online schedule: Laravel creates scene_apply command.
Offline fallback: ESP32 applies cached daily_timeline range locally.
```

See:

```text
docs/DAILY_TIMELINE_OPERATION.md
docs/DAILY_TIMELINE_TESTING.md
```

## MQTT command support

MQTT command subscriber support is implemented for V1. See:

```text
docs/MQTT_COMMANDS.md
```
