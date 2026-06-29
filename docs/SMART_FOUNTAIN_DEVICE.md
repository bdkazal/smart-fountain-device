# Biztola Smart Fountain Device Firmware

This document describes the ESP32 firmware for the Biztola Smart Fountain device.

## Product model

Smart Fountain is a persistent-state IoT device.

Persistent state means:

```text
A requested output state continues until another event changes it.
```

Examples:

```text
Pump ON stays ON until dashboard, schedule, local button, water safety, or recovery logic changes it.
COB ON stays ON until changed.
RGB effect continues until changed.
```

This is different from a timed-action product such as the Smart Plant Bed.

## Backend repository

```text
bdkazal/biztola-iot-platform
```

Laravel owns:

```text
device identity
claim/owner records
output state records
scenes
daily timeline ranges
commands
ACK records
history
customer dashboard
```

Firmware owns:

```text
safe boot
hardware output state
local buttons
status indicator LEDs
water-low safety
config cache
API offline mode
recovery sync
offline timeline fallback
```

## Device identity

The device authenticates to Laravel using:

```http
X-DEVICE-KEY: <device_api_key>
```

The device identifies itself with:

```json
{
  "device_uuid": "...",
  "device_type": "smart_fountain"
}
```

The current development firmware version string is:

```text
smart-fountain-dev-0.1
```

## Main outputs

| Output | API key | Firmware behavior |
| --- | --- | --- |
| Pump | `pump` | On/off pump state, forced OFF when water_low is true |
| COB light | `cob_light` | On/off light state |
| RGB light | `rgb_light` | Enabled, brightness, color, and effect |

Current V1 pump and COB behavior is intentionally simple ON/OFF. Speed and brightness fields may exist in API payloads, but the current local button model treats pump and COB as simple toggles.

## Runtime overview

Normal boot:

```text
1. Start firmware
2. Initialize storage
3. Initialize output GPIO/NeoPixel
4. Initialize status indicator LEDs
5. Force outputs OFF for safe hardware boot
6. Mark state untrusted so safe boot OFF is not synced to Laravel
7. Initialize water-low sensor
8. Initialize local buttons
9. Initialize Wi-Fi reset button
10. Load RTC time if available
11. Load cached Laravel config if available
12. Apply trusted cached output state
13. Connect Wi-Fi
14. Fetch fresh Laravel config when available
15. Sync DeviceClock from server_time_utc when available
16. Update RTC if needed
17. Parse outputs, scenes, and daily_timeline
18. Save compact cache if changed
19. POST actual state to Laravel when API is available
20. Start background Network/API task
21. Enter main hardware runtime loop
```

Main hardware runtime loop:

```text
local button processing
water-low safety enforcement
hardware output apply
status indicator LED update
NeoPixel/effect rendering
offline cached timeline check
setup portal handling when active
```

Background Network/API task:

```text
Wi-Fi retry
API recovery probing
queued local state sync
regular state sync
command polling
command ACK
config refresh
cloud/API mode tracking
```

The main loop must stay responsive even when HTTPS requests are slow. Network/API work is handled in a FreeRTOS task so remote production requests do not freeze buttons or RGB animation.

## Online mode

When Laravel is reachable:

```text
Cloud mode: ONLINE - Laravel reachable, dashboard/API control active.
```

Online behavior:

```text
Dashboard/API commands are the live source of truth.
ESP32 polls pending commands in the Network/API task.
ESP32 applies commands locally.
ESP32 ACKs command state.
ESP32 POSTs actual hardware state.
Local button changes are applied immediately and queued for state sync.
```

## API offline mode

When repeated API failures happen:

```text
Cloud mode: OFFLINE - API unavailable, local buttons/water safety active, keeping last trusted saved output state.
```

Offline behavior:

```text
local pump button still works
local COB button still works
water-low safety still works
RGB animation continues from current/cached state
state sync is skipped while API is offline
final local output state is queued
firmware periodically probes Laravel recovery
when Laravel returns, final actual state syncs
```

## Wi-Fi offline mode

When Wi-Fi is unavailable:

```text
Cloud mode: OFFLINE - Wi-Fi disconnected, local buttons/water safety active, keeping last trusted saved output state.
```

Expected behavior:

```text
cached Laravel config loads from flash
last trusted output state is restored
local pump and COB buttons work
water-low safety remains active
RGB animation continues from the cached/current state
state sync remains queued until Wi-Fi/API recovery
```

The current Wi-Fi retry path can still spend noticeable time attempting stored credentials and DeviceSecrets fallback credentials. Local controls are serviced during retry, but future work should reduce remaining blocking/retry pressure.

## Controller GPIO layout

Current ESP32 DevKit V1 / ESP32-WROOM-32 wiring groups external fountain wiring on the left side and local controller-box wiring on the right side.

```text
Fountain/RJ45/external-load side:
GPIO25 = pump MOSFET control
GPIO26 = COB MOSFET control
GPIO27 = RGB / NeoPixel data
GPIO32 = float switch input

Controller-box local side:
GPIO19 = pump local button
GPIO18 = COB local button
GPIO23 = Wi-Fi reset/setup button
GPIO17 = network/server status LED
GPIO16 = water safety status LED
GPIO21 = RTC SDA
GPIO22 = RTC SCL
```

## Local buttons

Local buttons are hardware controls independent of Laravel availability.

| Button | GPIO | Action |
| --- | --- | --- |
| Pump button | GPIO19 | Toggle pump ON/OFF, unless water_low prevents pump ON |
| COB button | GPIO18 | Toggle COB ON/OFF |
| Wi-Fi reset/setup button | GPIO23 | Hold 3 seconds to clear saved Wi-Fi and start setup hotspot |

Local button changes use source:

```text
local_button
```

When Laravel is online, local changes are synced by the Network/API task. When Laravel is offline, the final state is queued and synced after recovery.

## Status indicators

| Indicator | GPIO | Behavior |
| --- | --- | --- |
| Network/server LED | GPIO17 | Fast blink for Wi-Fi/setup, slow blink for API offline, solid ON when online |
| Water safety LED | GPIO16 | ON when water_low is true / pump is protected |

## Water-low safety

Pump safety is a firmware responsibility, not only a Laravel rule.

If `water_low=true`:

```text
pump OFF
pump speed 0
pump ON commands ignored locally
COB and RGB may still work
state payload reports pump OFF
```

This must work even when:

```text
Wi-Fi is unavailable
Laravel is stopped
router is down
internet is down
```

## Daily timeline

Smart Fountain uses a daily timeline with three V1 blocks:

```text
Day
Evening
Night
```

Each range has:

```text
period
name
start_time
end_time
scene_id
scene_name
outputs
```

Example:

```text
day      06:00 -> 18:00  scene: Day Fountain
evening  18:00 -> 23:00  scene: Display Mode
night    23:00 -> 06:00  scene: Night Glow
```

The night range crosses midnight. Firmware treats cross-midnight ranges as active when:

```text
local_time >= start_time OR local_time < end_time
```
