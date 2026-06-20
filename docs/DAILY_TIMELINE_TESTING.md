# Daily Timeline Testing

This document explains how to test Smart Fountain daily timeline behavior on real ESP32 hardware.

## Current test goal

Smart Fountain daily timeline is now tested as a device-side executor:

```text
Laravel = config/dashboard/state recorder
ESP32   = online/offline timeline executor + hardware safety authority
```

The old Laravel `smart-fountain:check-schedules` command still exists, but it is a development/backstop/manual compatibility path, not the preferred production timeline engine.

## Preconditions

Before testing:

```text
Laravel server is running on LAN IP
ESP32 has correct DEVICE_UUID
ESP32 has correct DEVICE_API_KEY
ESP32 has fetched config at least once
Serial monitor is open
Daily timeline ranges exist in Laravel
Each range has a start scene
Each scene has complete pump/COB/RGB output states
ESP32 has valid clock data from Laravel server_time_utc or RTC
```

## Confirm config contains daily timeline

Boot ESP32 with Laravel online.

Expected logs:

```text
Config HTTP status: 200
server_time_utc: ...
DeviceClock synced from Laravel UTC.
Daily timeline enabled: yes
Daily timeline repeat: daily
Timeline range count: 3
 - day ... scene: Day Fountain outputs: yes
 - evening ... scene: Night Glow outputs: yes
 - night ... scene: All Off outputs: yes
```

If `outputs: no` appears, fix the Laravel scene/range data first.

## Test 1: ESP32 applies active range while online

Goal:

```text
ESP32 fetches Laravel config
ESP32 syncs clock
ESP32 determines active local range
ESP32 applies the active range locally
ESP32 sends actual state to Laravel
```

Expected ESP32 output:

```text
DailyTimeline active range: ...
DailyTimeline applying range: ...
... outputs applied ...
POST /api/device/state
State HTTP status: 200
State synced.
```

Expected Laravel result:

```text
platform output states match ESP32 actual state
last_seen_at updates
water_low reading updates if present
```

## Test 2: Idempotency protection

The same active range should not be re-applied on every loop.

Expected behavior after first apply:

```text
DailyTimeline active range unchanged; already applied.
```

or equivalent.

Recommended key:

```text
YYYY-MM-DD:range_id:start_time
```

Example:

```text
2026-06-20:2:18:00
```

## Test 3: Online boundary transition

1. Keep Laravel running.
2. Set a timeline range boundary a few minutes ahead.
3. Make sure ESP32 fetches the latest config.
4. Keep ESP32 serial monitor open.
5. Wait for local device time to cross the boundary.

Expected:

```text
ESP32 detects new active range
ESP32 applies new range scene locally
ESP32 sends /api/device/state
Laravel output states update from actual device state
```

This test should not depend on Laravel creating a `scene_apply` command.

## Test 4: Offline cached timeline execution

1. Start Laravel.
2. Boot ESP32.
3. Confirm config fetch 200.
4. Confirm daily timeline ranges printed.
5. Confirm compact cache saved/unchanged.
6. Set a range boundary a few minutes ahead if needed.
7. Let ESP32 fetch updated config.
8. Stop Laravel.
9. Wait for API offline mode.
10. Wait for range boundary.
11. Confirm ESP32 applies cached outputs.
12. Restart Laravel.
13. Confirm final actual state syncs.

Expected offline logs:

```text
Cloud mode: OFFLINE - API unavailable...
DailyTimeline active range: ...
DailyTimeline applying cached range: ...
... state applied from offline_timeline or device_timeline ...
Local output change queued for Laravel state sync.
```

Expected recovery logs:

```text
API offline mode: probing Laravel before local state sync...
Config HTTP status: 200
API recovered. request=config_probe
Cloud mode: ONLINE - Laravel reachable, dashboard/API control active.
POST /api/device/state
State HTTP status: 200
State synced.
```

The important behavior is that final actual outputs sync to Laravel after recovery.

## Test 5: Cross-midnight range

Use a range like:

```text
night 23:00 -> 06:00
```

Expected active periods:

```text
23:00 to 23:59 active
00:00 to 05:59 active
06:00 onward inactive
```

Firmware rule:

```text
active when local_time >= start_time OR local_time < end_time
```

Confirm the idempotency key does not cause repeated re-apply every loop after midnight.

## Test 6: Water-low safety during timeline execution

Force/simulate `water_low=true`.

Then let a timeline range apply a scene that would normally turn pump ON.

Expected result:

```text
pump remains OFF
COB/RGB may still apply
state sync reports pump OFF
water_low reading is reported
source may include water_safety for pump
```

## Test 7: Manual Laravel command compatibility

This is a compatibility/backstop test only.

In the Laravel repo, run the schedule command manually:

```bash
php artisan smart-fountain:check-schedules --time=18:00
```

Important:

```text
Use a time that exactly matches an enabled daily timeline range start_time.
The current command supports --time only.
There is no --day option currently.
```

Possible Laravel output:

```text
Created 1 Smart Fountain daily timeline command(s).
```

If a command is created, expected ESP32 output:

```text
GET /api/device/commands
Command HTTP status: 200
Processing command #... type=scene_apply
ACK command ... as acknowledged
ACK HTTP status: 200
... outputs applied ...
ACK command ... as executed
ACK HTTP status: 200
POST /api/device/state
State HTTP status: 200
State synced.
```

## If Laravel manual command creates 0 commands

This only applies to the backstop/manual command path.

Check:

```text
device status is active
device last_seen_at is fresh
timeline range is enabled
time exactly matches start_time
period_key is day, evening, or night
range has start_scene_id
scene has valid outputs
range was not already started today
no pending/acknowledged command exists for that range
```

For same-day retesting, Laravel may skip the range because `last_started_on` already equals today. Use another range/time or reset the schedule range started fields during development.

## Test evidence to capture

When reporting a timeline test, paste logs containing:

```text
Config HTTP status
Daily timeline enabled/repeat/range count
DailyTimeline active/applying range
State HTTP status
Cloud mode ONLINE/OFFLINE
API recovered
final State synced
water_low safety decision if tested
```

For manual-command compatibility testing, also capture:

```text
Processing command type=scene_apply
ACK acknowledged/executed
```

## Decision rule

Do not begin MQTT work until these tests pass:

```text
[ ] Config contains daily_timeline ranges with outputs
[ ] ESP32 applies active range while online
[ ] ESP32 does not re-apply the same range repeatedly
[ ] ESP32 applies next range at an online boundary
[ ] ESP32 applies cached range while Laravel/API is offline
[ ] Recovery final sync works
[ ] Water-low safety blocks pump during timeline execution
[ ] Manual Laravel command compatibility still works if needed
```
