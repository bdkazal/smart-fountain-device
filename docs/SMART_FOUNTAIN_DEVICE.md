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
13. Fetch fresh Laravel config when available
14. Sync DeviceClock from server_time_utc when available
15. Update RTC if needed
16. Parse outputs, scenes, and daily_timeline
17. Save compact cache if changed
18. POST actual state to Laravel when API is available
19. Start background Network/API task
20. Enter main hardware runtime loop
```

Main hardware runtime loop:

```text
local button processing
water-low safety enforcement
hardware output apply
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

When Laravel is online, local changes are synced by the Network/API task. When Laravel is offline, the final state is queued and synced after recovery.

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

There are two timeline behaviors:

```text
Online: Laravel may create scheduled scene_apply commands.
Offline: ESP32 applies cached range outputs locally.
```

Important offline rule:

```text
Offline timeline uses the last cached Laravel config from flash.
```

So if the server scene/timeline is changed, the device must connect at least once and fetch the updated config before that new timeline can be used offline.

## Setup portal mode

Setup portal mode is a special maintenance/setup state, not normal fountain operation.

Current V1 behavior:

```text
setup request active
cached config is not loaded
normal daily timeline is not run
outputs are forced OFF for safety
setup hotspot starts
local pump/COB buttons can still work
safe boot OFF is not synced to Laravel
```

This is intentionally conservative. Normal scene RGB behavior is not required during setup mode.

Future optional improvement:

```text
Add setup-mode RGB indicator effect.
```

Goal:

```text
show a simple setup/hotspot indicator, such as slow blue breathing
avoid loading normal scene/timeline during setup mode
keep pump OFF by default
keep setup mode safe and visually understandable
```

## Production response issue and fix

### Issue found

During production testing against:

```text
https://iot.biztola.com
```

local button response and NeoPixel effects were poor compared with the local LAN Laravel server.

Symptoms:

```text
NeoPixel effects looked choppy or only changed around command/state activity
local pump/COB buttons sometimes required repeated pressing
pump/COB dashboard commands still worked
```

### Cause

The original runtime handled Laravel/API work inside the same main loop that handled local buttons, safety, and RGB rendering.

Production HTTPS requests are slower than local LAN HTTP requests. A blocking config fetch, command poll, command ACK, or state POST could pause the main loop long enough to make local buttons and NeoPixel animation feel unresponsive.

### Fix applied

Network/API work was moved into a dedicated FreeRTOS task.

Main loop now handles:

```text
local buttons
water safety
hardware output apply
RGB animation
setup portal handling
offline timeline evaluation
```

Network/API task handles:

```text
Wi-Fi retry
API offline probe
config fetch
command poll
command ACK
state sync
queued local-state recovery sync
```

Commit:

```text
3274340 Move API runtime into network task
```

### Verified result

Production testing showed:

```text
local pump button works
local COB button works
dashboard pump/COB commands work
state sync works
command ACK works
NeoPixel remains smoother while HTTPS requests run
API offline mode works
Wi-Fi offline mode keeps local controls active
cached config is restored after reboot
```

## Future improvement goals

### 1. Setup-mode RGB indicator effect

Add a simple visual state for setup portal mode.

Target behavior:

```text
setup portal active -> RGB slow blue breathing/pulse
normal cached scene/timeline still disabled
pump stays OFF by default
COB stays OFF by default unless local button toggles it
```

Reason:

```text
Customer/technician can see that the device is in setup/hotspot mode.
```

This is not required for normal online/offline runtime.

### 2. Reduce blocking Wi-Fi retry behavior

Current retry works, but Wi-Fi connection attempts can still be long and noisy.

Future goal:

```text
make Wi-Fi retry less blocking
avoid long repeated connect attempts
keep local button/RGB response fully smooth during Wi-Fi reconnect attempts
retry less aggressively after repeated failures
keep setup portal separate from normal retry flow
```

Possible approach:

```text
state-machine Wi-Fi retry
shorter connection windows
backoff after repeated failures
avoid repeated DeviceSecrets fallback attempts in production builds
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
[x] dashboard pump command
[x] dashboard COB command
[x] command ACK acknowledged/executed
[x] local pump button
[x] local COB button
[x] local button state sync
[x] background Network/API task
[x] production HTTPS responsiveness improvement
[x] API offline mode
[x] API recovery probe
[x] Wi-Fi offline mode local controls
[x] cached config restore after reboot
[x] queued local state while offline
```

Needs focused testing:

```text
[ ] online daily timeline command creation from Laravel scheduler
[ ] firmware applying scheduled scene_apply command
[ ] offline cached timeline apply at exact range boundary
[ ] water-low safety during scheduled/offline timeline apply
[ ] recovery sync after real production outage, not only wrong/offline test URL
[ ] setup-mode RGB indicator effect, optional future UX improvement
[ ] reduced blocking Wi-Fi retry behavior, optional future robustness improvement
```
