# Offline Recovery and Safety

This document explains how the Smart Fountain firmware behaves when Laravel/API is unavailable.

## Core principle

The fountain must remain safe and locally controllable even when Laravel is down.

```text
Water safety does not depend on Laravel.
Local buttons do not depend on Laravel.
Actual final state is synced after Laravel recovers.
```

## Cloud modes

### Online

```text
Cloud mode: ONLINE - Laravel reachable, dashboard/API control active.
```

Online behavior:

```text
state sync works
command polling works
dashboard commands work
local buttons sync immediately
Laravel schedules create scene_apply commands
```

### API offline

```text
Cloud mode: OFFLINE - API unavailable, local buttons/water safety active, keeping last trusted saved output state.
```

API offline behavior:

```text
state sync skipped
command polling reduced/stopped
local buttons still work
water safety still works
local output changes queue for later sync
firmware probes config endpoint for recovery
```

### Wi-Fi offline

```text
Cloud mode: OFFLINE - Wi-Fi disconnected, local buttons/water safety active, keeping last trusted saved output state.
```

Wi-Fi offline behavior is similar, but API recovery cannot happen until Wi-Fi returns.

## API failure tracking

`ApiHealth` tracks repeated failures.

Requests that count toward server-offline mode:

```text
commands
state
config
```

After repeated failures, firmware enters API offline mode.

Typical log:

```text
API server-offline mode entered after 3 repeated failure(s). request=commands status=-1
```

## Recovery probe

While API is offline, firmware probes Laravel using config fetch.

Typical log:

```text
API offline mode: probing Laravel...
GET /api/device/config?device_uuid=...
Config HTTP status: -1
API offline probe failed. request=config status=-1
```

When Laravel returns:

```text
Config HTTP status: 200
API recovered. request=config_probe
Cloud mode: ONLINE - Laravel reachable, dashboard/API control active.
```

## Local button recovery sync

When local buttons change output state while API is offline, firmware queues a local state sync.

Example offline changes:

```text
COB local button pressed.
COB state applied from local_button: enabled=true mode=on_off
Local output change queued for Laravel state sync.

Pump local button pressed.
Pump state applied from local_button: enabled=false mode=on_off
Local output change queued for Laravel state sync.
```

When API recovers:

```text
API offline mode: probing Laravel before local state sync...
Config HTTP status: 200
API recovered. request=config_probe
Syncing local button output change to Laravel...
POST /api/device/state
State HTTP status: 200
State synced.
```

Only the final actual output state matters. Multiple local button presses while offline collapse into the latest real state.

## Config cache

The firmware stores a compact config cache in ESP32 Preferences/NVS.

The cache is used to:

```text
restore last trusted output state after reboot
load daily timeline ranges
load timezone offset
run offline timeline fallback
avoid syncing unsafe boot OFF to Laravel
```

The cache does not store the full Laravel response.

It stores only firmware-needed data:

```text
device_type
timezone_offset_minutes
outputs
compact daily_timeline ranges
range scene output states
```

## Safe boot rule

At boot, outputs are forced OFF first.

This is a hardware safety default only.

```text
safe boot OFF is untrusted
safe boot OFF must not be pushed to Laravel
```

State becomes trusted after cached or fresh config loads.

## Water-low safety

Water-low safety is local and immediate.

If water is low:

```text
pump forced OFF
pump ON ignored
pump speed 0
COB/RGB allowed
state payload reports water_low
```

This applies during:

```text
manual local button control
dashboard output_set command
scene_apply command
Laravel schedule command
offline cached timeline apply
```

## Offline timeline fallback

If API is offline and a valid cached daily timeline exists, firmware can apply the active range locally.

Important:

```text
OfflineTimeline does not apply cached range while Laravel is online.
```

This prevents dashboard commands and cached schedule outputs from fighting.

## Blocking HTTP limitation

Current firmware uses Arduino `HTTPClient`, which blocks during requests.

Known effect:

```text
local button response can pause while an HTTP request is waiting
```

Current mitigation:

```text
normal HTTP timeout remains suitable for LAN/remote hosting
API recovery probe timeout is shorter
```

Future fix:

```text
reduce timeouts carefully
or move API work into a FreeRTOS task
```

Do not mix this future responsiveness work with timeline testing.

## Verified behavior from stable main

The following has been verified after runtime refactors:

```text
[x] API offline mode enters after repeated failures
[x] state sync skipped while API offline
[x] local pump/COB buttons work while offline
[x] final local state queues while offline
[x] API recovery probe works
[x] queued final local state syncs after recovery
[x] dashboard commands still work after recovery
```
