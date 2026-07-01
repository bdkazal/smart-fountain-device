# Adding MQTT to a New Firmware Device

Last updated: 2026-07-01

Use this document when another Biztola firmware project, such as Smart Plant Bed, needs MQTT command delivery.

## Core rule

Copy the Smart Fountain V1 pattern:

```text
Laravel creates command
-> Laravel publishes MQTT
-> device receives MQTT
-> device processes through existing command flow
-> device ACKs through REST
-> device posts actual state through REST
```

MQTT is only the fast delivery channel. REST still owns config, ACK, state sync, heartbeat, and fallback polling.

## Module pattern

Recommended firmware modules:

```text
MqttCommandRuntime
  Connects to the broker.
  Subscribes to the device command topic.
  Stores one pending raw MQTT message.

MqttCommandFlowRuntime
  Parses and validates the MQTT envelope.
  Checks schema and device UUID.
  Converts the MQTT command to the same shape used by REST polling.

CommandFlowRuntime
  Applies the command.
  Handles ACK, safety, hardware state, timeline satisfaction, and state sync.
```

Do not create a separate MQTT-only command executor.

## Topic

Use the same topic shape:

```text
biztola/v1/devices/{DEVICE_UUID}/commands
```

## Payload envelope

Expected shape:

```json
{
  "schema": "biztola.command.v1",
  "command_id": 123,
  "device_uuid": "device-uuid",
  "command_type": "output_set",
  "payload": {},
  "issued_at": "2026-07-01T20:00:00+06:00"
}
```

Validate before applying:

```text
schema matches expected version
device_uuid matches local DEVICE_UUID
command_id is present
command_type is present
payload is an object
```

## Runtime loop rule

MQTT should run alongside the existing network runtime.

Recommended priority:

```text
1. Keep Wi-Fi and API health logic active.
2. Run MQTT loop when Wi-Fi is connected.
3. If MQTT command is pending, process it through shared command flow.
4. Keep REST polling as fallback.
```

## ACK rule

A command received through MQTT must still follow the same ACK path:

```text
ACK acknowledged
apply command
ACK executed or failed
POST actual state
```

This keeps Laravel history and dashboard status consistent.

## Safety rule

MQTT must not bypass hardware safety.

Example:

```text
Smart Fountain water_low=true
-> Pump ON command arrives through MQTT
-> firmware must still keep pump OFF
```

Future devices must apply their own local safety rules even if Laravel already checked safety.

## Fallback test

Before marking a device's MQTT support complete, test:

```text
[ ] MQTT command works
[ ] REST polling command still works when MQTT is disabled or unavailable
[ ] ACKs still reach Laravel
[ ] actual state still syncs to Laravel
[ ] local safety still overrides unsafe commands
[ ] device recovers cleanly after Wi-Fi/API/MQTT outage
```

## When to use optimistic dashboard UI

Persistent-state outputs are good candidates:

```text
lights
pumps
fans
simple on/off outputs
brightness changes
```

Timed actions need more care:

```text
manual watering
one-shot dosing
timed run commands
```

For timed-action devices, keep the dashboard more conservative unless the product design explicitly chooses optimistic behavior.
