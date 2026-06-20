# Daily Timeline Operation

This document explains the current Smart Fountain daily timeline design.

## Architecture decision

Smart Fountain daily timeline is a **24-hour mode timeline**, not a short-duration Laravel schedule.

The device normally has three persistent blocks:

```text
Day
Evening
Night
```

Each block applies a scene and keeps that scene active until the next block begins, unless a manual command, local button, safety rule, or recovery flow changes the outputs.

The authority split is:

```text
Laravel = configuration owner, dashboard/API owner, audit/state recorder
ESP32   = hardware executor, daily timeline executor, local safety authority
```

## Correct V1 execution model

```text
Dashboard/manual command = Laravel creates command, ESP32 applies it
Daily recurring timeline = ESP32 applies locally from cached config
Offline behavior         = ESP32 continues cached daily timeline
Recovery behavior        = ESP32 reports final actual state after Laravel/API recovers
```

Laravel should not be treated as the primary production executor for the day/evening/night timeline.

The ESP32 should evaluate and apply the daily timeline both while online and while offline.

## Daily timeline model

Example:

```text
day      06:00 -> 18:00  scene: Day Fountain
evening  18:00 -> 23:00  scene: Night Glow
night    23:00 -> 06:00  scene: All Off
```

Each range should include:

```text
id
period
name
is_enabled
start_time
end_time
scene_id
scene_name
outputs
```

Outputs should include complete V1 states for:

```text
pump
cob_light
rgb_light
```

V1 output shape:

```json
{
  "pump": {
    "enabled": true
  },
  "cob_light": {
    "enabled": true
  },
  "rgb_light": {
    "enabled": true,
    "brightness_percent": 25,
    "color": "#CD0404",
    "effect": "slow_rainbow"
  }
}
```

## Config source

Laravel `/api/device/config` is the source of configured timeline data.

Firmware should use:

```text
server_time_utc
config.timezone
config.timezone_offset_minutes
config.daily_timeline.enabled
config.daily_timeline.repeat
config.daily_timeline.ranges[]
```

Firmware should cache the compact config so the timeline can continue while Laravel/API is unavailable.

## Time and clock rule

```text
RTC/device clock stores UTC.
timezone_offset_minutes converts UTC to local schedule time.
start_time/end_time are local wall-clock times.
```

`server_time_utc` from Laravel is the preferred sync source when config is reachable.

## Active range detection

For normal same-day ranges:

```text
active when local_time >= start_time AND local_time < end_time
```

For cross-midnight ranges:

```text
active when start_time > end_time
active when local_time >= start_time OR local_time < end_time
```

Example:

```text
night 23:00 -> 06:00
active from 23:00 to 23:59
active from 00:00 to 05:59
inactive from 06:00 onward
```

## Idempotency rule

The ESP32 must not re-apply the same range repeatedly every loop.

Use a local key similar to:

```text
YYYY-MM-DD:range_id:start_time
```

Example:

```text
2026-06-20:2:18:00
```

Firmware should store:

```text
last_applied_timeline_key
```

Then:

```text
if current_range_key != last_applied_timeline_key:
    apply range scene
    save current_range_key
    queue/sync actual state when possible
else:
    do nothing
```

For a cross-midnight range, the key should use the local date that owns the active schedule occurrence. Keep this simple for V1, but make sure the range does not re-apply on every loop after midnight.

## Online timeline behavior

When Laravel/API is reachable:

```text
ESP32 fetches config
ESP32 syncs clock from server_time_utc
ESP32 evaluates current active range
ESP32 applies the active range once if needed
ESP32 posts actual state to Laravel
ESP32 continues polling manual/dashboard commands
```

Dashboard/manual commands can still override outputs. After any manual command, the next timeline transition may apply the next range.

## Offline timeline behavior

When Laravel/API is unavailable:

```text
compact daily_timeline cache already exists
DeviceClock has valid time
API/cloud becomes unavailable
ESP32 evaluates current local time
active cached range is detected
cached range outputs are applied once if needed
state sync is queued
Laravel/API recovers
ESP32 fetches config and syncs final actual state
```

## Laravel schedule command role

Laravel still has a compatibility/backstop command:

```bash
php artisan smart-fountain:check-schedules --time=18:00
```

Important:

```text
Current command supports --time only.
There is no --day option currently.
```

This command may create a `scene_apply` command during testing/migration, and firmware should still support that command type.

But this command is no longer the preferred production timeline authority.

## scene_apply compatibility

Firmware should continue supporting schedule-created `scene_apply` commands.

Example metadata:

```json
{
  "source": "schedule:5:evening",
  "schedule_range_id": 5,
  "schedule_name": "Evening",
  "schedule_period": "evening",
  "schedule_phase": "start",
  "repeat": "daily"
}
```

This keeps the old path testable while the device-side timeline executor becomes the main path.

## Water-low safety

Water-low safety must be enforced locally by firmware.

If `water_low=true`:

```text
pump must be forced OFF immediately
pump ON from manual commands must be ignored/blocked
pump ON from timeline scenes must be ignored/blocked
COB/RGB may still apply
state sync must report pump OFF and water_low=1
```

Laravel may also protect command payloads, but firmware must never depend only on Laravel for physical pump safety.

## State sync after timeline apply

After a timeline range changes actual outputs, firmware should send `/api/device/state` when possible.

Common source labels:

```text
device_timeline
offline_timeline
water_safety
```

Use `device_timeline` for normal online timeline application if implemented. Existing code may still use `offline_timeline` for cached/offline application until the source naming is refactored.

## Main behavior to verify next

```text
[ ] Config contains daily_timeline ranges with outputs
[ ] ESP32 applies active range while online
[ ] ESP32 does not re-apply the same range repeatedly
[ ] ESP32 applies next range at an online boundary
[ ] ESP32 applies cached range while Laravel/API is offline
[ ] ESP32 syncs final actual state after recovery
[ ] Cross-midnight range behaves correctly
[ ] Water-low safety blocks pump during manual command and timeline execution
```

Do not start MQTT yet.
