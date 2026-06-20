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
4. Force outputs OFF for safe hardware boot
5. Mark state untrusted so safe boot OFF is not synced to Laravel
6. Initialize water-low sensor
7. Initialize local buttons
8. Initialize Wi-Fi reset button
9. Load RTC time if available
10. Load cached Laravel config if available
11. Apply trusted cached output state
12. Connect Wi-Fi
13. Fetch fresh Laravel config
14. Sync DeviceClock from server_time_utc
15. Update RTC if needed
16. Parse outputs, scenes, and daily_timeline
17. Save compact cache if changed
18. POST actual state to Laravel
19. Enter runtime loop
```

Runtime loop:

```text
local button processing
water-low safety enforcement
cloud/API mode tracking
state sync
command polling
offline recovery probing
offline cached timeline check
NeoPixel/effect rendering
```

## Online mode

When Laravel is reachable:

```text
Cloud mode: ONLINE - Laravel reachable, dashboard/API control active.
```

Online behavior:

```text
Dashboard/API commands are the live source of truth.
ESP32 polls pending commands.
ESP32 applies commands locally.
ESP32 ACKs command state.
ESP32 POSTs actual hardware state.
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
state sync is skipped while API is offline
final local output state is queued
firmware periodically probes Laravel recovery
when Laravel returns, final actual state syncs
```

## Local buttons

Local buttons are hardware controls independent of Laravel availability.

| Button | GPIO | Action |
| --- | --- | --- |
| Pump button | GPIO18 | Toggle pump ON/OFF, unless water_low prevents pump ON |
| COB button | GPIO19 | Toggle COB ON/OFF |

Local button changes use source:

```text
local_button
```

When Laravel is online, local changes are synced immediately. When Laravel is offline, the final state is queued and synced after recovery.

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
evening  18:00 -> 23:00  scene: Night Glow
night    23:00 -> 06:00  scene: All Off
```

The night range crosses midnight. Firmware treats cross-midnight ranges as active when:

```text
local_time >= start_time OR local_time < end_time
```

There are two timeline behaviors:

```text
Online: Laravel creates scheduled scene_apply commands.
Offline: ESP32 applies cached range outputs locally.
```

## Current verification status

Verified:

```text
[x] build succeeds
[x] Wi-Fi stored credentials boot path
[x] cached config load
[x] fresh config fetch
[x] DeviceClock server UTC sync
[x] RTC drift check/update path
[x] boot state sync
[x] dashboard RGB command
[x] command ACK acknowledged/executed
[x] local pump button
[x] local COB button
[x] local button state sync
[x] API offline mode
[x] API recovery probe
[x] queued local state sync after recovery
```

Needs focused testing:

```text
[ ] online daily timeline command creation from Laravel scheduler
[ ] firmware applying scheduled scene_apply command
[ ] offline cached timeline apply at range boundary
[ ] water-low safety during scheduled/offline timeline apply
```
