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

Verified flow:

```text
1. Boot ESP32
2. Load cached compact config from flash if available
3. Connect Wi-Fi
4. GET /api/device/config
5. Parse config.outputs
6. Parse daily_timeline
7. Save compact config cache if changed
8. POST /api/device/state
9. GET /api/device/commands
10. ACK command as acknowledged
11. Apply output_set or scene_apply locally
12. ACK command as executed or failed
13. POST final actual output state
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
Compact config cache JSON length: 1228
Config cache unchanged. Flash write skipped.
State HTTP status: 200
Command HTTP status: 200
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

## Offline timeline readiness

The compact cache is now apply-ready for offline timeline because the Laravel config includes output states inside each daily timeline range.

Verified serial output:

```text
- day 06:00 -> 17:30 scene: Day Fountain outputs: yes
- evening 17:30 -> 23:00 scene: Night Glow outputs: yes
- night 23:00 -> 06:00 scene: All Off outputs: yes
```

This means firmware can later apply offline schedules without asking Laravel for a scene.

Remaining requirement:

```text
DeviceClock must exist before OfflineTimeline can run safely.
```

Without valid time, the device must not guess schedule ranges.

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
  Boot/runtime loop, Wi-Fi, high-level API flow, state sync, command polling.

include/ApiClient.h
src/ApiClient.cpp
  Laravel base URL normalization and common device API headers.

include/FountainConfig.h
src/FountainConfig.cpp
  Config parsing, compact cache building, daily timeline printing.

include/ConfigCache.h
src/ConfigCache.cpp
  ESP32 Preferences/NVS compact config storage.

include/FountainOutputs.h
src/FountainOutputs.cpp
  Pump, COB, RGB output state application and water-low pump safety.

include/WaterLevelSensor.h
src/WaterLevelSensor.cpp
  Simulated water-level values now; later real ADC sensor implementation.

include/FountainTypes.h
  Shared state structs.
```

## Known HTTP behavior

Occasionally, right after boot or reconnect, an HTTP call can return:

```text
Config HTTP status: -11
```

The firmware now retries config fetch once. This is treated as a temporary network/server timing issue, not a fatal firmware error.

State sync and command polling have continued successfully after such temporary failures.

## Next engineering steps

Recommended order:

```text
1. DeviceClock
   - parse server_time_utc
   - apply timezone_offset_minutes
   - keep approximate local time using millis()
   - expose current HH:MM

2. OfflineTimeline
   - read cached daily_timeline
   - find active range by local HH:MM
   - apply cached range outputs locally
   - avoid reapplying same range repeatedly
   - respect water-low pump safety

3. HardwareOutputs
   - map pump to MOSFET output
   - map COB to PWM output
   - map RGB hardware type/pins

4. Real WaterLevelSensor
   - ADC read
   - calibration
   - filtering/debounce
   - failure-safe behavior

5. Further API cleanup
   - DeviceStateReporter
   - CommandClient
   - CommandProcessor
```

## Important testing checklist

Before merging major firmware changes, verify:

```text
[ ] Build succeeds
[ ] First boot loads cached config if present
[ ] GET /api/device/config returns 200
[ ] Compact cache length remains safely below NVS guard
[ ] Unchanged config skips flash write
[ ] POST /api/device/state returns 200
[ ] GET /api/device/commands returns 200
[ ] output_set works
[ ] scene_apply works
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
```
