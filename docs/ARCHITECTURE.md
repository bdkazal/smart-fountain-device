# Firmware Architecture

Last updated: 2026-07-01

This document explains the current Smart Fountain firmware architecture after the runtime refactor chain and MQTT command subscriber work.

## Architecture goal

The firmware should not become one giant Arduino sketch.

The goal is:

```text
FountainApp = coordinator
Runtime modules = focused behavior
Hardware modules = direct device I/O
Support modules = parsing, clock, cache, API helpers
```

## Main entry point

```text
src/main.cpp
```

`main.cpp` should stay small. It should initialize and call `FountainApp`.

## Coordinator

```text
include/FountainApp.h
src/FountainApp.cpp
```

`FountainApp` coordinates boot and runtime flow.

It owns high-level decisions such as:

```text
boot order
when to fetch config
when to post state
when to run MQTT loop
when to poll REST commands
when to probe API recovery
when offline timeline is allowed to apply
when hardware should be synced
```

It should not permanently own low-level parsing, transport, button logic, or hardware-specific behavior.

## Runtime modules

| Module | Files | Responsibility |
| --- | --- | --- |
| ApiHealth | `include/ApiHealth.h`, `src/ApiHealth.cpp` | Tracks repeated API failures and server-offline mode. |
| CloudRuntime | `include/CloudRuntime.h`, `src/CloudRuntime.cpp` | Classifies API failures and logs ONLINE/OFFLINE cloud mode. |
| HttpDeviceApi | `include/HttpDeviceApi.h`, `src/HttpDeviceApi.cpp` | HTTP transport for config, state, commands, and ACKs. |
| MqttCommandRuntime | `include/MqttCommandRuntime.h`, `src/MqttCommandRuntime.cpp` | MQTT broker connection, command topic subscription, and pending raw message storage. |
| MqttCommandFlowRuntime | `include/MqttCommandFlowRuntime.h`, `src/MqttCommandFlowRuntime.cpp` | Parses MQTT command envelope, validates schema/device UUID, and forwards to shared command flow. |
| StateSyncRuntime | `include/StateSyncRuntime.h`, `src/StateSyncRuntime.cpp` | Builds state payloads, posts state, tracks queued local sync. |
| CommandRuntime | `include/CommandRuntime.h`, `src/CommandRuntime.cpp` | Fetches REST commands, sends ACKs, applies output/scene command payloads. |
| CommandFlowRuntime | `include/CommandFlowRuntime.h`, `src/CommandFlowRuntime.cpp` | Shared command execution flow used by REST polling and MQTT commands. |
| ConfigRuntime | `include/ConfigRuntime.h`, `src/ConfigRuntime.cpp` | Fetches/parses config and manages compact config cache helpers. |
| LocalRuntime | `include/LocalRuntime.h`, `src/LocalRuntime.cpp` | Handles local pump/COB button decisions. |
| WifiRuntime | `include/WifiRuntime.h`, `src/WifiRuntime.cpp` | Handles Wi-Fi connection attempts and stored/development credential fallback. |
| OfflineTimeline | `include/OfflineTimeline.h`, `src/OfflineTimeline.cpp` | Tracks active cached timeline range and applies cached outputs while offline. |

## MQTT boundary

MQTT is only a delivery path.

```text
MqttCommandRuntime receives raw MQTT message.
MqttCommandFlowRuntime validates and adapts it.
CommandFlowRuntime applies it.
```

Do not duplicate output or scene application logic for MQTT.

REST polling and MQTT commands must both reach the same command execution path so ACK, state sync, timeline satisfaction, and water safety remain consistent.

## Product modules

| Module | Responsibility |
| --- | --- |
| FountainOutputs | Applies pump/COB/RGB state safely and enforces water-low pump protection. |
| FountainConfig | Parses Laravel Smart Fountain config into firmware structs. |
| FountainTypes | Shared structs for outputs, readings, scenes, and timeline ranges. |

## Hardware modules

| Module | Responsibility |
| --- | --- |
| HardwareOutputs | GPIO/NeoPixel output rendering. |
| WaterLevelSensor | Water-low switch/readings. |
| LocalControls | Debounced local pump/COB button inputs. |
| WifiReset | Wi-Fi reset button and setup portal trigger. |
| DeviceClock | UTC/local time, server time parsing, millis-based clock. |
| DeviceStorage | Stored Wi-Fi/device setup data. |
| ConfigCache | Compact cached Laravel config in ESP32 Preferences/NVS. |

## External integration modules

| Module | Responsibility |
| --- | --- |
| ApiClient | Base URL normalization and common API request setup. |
| SetupPortal | Wi-Fi setup portal behavior. |
| DeviceRtc | DS3231/HW-111 RTC integration, if available. |

## Current high-level boot sequence

```text
FountainApp::begin()
  initialize storage
  initialize outputs
  initialize sensors/buttons
  force safe boot OFF
  load RTC time
  load cached config
  connect Wi-Fi
  fetch fresh config
  sync server time / RTC
  save compact config cache
  initialize MQTT command runtime
  post boot state
```

## Current high-level update sequence

```text
FountainApp::update()
  process local controls
  update water readings
  enforce safety
  log cloud mode changes
  render/update outputs
  if Wi-Fi connected: run MQTT loop
  if MQTT command pending and API usable: process through shared command flow
  if API offline: probe recovery
  if local sync pending: try sync when allowed
  if online: poll REST commands as fallback
  periodically post device_state
  check offline timeline if cloud/API unavailable
```

## Source ownership rules

The firmware uses source labels in state payloads.

Common sources:

```text
boot
local_button
device_state
dashboard
offline_timeline
water_safety
```

Important rule:

```text
POST /api/device/state is the final hardware truth.
```

Dashboard/Laravel commands are requests. The actual device state becomes trusted only after firmware applies the command and posts state.

## Boundaries to keep

Do not mix these responsibilities:

```text
HTTP transport belongs in HttpDeviceApi.
MQTT transport belongs in MqttCommandRuntime.
MQTT envelope parsing belongs in MqttCommandFlowRuntime.
Command execution belongs in CommandFlowRuntime.
API failure policy belongs in CloudRuntime/ApiHealth.
State payload construction belongs in StateSyncRuntime.
Local button decisions belong in LocalRuntime.
Wi-Fi connection logic belongs in WifiRuntime.
Physical pin writes belong in HardwareOutputs.
Water-low pump safety belongs in firmware, especially FountainOutputs.
```

## Future refactor candidates

Only do these when a clear need exists:

```text
Config orchestration: move more fetch/cache/RTC boot flow out of FountainApp.
App lifecycle: split begin/update into smaller lifecycle helpers.
API responsiveness: move blocking HTTP work into shorter timeouts or FreeRTOS task.
MQTT queueing: replace one pending MQTT message with a small queue if real use needs it.
Presence: optionally add MQTT presence/last-will later.
```

## Current caution

Arduino `HTTPClient` is blocking. During an active HTTP request, local buttons can pause briefly. Current mitigation keeps recovery probe timeout shorter. A full fix should be a separate architecture PR, not mixed with timeline or MQTT documentation work.
