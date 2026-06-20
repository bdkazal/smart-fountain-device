# Daily Timeline Testing

This document explains how to test Smart Fountain daily timeline behavior on real ESP32 hardware.

## Test paths

There are two paths to test:

```text
1. Online Laravel schedule command path
2. Offline cached firmware timeline fallback path
```

Run them separately.

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

## Test 1: Manual Laravel online schedule command

In the Laravel repo, run the schedule command manually.

Example:

```bash
php artisan smart-fountain:check-schedules --time=18:00
```

Important:

```text
Use a time that exactly matches an enabled daily timeline range start_time.
The current command supports --time only.
There is no --day option currently.
```

Expected Laravel output:

```text
Created 1 Smart Fountain daily timeline command(s).
```

Expected ESP32 output:

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

## If Laravel creates 0 commands

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
```

For same-day retesting, Laravel may skip the range because `last_started_on` already equals today. Use another range/time or reset the schedule range started fields during development.

## Test 2: Real scheduler minute test

Set one timeline range start time a few minutes ahead.

Run Laravel scheduler worker:

```bash
php artisan schedule:work
```

Keep ESP32 serial monitor open.

Expected flow:

```text
Laravel scheduler runs smart-fountain:check-schedules every minute
Laravel creates pending scene_apply command
ESP32 polls command
ESP32 applies scene
ESP32 ACKs executed
ESP32 POSTs state
```

## Test 3: Offline cached timeline fallback

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
11. Confirm OfflineTimeline applies cached outputs.
12. Restart Laravel.
13. Confirm final actual state syncs.

Expected offline logs:

```text
Cloud mode: OFFLINE - API unavailable...
OfflineTimeline active range: ...
OfflineTimeline applying cached range: ...
... state applied from offline_timeline ...
Local output change queued for Laravel state sync.
```

Expected recovery logs:

```text
API offline mode: probing Laravel before local state sync...
Config HTTP status: 200
API recovered. request=config_probe
Cloud mode: ONLINE - Laravel reachable, dashboard/API control active.
Syncing local button output change to Laravel...
POST /api/device/state
State HTTP status: 200
State synced.
```

The log may still say `local button output change` for queued local sync. The important behavior is that final actual outputs sync to Laravel after recovery.

## Test 4: Cross-midnight range

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

## Test 5: Water-low safety during schedule

Force/simulate `water_low=true`.

Then apply a scheduled scene that tries to turn pump ON.

Expected result:

```text
pump remains OFF
COB/RGB may still apply
state sync reports pump OFF
water_low reading is reported
```

## Test evidence to capture

When reporting a timeline test, paste logs containing:

```text
Config HTTP status
Daily timeline enabled/repeat/range count
Processing command type=scene_apply
ACK acknowledged/executed
State HTTP status
Cloud mode ONLINE/OFFLINE
OfflineTimeline active/applying range
API recovered
final State synced
```

## Decision rule

Do not begin MQTT work until these tests pass:

```text
[ ] Online manual schedule command
[ ] Online real scheduler minute command
[ ] Offline cached range apply
[ ] Recovery final sync
[ ] Water-low safety during timeline
```
