# Biztola Smart Fountain Device Firmware

ESP32-C3 firmware for the **Biztola Smart Fountain** product.

This device is part of the shared Biztola IoT Platform. It is not designed as a one-off fountain-only sketch. The firmware is being shaped into reusable platform-style modules plus a Smart Fountain product module.

Backend repository:

```text
bdkazal/biztola-iot-platform
```

Firmware repository:

```text
bdkazal/smart-fountain-device
```

## Current firmware status

Core API and state flow:

```text
[x] PlatformIO ESP32-C3 project
[x] Wi-Fi connection and retry
[x] GET /api/device/config
[x] parse server_time_utc
[x] parse Smart Fountain config.outputs
[x] parse daily_timeline ranges into RAM structs
[x] POST /api/device/state
[x] GET /api/device/commands
[x] ACK command as acknowledged
[x] ACK command as executed/failed
[x] handle output_set in local state
[x] handle scene_apply in local state
[x] report actual hardware/output state as final truth
```

Safety, cache, and offline behavior:

```text
[x] local water_low pump safety
[x] pump ON ignored when water_low=true
[x] COB/RGB allowed when water_low=true
[x] compact config cache in ESP32 Preferences/NVS
[x] cache loads before Wi-Fi/API fetch
[x] cache avoids unnecessary flash writes when unchanged
[x] cached daily_timeline ranges include output states
[x] DeviceClock parses server_time_utc and local timezone offset
[x] OfflineTimeline detects active daily range
[x] OfflineTimeline applies cached range outputs when API/cloud is unavailable
[x] cloud mode logging: ONLINE / OFFLINE
[x] offline API retry backoff to reduce socket-error spam
[x] recovery: actual locally-applied state syncs after Laravel returns
```

Hardware still pending:

```text
[ ] real pump output pin / MOSFET module integration
[ ] COB PWM pin / dimming integration
[ ] RGB light hardware type and pin integration
[ ] real water-level sensor pin/calibration
[ ] optional RTC module
[ ] optional display
```

Next firmware modules:

```text
[x] DeviceClock: server UTC + timezone offset + millis-based clock
[x] OfflineTimeline: use cached timeline to apply outputs without Laravel
[ ] HardwareOutputs: real pump / COB / RGB drivers
[ ] WaterLevelSensor: ADC calibration and filtering
[ ] API extraction: state reporter, command client, command processor
```

## Local setup

Copy the example secrets file:

```bash
cp include/DeviceSecrets.example.h include/DeviceSecrets.h
```

Edit `include/DeviceSecrets.h` with your local values:

```cpp
#define WIFI_SSID "YOUR_WIFI_NAME"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"
#define API_BASE_URL "http://192.168.0.xxx:8000"
#define DEVICE_UUID "YOUR_SMART_FOUNTAIN_DEVICE_UUID"
#define DEVICE_API_KEY "YOUR_SMART_FOUNTAIN_DEVICE_API_KEY"
```

Important: do **not** use `127.0.0.1` for `API_BASE_URL` from ESP32. Use the Mac/Laravel server LAN IP.

Run Laravel backend like this:

```bash
php artisan serve --host=0.0.0.0 --port=8000
```

## Build, upload, monitor

From this repository:

```bash
pio run
pio run --target upload
pio device monitor
```

The serial monitor speed is configured in `platformio.ini`.

## Expected boot behavior

Normal online boot should show:

```text
Biztola Smart Fountain ESP32 starting...
Loading cached Laravel config from flash. bytes=...
Cached Laravel config loaded.
Wi-Fi connected
GET /api/device/config
Config HTTP status: 200
server_time_utc: ...
device_type: smart_fountain
DeviceClock synced. Local time: ...
Daily timeline repeat: daily
Compact config cache JSON length: ...
Config cache unchanged. Flash write skipped.
POST /api/device/state
GET /api/device/commands
Cloud mode: ONLINE - Laravel reachable, dashboard/API control active.
```

First boot after empty cache should show:

```text
No cached Laravel config found.
GET /api/device/config
Compact config cache JSON length: ...
Config cache saved to flash. bytes=...
```

## Online / offline behavior

When Laravel/API is reachable, dashboard/API control remains primary:

```text
Cloud mode: ONLINE - Laravel reachable, dashboard/API control active.
OfflineTimeline active range: day scene: Day Fountain local_time=15:31 range=360->1050
```

In online mode, OfflineTimeline only detects the active range. It does **not** apply cached outputs while Laravel is healthy.

When Laravel/API becomes unavailable but Wi-Fi is still connected:

```text
Cloud mode: OFFLINE - API unavailable, cached local schedule may run.
OfflineTimeline applying cached range: day scene: Day Fountain
Pump state applied from offline_timeline: enabled=true speed=60
COB state applied from offline_timeline: enabled=true brightness=40
RGB state applied from offline_timeline: enabled=true brightness=35 color=#FFB066 effect=warm_glow
OfflineTimeline applied cached outputs locally. State will sync when Laravel is reachable.
```

When Laravel/API recovers, the firmware syncs the actual locally-applied state back to Laravel:

```text
State HTTP status: 200
State synced.
Cloud mode: ONLINE - Laravel reachable, dashboard/API control active.
```

Verified recovery state example:

```text
pump=true speed=60
cob_light=true brightness=40
rgb_light=true brightness=35 color=#FFB066 effect=warm_glow
```

## API retry intervals

Online mode:

```text
state sync: 5 seconds
command poll: 5 seconds
config fetch: 60 seconds
```

Offline/API-unavailable mode:

```text
state retry: 30 seconds
command retry: 30 seconds
config retry: 30 seconds
offline timeline check: 30 seconds
local water safety: always active
```

This keeps development responsive while online, but reduces repeated socket-error logs while Laravel/API is unavailable.

## Safety rule

The pump must be protected locally.

If `water_low=true`:

```text
pump OFF
speed_percent = 0
pump ON commands ignored locally
lights may still work
```

This rule must remain inside firmware, not only Laravel. The fountain should protect the pump even when Wi-Fi, internet, router, or Laravel is unavailable.

## Config cache rule

The firmware does **not** cache the full Laravel `/api/device/config` response.

Reason:

```text
The full config can be too large for ESP32 Preferences/NVS.
server_time_utc changes every request.
Dashboard-only metadata is not needed offline.
```

Instead, firmware stores a compact cache containing only what it needs:

```text
cache_version
device_type
timezone_offset_minutes
outputs:
  pump
  cob_light
  rgb_light
daily_timeline:
  enabled
  repeat
  ranges with scene output states
```

Latest verified cache size during development:

```text
Compact config cache JSON length: 1228
```

## Offline timeline behavior

The current Laravel config response includes output states inside each daily timeline range. Serial output confirms:

```text
- day 360 -> 1050 scene: Day Fountain outputs: yes
- evening 1050 -> 1380 scene: Night Glow outputs: yes
- night 1380 -> 360 scene: All Off outputs: yes
```

The `night` range crosses midnight, so firmware treats it as:

```text
active when local_time >= 23:00 OR local_time < 06:00
```

OfflineTimeline only applies cached outputs when cloud/API is unavailable. This avoids fighting dashboard commands while Laravel is online.

## Main firmware modules

```text
src/main.cpp
  Boot/runtime loop, Wi-Fi, high-level API flow, state sync, command polling, cloud-mode gating.

include/ApiClient.h
src/ApiClient.cpp
  Laravel base URL normalization and common device API headers.

include/DeviceClock.h
src/DeviceClock.cpp
  Parses server_time_utc, applies timezone offset, exposes local HH:MM/minutes.

include/FountainConfig.h
src/FountainConfig.cpp
  Config parsing, compact cache building, daily timeline loading into RAM structs.

include/ConfigCache.h
src/ConfigCache.cpp
  ESP32 Preferences/NVS storage for compact firmware config.

include/OfflineTimeline.h
src/OfflineTimeline.cpp
  Detects active daily timeline range and applies cached range outputs while offline.

include/FountainOutputs.h
src/FountainOutputs.cpp
  Pump, COB, RGB state application and local water-low pump safety.

include/WaterLevelSensor.h
src/WaterLevelSensor.cpp
  Simulated water-level readings for now; later real ADC sensor logic.

include/FountainTypes.h
  Shared output/readings/timeline structs.
```

## Current limitations

```text
- Output state is software-only until hardware pins are wired.
- Water sensor is simulated until ADC hardware is connected.
- Offline schedule depends on last successful server_time_utc sync.
- No RTC fallback yet.
- API code is still mostly in main.cpp and should later be extracted.
```
