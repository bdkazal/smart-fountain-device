# API and State Flow

Last updated: 2026-07-01

This document explains how the Smart Fountain firmware communicates with Laravel.

## Backend repository

```text
bdkazal/biztola-iot-platform
```

## Current transport model

Smart Fountain uses both REST and MQTT.

```text
REST = config, fallback commands, ACK, state sync, heartbeat
MQTT = fast dashboard command delivery
```

MQTT does not replace REST.

## Authentication

Every firmware REST request must include:

```http
X-DEVICE-KEY: <device_api_key>
Accept: application/json
Content-Type: application/json
```

The device identifies itself using:

```json
{
  "device_uuid": "...",
  "device_type": "smart_fountain"
}
```

## Main REST endpoints

```http
GET  /api/device/config
GET  /api/device/commands
POST /api/device/commands/{command}/ack
POST /api/device/state
POST /api/device/heartbeat
```

For Smart Fountain, `/api/device/state` is the important endpoint because it reports actual output state and readings.

## MQTT command topic

```text
biztola/v1/devices/{DEVICE_UUID}/commands
```

MQTT command payloads use this envelope:

```json
{
  "schema": "biztola.command.v1",
  "command_id": 1076,
  "device_uuid": "...",
  "command_type": "output_set",
  "payload": {},
  "issued_at": "2026-07-01T19:20:15+06:00"
}
```

Firmware validates the envelope and converts it into the same command shape used by REST polling.

## Config fetch

```http
GET /api/device/config?device_uuid=<uuid>
```

Laravel returns:

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
update RTC when needed
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

## Command delivery

There are now two command delivery paths.

Fast path:

```text
Laravel publishes MQTT
-> ESP32 receives command topic
-> ESP32 validates payload
-> ESP32 processes command
```

Fallback path:

```http
GET /api/device/commands?device_uuid=<uuid>
```

No pending command response:

```json
{
  "message": "No pending commands.",
  "command": null
}
```

Supported command types:

```text
output_set
scene_apply
```

## Command lifecycle

When command exists:

```text
1. ESP32 receives command through MQTT or REST polling.
2. ESP32 ACKs acknowledged through REST.
3. ESP32 applies command locally.
4. ESP32 ACKs executed or failed through REST.
5. ESP32 POSTs actual state through REST.
```

For persistent-state devices:

```text
executed = requested state was applied
```

It does not mean the output finished running.

## output_set

Example REST command shape after MQTT envelope conversion:

```json
{
  "id": 1076,
  "command_type": "output_set",
  "payload": {
    "output": "rgb_light",
    "state": {
      "enabled": true,
      "brightness_percent": 25,
      "color": "#CD0404",
      "effect": "slow_rainbow"
    },
    "source": "dashboard"
  }
}
```

## scene_apply

Example:

```json
{
  "command_type": "scene_apply",
  "payload": {
    "scene_id": 1,
    "scene_name": "Day Fountain",
    "source": "scene:1",
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
}
```

Schedule-created scene commands also use `scene_apply`, with extra metadata:

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

## ACK endpoint

```http
POST /api/device/commands/{command}/ack
```

ACK acknowledged:

```json
{
  "device_uuid": "...",
  "status": "acknowledged"
}
```

ACK executed:

```json
{
  "device_uuid": "...",
  "status": "executed"
}
```

ACK failed:

```json
{
  "device_uuid": "...",
  "status": "failed",
  "message": "Command could not be applied safely."
}
```

## State sync

```http
POST /api/device/state
```

State sync reports actual firmware/hardware truth.

Typical payload:

```json
{
  "device_uuid": "...",
  "device_type": "smart_fountain",
  "firmware_version": "smart-fountain-dev-0.1",
  "operation_state": "running",
  "outputs": {
    "pump": {
      "enabled": true,
      "source": "device_state"
    },
    "cob_light": {
      "enabled": false,
      "source": "device_state"
    },
    "rgb_light": {
      "enabled": true,
      "brightness_percent": 25,
      "color": "#CD0404",
      "effect": "slow_rainbow",
      "source": "device_state"
    }
  },
  "readings": {
    "water_low": {
      "value": 0,
      "unit": "boolean"
    }
  }
}
```

## Source values

Common output source values:

```text
boot
device_state
local_button
dashboard
offline_timeline
water_safety
```

## Trust rule

The firmware starts with outputs forced OFF for safety, but that state is untrusted.

Untrusted safe boot OFF must not be synced to Laravel.

State becomes trusted after:

```text
cached config loaded
fresh Laravel config fetched
cloud command applied
local button changed actual hardware state
offline timeline applied cached range
```

## Failure handling

Failures are classified by `CloudRuntime` and tracked by `ApiHealth`.

Repeated failures for these requests can enter API offline mode:

```text
commands
state
config
```

Parse warnings and less critical warnings are recorded without immediately forcing offline mode.

## Final rule

Laravel may request state, but the firmware reports actual state.

```text
Command payload = request
ACK executed = request applied
POST /api/device/state = actual hardware truth
```

This is still true when the command arrived through MQTT.
