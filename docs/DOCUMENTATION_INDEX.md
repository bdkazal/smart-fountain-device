# Documentation Index

Last updated: 2026-07-01

Use this file as the quick entry point for the Smart Fountain firmware project.

## Start here

```text
docs/SMART_FOUNTAIN_DEVICE.md
docs/ARCHITECTURE.md
docs/SETUP_AND_FLASHING.md
docs/API_AND_STATE_FLOW.md
```

## MQTT command support

```text
docs/MQTT_COMMANDS.md
docs/DEVICE_RUNTIME_CONFIG.md
docs/ADDING_MQTT_TO_NEW_DEVICE.md
```

Current rule:

```text
Laravel creates command.
Laravel publishes MQTT.
ESP32 receives MQTT.
ESP32 processes through shared CommandFlowRuntime.
ESP32 ACKs and syncs actual state through REST.
REST command polling remains fallback.
```

## Offline, safety, and timeline

```text
docs/OFFLINE_RECOVERY_AND_SAFETY.md
docs/DAILY_TIMELINE_OPERATION.md
docs/DAILY_TIMELINE_TESTING.md
docs/STATUS_INDICATORS.md
docs/HARDWARE_PINS.md
```

## Current status

```text
MQTT command subscriber: working
Dashboard output commands through MQTT: working
REST config/ACK/state sync: still required
REST fallback after MQTT work: needs explicit re-test
Daily timeline end-to-end test: still pending
Controller GPIO move verification: still pending
```

## Future firmware projects

When adding MQTT to another firmware project, start with:

```text
docs/ADDING_MQTT_TO_NEW_DEVICE.md
```

Do not create a separate MQTT-only command executor. Use the same command application path used by REST polling.
