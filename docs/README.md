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
| [`API_AND_STATE_FLOW.md`](API_AND_STATE_FLOW.md) | Laravel API contract from the firmware point of view: config, commands, ACK, and state sync. |
| [`OFFLINE_RECOVERY_AND_SAFETY.md`](OFFLINE_RECOVERY_AND_SAFETY.md) | API offline mode, local controls, recovery sync, water safety, and cache rules. |
| [`DAILY_TIMELINE_OPERATION.md`](DAILY_TIMELINE_OPERATION.md) | How online Laravel schedule commands and offline cached timeline fallback are supposed to work. |
| [`DAILY_TIMELINE_TESTING.md`](DAILY_TIMELINE_TESTING.md) | Real-device testing guide for daily timeline and schedule behavior. |
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
[ ] online Laravel daily timeline scene_apply command
[ ] offline cached daily timeline range apply on real boundary time
[ ] water-low behavior during scheduled scene/timeline apply
```

## Important rules

```text
Smart Fountain = persistent state device
Laravel online schedule = scene_apply command
Firmware offline timeline = cached local fallback
Firmware safety must protect pump even without Laravel
```

Do not start MQTT work until the daily timeline behavior is verified.
