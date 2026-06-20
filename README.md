# Biztola Smart Fountain Device Firmware

ESP32 firmware for the **Biztola Smart Fountain** product.

This repository contains the device-side runtime for the Smart Fountain. The backend lives in:

```text
bdkazal/biztola-iot-platform
```

The Smart Fountain is a **persistent state device**. Outputs stay in their last applied state until another command, daily timeline range, local button, safety rule, or recovery flow changes them.

## Documentation

Full documentation is kept in [`docs/`](docs/).

| File | Purpose |
| --- | --- |
| [`docs/SMART_FOUNTAIN_DEVICE.md`](docs/SMART_FOUNTAIN_DEVICE.md) | Full device/product overview and current firmware status. |
| [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) | Firmware module architecture and responsibility boundaries. |
| [`docs/SETUP_AND_FLASHING.md`](docs/SETUP_AND_FLASHING.md) | Setup, secrets, build, upload, and monitor workflow. |
| [`docs/HARDWARE_PINS.md`](docs/HARDWARE_PINS.md) | GPIO pin map and hardware assumptions. |
| [`docs/API_AND_STATE_FLOW.md`](docs/API_AND_STATE_FLOW.md) | Laravel config, dashboard/manual commands, firmware timeline execution, ACK, and state sync. |
| [`docs/OFFLINE_RECOVERY_AND_SAFETY.md`](docs/OFFLINE_RECOVERY_AND_SAFETY.md) | Offline mode, local controls, recovery sync, config cache, and water safety. |
| [`docs/DAILY_TIMELINE_OPERATION.md`](docs/DAILY_TIMELINE_OPERATION.md) | ESP32 online/offline daily timeline execution from cached Laravel config. |
| [`docs/DAILY_TIMELINE_TESTING.md`](docs/DAILY_TIMELINE_TESTING.md) | Real-device daily timeline test guide. |
| [`docs/TESTING_CHECKLIST.md`](docs/TESTING_CHECKLIST.md) | Firmware smoke-test checklist. |

Note: the Laravel repository uses uppercase `Docs/`. This firmware repository uses lowercase `docs/`.

## Current stable status

The firmware is stable after the runtime refactor chain through PR #9.

Verified on real ESP32 hardware:

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
```

Focused behavior still needing real-device verification:

```text
[ ] ESP32 applies active daily_timeline range while online
[ ] ESP32 does not re-apply the same range repeatedly
[ ] ESP32 applies next range at a real boundary time
[ ] ESP32 applies cached daily_timeline range while Laravel/API is offline
[ ] ESP32 syncs final actual state after Laravel/API recovery
[ ] water-low safety still protects pump during manual command and timeline apply
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

Laravel is reachable and provides config, dashboard/manual commands, and state recording.

```text
Dashboard command -> Laravel pending command -> ESP32 poll -> ACK acknowledged -> apply locally -> ACK executed -> POST actual state
```

### Offline/API-unavailable mode

Firmware keeps local control, timeline, and safety active.

```text
API failures -> server-offline mode -> local buttons/timeline still work -> local state queued -> recovery probe -> final actual state syncs to Laravel
```

### Daily timeline

There is one preferred timeline executor:

```text
Laravel configures daily_timeline.
ESP32 executes daily_timeline locally while online and offline.
Laravel stores final actual state after ESP32 syncs.
```

The Laravel schedule command may still be used as a compatibility/backstop/manual test path, but it is not the preferred production timeline engine.

See:

```text
docs/DAILY_TIMELINE_OPERATION.md
docs/DAILY_TIMELINE_TESTING.md
```
