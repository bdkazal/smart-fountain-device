# Smart Fountain Firmware Status and Design Notes

This document records the important firmware decisions, test findings, and next engineering steps for the Biztola Smart Fountain ESP32 firmware.

## Product context

Smart Fountain is part of the shared Biztola IoT Platform.

The firmware should be designed as:

```text
Platform-style reusable core
+ Smart Fountain product-specific module
```

It should not become a single large sketch that only works for one prototype.

## Current verified API flow

The firmware has been tested against the Laravel Biztola IoT Platform using the Smart Fountain device record.

Verified online flow:

```text
1. Boot ESP32
2. Load cached compact config from flash if available
3. Connect Wi-Fi
4. GET /api/device/config
5. Parse server_time_utc and sync DeviceClock
6. Parse config.outputs
7. Parse daily_timeline into RAM structs
8. Save compact config cache if changed
9. POST /api/device/state
10. GET /api/device/commands
11. ACK command as acknowledged
12. Apply output_set or scene_apply locally
13. ACK command as executed or failed
14. POST final actual output state
```

Important rule:

```text
POST /api/device/state is final trusted device truth.
```

Laravel may know the requested state, but firmware reports what was actually applied.

## Confirmed runtime behavior

Latest successful tests confirm:

```text
Loading cached Laravel config from flash. bytes=1228
Cached Laravel config loaded.
Config HTTP status: 200
DeviceClock synced. Local time: ...
Compact config cache JSON length: 1228
Config cache unchanged. Flash write skipped.
State HTTP status: 200
Command HTTP status: 200
Cloud mode: ONLINE - Laravel reachable, dashboard/API control active.
```

The previous crash loop was fixed by replacing full-config caching with compact-config caching.

## Config cache design

### Why full config caching failed

Earlier firmware attempted to save the full inner Laravel `config` object to ESP32 Preferences/NVS.

Observed size:

```text
Config cache JSON length: 4304
```

The device crashed with Guru Meditation after that line. The full config was too large/risky for Preferences/NVS string storage.

### Current compact cache

The firmware now builds and stores a compact firmware-only cache.

Current verified size:

```text
Compact config cache JSON length: 1228
```

The cache includes:

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
  ranges:
    period
    start_time
    end_time
    scene_id
    scene_name
    outputs:
      pump
      cob_light
      rgb_light
```

The cache intentionally excludes:

```text
server_time_utc
large capability metadata
dashboard-only labels
command documentation
safety description text
unused config blocks
```

Reason:

```text
server_time_utc changes every request and would cause unnecessary flash writes.
Large dashboard metadata is not needed for offline device behavior.
NVS should store small key-value data, not large documents.
```

## DeviceClock design

DeviceClock is the time source for offline scheduling.

It currently:

```text
parses server_time_utc from Laravel
applies timezone_offset_minutes
uses millis() to keep approximate time after sync
returns local HH:MM and local minutes since midnight
```

Verified example:

```text
server_time_utc: 2026-05-20T09:30:44+00:00
timezone_offset_minutes: 360
DeviceClock synced. Local time: 15:30 timezone_offset_minutes=360
```

Limitation:

```text
Time is only valid after at least one successful Laravel config sync.
No RTC fallback exists yet.
```

## Offline timeline behavior

The compact cache is apply-ready for offline timeline because the Laravel config includes output states inside each daily timeline range.

Verified serial output:

```text
- day 360 -> 1050 scene: Day Fountain outputs: yes
- evening 1050 -> 1380 scene: Night Glow outputs: yes
- night 1380 -> 360 scene: All Off outputs: yes
```

The firmware handles cross-midnight ranges. Example:

```text
night 1380 -> 360
```

means:

```text
active when local minute >= 1380 OR local minute < 360
```

OfflineTimeline behavior:

```text
Online / Laravel reachable:
  detect active range only
  do not apply cached outputs
  dashboard/API remains primary

Offline / API unavailable:
  detect active range
  apply cached range outputs locally
  use FountainOutputs so water-low pump safety still applies
  apply only once per active range

Recovery:
  when Laravel returns, POST /api/device/state sends actual locally-applied state
```

Verified offline apply example:

```text
Cloud mode: OFFLINE - API unavailable, cached local schedule may run.
OfflineTimeline active range: day scene: Day Fountain local_time=15:31 range=360->1050
OfflineTimeline applying cached range: day scene: Day Fountain
Pump state applied from offline_timeline: enabled=true speed=60
COB state applied from offline_timeline: enabled=true brightness=40
RGB state applied from offline_timeline: enabled=true brightness=35 color=#FFB066 effect=warm_glow
OfflineTimeline applied cached outputs locally. State will sync when Laravel is reachable.
```

Verified recovery example:

```text
State HTTP status: 200
State synced.
Cloud mode: ONLINE - Laravel reachable, dashboard/API control active.
```

Recovery payload preserved the locally-applied offline state:

```text
pump=true speed=60
cob_light=true brightness=40
rgb_light=true brightness=35 color=#FFB066 effect=warm_glow
```

## API retry and backoff behavior

Online intervals:

```text
state sync: 5 seconds
command poll: 5 seconds
config fetch: 60 seconds
```

Offline/API-unavailable intervals:

```text
state retry: 30 seconds
command retry: 30 seconds
config retry: 30 seconds
offline timeline check: 30 seconds
local water safety: every loop
```

Reason:

```text
Keep development responsive while Laravel is healthy.
Reduce repeated socket-error spam while Laravel/API is unavailable.
Keep local safety and offline timeline active during outage.
Automatically recover and sync actual state after Laravel returns.
```

## Local safety design

Water-low safety is a firmware responsibility.

Rules:

```text
water_low=true forces pump OFF locally
pump speed becomes 0
pump ON commands are ignored locally
COB light may still work
RGB light may still work
```

Reason:

```text
The pump must be protected even when Laravel, Wi-Fi, router, or internet is unavailable.
```

The firmware must keep this local rule even if Laravel also performs backend safety adjustments.

## Module structure

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
  ESP32 Preferences/NVS compact config storage.

include/OfflineTimeline.h
src/OfflineTimeline.cpp
  Detects active daily timeline range and applies cached range outputs while offline.

include/FountainOutputs.h
src/FountainOutputs.cpp
  Pump, COB, RGB output state application and water-low pump safety.

include/WaterLevelSensor.h
src/WaterLevelSensor.cpp
  Simulated water-level values now; later real ADC sensor implementation.

include/FountainTypes.h
  Shared output/readings/timeline structs.
```

## Known HTTP behavior

When Laravel is down or restarting, HTTP calls may return:

```text
State HTTP status: -1
Command HTTP status: -1
Config HTTP status: -1
Config HTTP status: -11
```

These are treated as temporary API availability failures, not firmware crashes.

The firmware then enters cloud-offline mode:

```text
Cloud mode: OFFLINE - API unavailable, cached local schedule may run.
```

## Next engineering steps

Recommended order from the current point:

```text
1. HardwareOutputs
   - map pump to MOSFET output
   - map COB to PWM output
   - map RGB hardware type/pins
   - keep product logic separate from physical pin writes

2. Real WaterLevelSensor
   - ADC read
   - calibration
   - filtering/debounce
   - failure-safe behavior

3. API cleanup
   - DeviceStateReporter
   - CommandClient
   - CommandProcessor
   - keep main.cpp smaller

4. Optional RTC/display
   - RTC for better offline time after long power loss
   - display for local product status
```

## Important testing checklist

Before merging major firmware changes, verify:

```text
[ ] Build succeeds
[ ] First boot loads cached config if present
[ ] GET /api/device/config returns 200
[ ] DeviceClock local time is correct
[ ] Compact cache length remains safely below NVS guard
[ ] Unchanged config skips flash write
[ ] POST /api/device/state returns 200
[ ] GET /api/device/commands returns 200
[ ] output_set works
[ ] scene_apply works
[ ] cloud mode ONLINE logs once
[ ] cloud mode OFFLINE logs once during API outage
[ ] OfflineTimeline applies cached outputs while API is unavailable
[ ] offline API retry backoff reduces socket-error spam
[ ] recovery syncs locally-applied state back to Laravel
[ ] water_low=true forces pump OFF
[ ] lights still work when water_low=true
[ ] reboot does not erase cached config
```

## Development note

Firmware comments should explain product rules and architectural intent, not obvious code operations.

Good comments explain:

```text
why local pump safety exists
why compact cache is used
why state sync is trusted hardware truth
why server_time_utc is not cached as config
why offline timeline needs valid time
why offline schedule must not fight Laravel while online
```
