# Daily Timeline Operation

This document explains how Smart Fountain schedule/timeline behavior is supposed to work.

## Important distinction

There are two different timeline paths.

```text
Online timeline = Laravel-owned schedule command
Offline timeline = firmware-owned cached fallback
```

Do not mix these two behaviors.

## Daily timeline model

Smart Fountain V1 uses three daily ranges:

```text
Day
Evening
Night
```

Example:

```text
day      06:00 -> 18:00  scene: Day Fountain
evening  18:00 -> 23:00  scene: Night Glow
night    23:00 -> 06:00  scene: All Off
```

Each range should include:

```text
period
name
start_time
end_time
scene_id
scene_name
outputs
```

Outputs should include complete states for:

```text
pump
cob_light
rgb_light
```

## Cross-midnight behavior

A range crosses midnight when:

```text
start_time > end_time
```

Example:

```text
23:00 -> 06:00
```

Firmware treats that as active when:

```text
local_time >= 23:00 OR local_time < 06:00
```

## Online timeline behavior

When Laravel is online, schedule execution happens in Laravel.

Flow:

```text
Laravel scheduler runs every minute
→ smart-fountain:check-schedules checks enabled schedule ranges
→ current device-local time matches range start_time
→ Laravel creates scene_apply command
→ ESP32 polls command
→ ESP32 ACKs acknowledged
→ ESP32 applies scene outputs locally
→ ESP32 ACKs executed or failed
→ ESP32 POSTs actual state
```

The firmware does not create online schedule commands by itself.

## Laravel manual schedule command

Current Laravel command:

```bash
php artisan smart-fountain:check-schedules --time=18:00
```

Important:

```text
Current command supports --time only.
There is no --day option currently.
```

The time must exactly match a range `start_time`.

## Laravel online command creation conditions

Laravel creates a scheduled command only if:

```text
device exists
device type is smart_fountain
device status is active
schedule range is enabled
period_key is day/evening/night
current time exactly equals start_time
range was not already started today
device is recently online
range has start scene
scene has valid output states
```

Current Laravel online check is based on recent `last_seen_at`.

If the device is not recently online, Laravel skips command creation.

## Schedule-created command payload

Online schedule uses the normal `scene_apply` command type.

Payload includes schedule metadata:

```json
{
  "scene_id": 1,
  "scene_name": "Night Glow",
  "source": "schedule:5:evening",
  "schedule_range_id": 5,
  "schedule_name": "Evening",
  "schedule_period": "evening",
  "schedule_phase": "start",
  "repeat": "daily",
  "outputs": {
    "pump": {
      "enabled": true,
      "speed_percent": 100
    },
    "cob_light": {
      "enabled": true,
      "brightness_percent": 100
    },
    "rgb_light": {
      "enabled": true,
      "brightness_percent": 25,
      "color": "#CD0404",
      "effect": "slow_rainbow"
    }
  }
}
```

## Offline timeline behavior

When Laravel/API is unavailable, firmware can use cached daily timeline data.

Flow:

```text
Laravel config was fetched successfully earlier
→ compact daily_timeline cache exists in ESP32 flash
→ DeviceClock has valid time
→ API/cloud becomes unavailable
→ OfflineTimeline checks current local time
→ active cached range is detected
→ cached range outputs are applied locally if needed
→ local state sync is queued
→ after Laravel recovery, final actual state syncs
```

## Why offline timeline does not apply while online

When Laravel is reachable, dashboard/API commands are the active source of truth.

If firmware applied cached timeline while online, it could fight dashboard commands.

So the rule is:

```text
Online: detect active range but do not apply cached outputs.
Offline: cached timeline may apply range outputs locally.
```

## Current firmware OfflineTimeline behavior

`OfflineTimeline.update()`:

```text
detects active range
logs range changes
safe to run while online
no output changes
```

`OfflineTimeline.applyActiveRangeIfNeeded()`:

```text
checks active range
applies only if not already applied
uses FountainOutputs safe output path
honors water-low pump protection
```

## Timeline and recovery

If offline timeline changes outputs while Laravel is down:

```text
outputs change locally
state sync is queued
Laravel recovers
firmware probes config endpoint
firmware syncs final actual output state
```

## Main behavior not yet fully verified

Needs real-device verification:

```text
[ ] Laravel manual schedule command creates scene_apply
[ ] ESP32 receives and applies schedule-created scene_apply
[ ] Real every-minute Laravel scheduler triggers command at start_time
[ ] OfflineTimeline applies cached range at actual boundary while Laravel is down
[ ] Cross-midnight range behaves correctly on device
[ ] Water-low safety blocks pump during scheduled scene/offline range
```
