# MQTT Command Runtime

Last updated: 2026-07-01

Smart Fountain V1 supports MQTT for fast dashboard command delivery.

MQTT is not the main device API. It is only the command delivery channel.

## Current decision

```text
Laravel dashboard
-> Laravel creates DeviceCommand
-> Laravel publishes MQTT command
-> ESP32 receives MQTT command
-> ESP32 processes command through shared command flow
-> ESP32 ACKs through REST
-> ESP32 posts actual state through REST
```

REST remains active and is still used for:

```text
GET /api/device/config
POST /api/device/commands/{command}/ack
POST /api/device/state
GET /api/device/commands fallback polling
POST /api/device/heartbeat
```

## Why MQTT was added

Before MQTT, dashboard commands depended on REST polling:

```text
Laravel creates command
-> ESP32 polls /api/device/commands every few seconds
-> command applies
```

With MQTT:

```text
Laravel creates command
-> Laravel publishes MQTT immediately
-> ESP32 receives command quickly
-> command applies quickly
```

This improves dashboard responsiveness. It does not remove REST.

## Topic

```text
biztola/v1/devices/{DEVICE_UUID}/commands
```

Example:

```text
biztola/v1/devices/c2e6eb95-dc09-4344-a996-bc43b3c24da5/commands
```

## Payload envelope

Laravel publishes this shape:

```json
{
  "schema": "biztola.command.v1",
  "command_id": 1076,
  "device_uuid": "c2e6eb95-dc09-4344-a996-bc43b3c24da5",
  "command_type": "output_set",
  "payload": {
    "output": "pump",
    "state": {
      "enabled": true
    },
    "source": "dashboard"
  },
  "issued_at": "2026-07-01T19:20:15+06:00"
}
```

Firmware validates:

```text
schema == biztola.command.v1
device_uuid == DEVICE_UUID
command_id exists
command_type exists
payload is an object
```

Then firmware converts this MQTT envelope into the same command shape used by REST polling.

## Firmware modules

```text
MqttCommandRuntime
```

Responsibilities:

```text
connect to broker
build device command topic
subscribe to command topic
receive raw MQTT message
store one pending message for the app loop
```

```text
MqttCommandFlowRuntime
```

Responsibilities:

```text
parse MQTT JSON
validate schema/device UUID
convert MQTT envelope to REST command shape
call CommandFlowRuntime::processCommand(...)
```

```text
CommandFlowRuntime
```

Responsibilities:

```text
ACK acknowledged
apply output_set or scene_apply
run water safety and hardware sync
ACK executed or failed
post actual state
```

Important: MQTT and REST use the same command executor. Do not duplicate command application logic.

## Config files

MQTT config is split across two files.

Committed non-secret defaults:

```text
include/DeviceRuntimeConfig.h
```

Current local lab defaults in that file:

```cpp
#define MQTT_ENABLED true
#define MQTT_HOST "192.168.0.113"
#define MQTT_PORT 1883
#define MQTT_USERNAME DEVICE_UUID
#define MQTT_CLIENT_ID DEVICE_UUID
#define MQTT_TOPIC_PREFIX "biztola/v1"
```

Local secret only:

```text
include/DeviceSecrets.h
```

The secret file should contain:

```cpp
#define MQTT_PASSWORD "YOUR_DEVICE_MQTT_SECRET"

#include "DeviceRuntimeConfig.h"
```

From ESP32, do not use `127.0.0.1` for `MQTT_HOST`. Use the Mac or broker LAN IP.

More details:

```text
docs/DEVICE_RUNTIME_CONFIG.md
```

## Expected serial messages

```text
MQTT command runtime ready. Broker: 192.168.0.113:1883
MQTT command topic: biztola/v1/devices/{DEVICE_UUID}/commands
MQTT connected.
MQTT subscribed: biztola/v1/devices/{DEVICE_UUID}/commands
MQTT command received. bytes=...
MQTT command topic: biztola/v1/devices/{DEVICE_UUID}/commands
Processing command #... type=output_set
ACK command ... as acknowledged
ACK command ... as executed
State synced.
```

## If MQTT shows disabled

If the serial monitor shows:

```text
MQTT command runtime disabled.
```

Check that `include/DeviceSecrets.h` includes the committed runtime config file:

```cpp
#include "DeviceRuntimeConfig.h"
```

Also check that `include/DeviceRuntimeConfig.h` has:

```cpp
#define MQTT_ENABLED true
```

## Safety behavior

MQTT does not bypass existing safety logic.

The command is applied through the same command flow used by REST polling, so water-low pump protection, ACK handling, actual state sync, and timeline range satisfaction still run through the existing firmware path.

## Dashboard behavior

The Laravel dashboard uses optimistic UI for Smart Fountain outputs.

That means:

```text
customer sees desired state immediately
badge shows Sent/Syncing
Laravel/device state later confirms actual state
```

Firmware does not need special UI logic. It only needs to report actual state accurately.

## Fallback behavior

REST command polling remains enabled.

If MQTT is unavailable, the device can still receive commands through the existing polling path as long as Laravel API is reachable.

Before final V1 signoff, re-test this fallback explicitly:

```text
[ ] MQTT enabled command works
[ ] MQTT unavailable but REST polling still processes output_set
[ ] ACKs still update Laravel
[ ] POST /api/device/state still confirms actual state
```

## Future-device reference

For Plant Bed or another future firmware project, start with:

```text
docs/ADDING_MQTT_TO_NEW_DEVICE.md
```
