# API and State Flow

This document explains how the Smart Fountain firmware communicates with Laravel.

## Backend repository

```text
bdkazal/biztola-iot-platform
```

## Authority split

```text
Laravel = configuration owner, dashboard/API owner, audit/state recorder
ESP32   = hardware executor, daily timeline executor, local safety authority
```

Laravel provides config and manual/dashboard commands.

Firmware applies safe hardware state, evaluates the daily timeline, and reports actual state back.

## Main endpoints

```http
GET  /api/device/config
GET  /api/device/commands
POST /api/device/commands/{command}/ack
POST /api/device/state
POST /api/device/heartbeat
```

For Smart Fountain, `/api/device/state` is the important endpoint because it reports actual output state and readings.

## Config fetch

```http
GET /api/device/config?device_uuid=<uuid>
```

Laravel returns the firmware-facing config:

```text
server_time_utc
server_time_local
config.device_uuid
config.device_type
config.timezone
config.timezone_offset_minutes
config.outputs
config.scenes
config.daily_timeline
config.safety
config.commands
```

Firmware uses config to:

```text
sync DeviceClock from server_time_utc
load latest server-known outputs
load scenes
load daily timeline ranges
save compact cache
```

## Time rules

```text
server_time_utc = canonical time for firmware and RTC
server_time_local = display/log only
timezone_offset_minutes = local schedule interpretation
RTC stores UTC, not local time
```

## Daily timeline flow

Daily timeline execution is primarily firmware-side.

Online flow:

```text
ESP32 fetches config
ESP32 syncs time
ESP32 caches daily_timeline
ESP32 determines active local range
ESP32 applies active range once if needed
ESP32 POSTs actual state
```

Offline flow:

```text
cached daily_timeline exists
DeviceClock has valid time
Laravel/API becomes unavailable
ESP32 determines active local range from cache
ESP32 applies active range once if needed
ESP32 queues state sync
Laravel/API recovers
ESP32 POSTs final actual state
```

The Laravel schedule command may still create `scene_apply` commands during compatibility/backstop testing, but normal production timeline execution should come from local evaluation of cached `daily_timeline` config.

## Command polling

```http
GET /api/device/commands?device_uuid=<uuid>
```

Supported command types:

```text
output_set
scene_apply
```

Commands are mainly for dashboard/manual actions and compatibility/backstop schedule testing.

## Command lifecycle

When command exists:

```text
1. ESP32 receives command.
2. ESP32 ACKs acknowledged.
3. ESP32 applies command locally if safe.
4. ESP32 ACKs executed or failed.
5. ESP32 POSTs actual state.
```

For persistent-state devices:

```text
executed = requested state was applied
```

It does not mean the output finished running.

## scene_apply compatibility

Schedule-created scene commands may still exist during migration/backstop testing.

They use `scene_apply`, with extra metadata such as:

```text
source = schedule:<range_id>:<period>
schedule_range_id
schedule_name
schedule_period
schedule_phase = start
repeat = daily
```

Firmware should support them, but the daily recurring timeline should not depend on them.

## State sync

```http
POST /api/device/state
```

State sync reports actual firmware/hardware truth.

Typical output sources:

```text
boot
device_state
local_button
dashboard
device_timeline
offline_timeline
water_safety
```

`device_timeline` is recommended for normal firmware-side timeline application. Existing code may still use `offline_timeline` for cached/offline application until source naming is refactored.

## Trust rule

The firmware starts with outputs forced OFF for safety, but that state is untrusted.

Untrusted safe boot OFF must not be synced to Laravel.

State becomes trusted after:

```text
cached config loaded
fresh Laravel config fetched
cloud command applied
local button changed actual hardware state
firmware timeline applied cached range
offline timeline applied cached range
```

## Final rule

Laravel may request or configure state, but firmware reports actual state.

```text
Config payload = configured timeline/scenes
Command payload = request
ACK executed = request applied
POST /api/device/state = actual hardware truth
```
