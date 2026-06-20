# Smart Fountain Device Documentation

This directory contains the firmware documentation for the **Biztola Smart Fountain ESP32 device**.

The Laravel platform documentation uses `Docs/` in the Laravel repo. This firmware repository uses lowercase `docs/`.

## Documentation map

| File | Purpose |
| --- | --- |
| [`SMART_FOUNTAIN_DEVICE.md`](SMART_FOUNTAIN_DEVICE.md) | Full product/device overview, firmware responsibilities, runtime flow, and current status. |
| [`ARCHITECTURE.md`](ARCHITECTURE.md) | Firmware module architecture and responsibility boundaries. |
| [`SETUP_AND_FLASHING.md`](SETUP_AND_FLASHING.md) | Local setup, secrets, build, upload, and serial monitor workflow. |
| [`HARDWARE_PINS.md`](HARDWARE_PINS.md) | GPIO pin map, hardware roles, and current hardware assumptions. |
| [`API_AND_STATE_FLOW.md`](API_AND_STATE_FLOW.md) | Laravel config, dashboard/manual commands, firmware timeline execution, ACK, and state sync. |
| [`OFFLINE_RECOVERY_AND_SAFETY.md`](OFFLINE_RECOVERY_AND_SAFETY.md) | API offline mode, local controls, recovery sync, water safety, and cache rules. |
| [`DAILY_TIMELINE_OPERATION.md`](DAILY_TIMELINE_OPERATION.md) | How ESP32 executes the daily timeline online/offline from Laravel config. |
| [`DAILY_TIMELINE_TESTING.md`](DAILY_TIMELINE_TESTING.md) | Real-device testing guide for device-side daily timeline behavior. |
| [`TESTING_CHECKLIST.md`](TESTING_CHECKLIST.md) | Smoke-test checklists before and after firmware changes. |

## Current stable firmware base

The firmware is stable after the runtime refactor chain through PR #9.

Verified on device:

```text
[x] boot config fetch
[x] cached config restore
[x] boot state sync
[x] dashboard output_set commands
[x] command ACK acknowledged/executed
[x] local pump button
[x] local COB button
[x] local button state sync
[x] API offline mode
[x] API recovery probe
[x] final local state sync after recovery
[x] water-low safety path kept in firmware
```

Main behavior still needing focused testing:

```text
[ ] ESP32 applies active daily_timeline range while online
[ ] ESP32 does not re-apply the same range repeatedly
[ ] ESP32 applies next range at a real boundary time
[ ] ESP32 applies cached range while Laravel/API is offline
[ ] ESP32 syncs final actual state after recovery
[ ] water-low behavior during manual command and timeline apply
```

## Important rules

```text
Smart Fountain = persistent state device
Laravel = configuration/dashboard/state recorder
ESP32 = hardware executor + online/offline daily timeline executor
Firmware safety must protect pump even without Laravel
Laravel schedule command = compatibility/backstop test path only
```

Do not start MQTT work until the daily timeline behavior is verified.
