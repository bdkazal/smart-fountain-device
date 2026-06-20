# Firmware Architecture

This document explains the current Smart Fountain firmware architecture after the runtime refactor chain through PR #9.

## Architecture goal

The firmware should not become one giant Arduino sketch.

The goal is:

```text
FountainApp = coordinator
Runtime modules = focused behavior
Hardware modules = direct device I/O
Support modules = parsing, clock, cache, API helpers
```

## Product authority model

Smart Fountain daily timeline is now treated as a device-side timeline executor.

```text
Laravel = configuration owner, dashboard/API owner, audit/state recorder
ESP32   = hardware executor, daily timeline executor, local safety authority
```

Laravel provides config, scenes, manual/dashboard commands, and stores actual state.

ESP32 applies safe hardware state, evaluates the daily timeline, and reports actual state back.

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

It still owns high-level decisions such as:

```text
boot order
when to fetch config
when to post state
when to poll commands
when to probe API recovery
when daily timeline is evaluated/applied
when hardware should be synced
```

It should not permanently own low-level parsing, transport, button logic, or hardware-specific behavior.

## Runtime modules

| Module | Files | Responsibility |
| --- | --- | --- |
| ApiHealth | `include/ApiHealth.h`, `src/ApiHealth.cpp` | Tracks repeated API failures and server-offline mode. |
| CloudRuntime | `include/CloudRuntime.h`, `src/CloudRuntime.cpp` | Classifies API failures and logs ONLINE/OFFLINE cloud mode. |
| HttpDeviceApi | `include/HttpDeviceApi.h`, `src/HttpDeviceApi.cpp` | HTTP transport for config, state, commands, and ACKs. |
| StateSyncRuntime | `include/StateSyncRuntime.h`, `src/StateSyncRuntime.cpp` | Builds state payloads, posts state, tracks queued local/timeline sync. |
| CommandRuntime | `include/CommandRuntime.h`, `src/CommandRuntime.cpp` | Fetches commands, sends ACKs, applies output/scene command payloads. |
| ConfigRuntime | `include/ConfigRuntime.h`, `src/ConfigRuntime.cpp` | Fetches/parses config and manages compact config cache helpers. |
| LocalRuntime | `include/LocalRuntime.h`, `src/LocalRuntime.cpp` | Handles local pump/COB button decisions. |
| WifiRuntime | `include/WifiRuntime.h`, `src/WifiRuntime.cpp` | Handles Wi-Fi connection attempts and stored/development credential fallback. |
| OfflineTimeline | `include/OfflineTimeline.h`, `src/OfflineTimeline.cpp` | Tracks active cached timeline range and should evolve into the main online/offline timeline evaluator. |

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
  evaluate/apply active daily timeline range if valid
  post boot/current state
```

## Current high-level update sequence

```text
FountainApp::update()
  process local controls
  update water readings
  enforce safety
  log cloud mode changes
  render/update outputs
  if API offline: probe recovery
  evaluate daily timeline from cached/latest config
  if local/timeline sync pending: try sync when allowed
  if online: poll manual/dashboard commands
  periodically post device_state
```

## Source ownership rules

The firmware uses source labels in state payloads.

Common sources:

```text
boot
local_button
device_state
dashboard
device_timeline
offline_timeline
water_safety
```

Important rule:

```text
POST /api/device/state is the final hardware truth.
```

Dashboard/Laravel commands are requests. The actual device state becomes trusted only after firmware applies the command or timeline range and posts state.

## Boundaries to keep

Do not mix these responsibilities:

```text
HTTP transport belongs in HttpDeviceApi.
API failure policy belongs in CloudRuntime/ApiHealth.
State payload construction belongs in StateSyncRuntime.
Local button decisions belong in LocalRuntime.
Wi-Fi connection logic belongs in WifiRuntime.
Physical pin writes belong in HardwareOutputs.
Water-low pump safety belongs in firmware, especially FountainOutputs.
Daily timeline range detection/application should live in timeline runtime, not scattered through API code.
```

## Future refactor candidates

Only do these when a clear need exists:

```text
Timeline runtime: rename/expand OfflineTimeline into a general DailyTimeline runtime.
Config orchestration: move more fetch/cache/RTC boot flow out of FountainApp.
App lifecycle: split begin/update into smaller lifecycle helpers.
API responsiveness: move blocking HTTP work into shorter timeouts or FreeRTOS task.
MQTT: future feature, not current priority.
```

## Current caution

Arduino `HTTPClient` is blocking. During an active HTTP request, local buttons can pause briefly. Current mitigation keeps recovery probe timeout shorter. A full fix should be a separate architecture PR, not mixed with timeline testing.
