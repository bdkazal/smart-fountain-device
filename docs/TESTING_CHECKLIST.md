# Firmware Testing Checklist

Use this checklist before and after firmware changes.

## Quick build check

```bash
pio run
```

Expected:

```text
SUCCESS
```

## Upload and monitor

```bash
pio run -t upload
pio device monitor -b 115200
```

## Stable-main smoke test

### 1. Online boot

Expected:

```text
Wi-Fi connected. IP: ...
GET /api/device/config
Config HTTP status: 200
POST /api/device/state
State HTTP status: 200
State synced.
Cloud mode: ONLINE - Laravel reachable, dashboard/API control active.
```

Also check:

```text
Network/server status LED becomes solid ON.
Water safety LED is OFF when water is OK.
```

### 2. Config and timeline parse

Expected:

```text
server_time_utc: ...
DeviceClock synced from Laravel UTC.
Daily timeline enabled: yes
Daily timeline repeat: daily
Timeline range count: 3
```

### 3. Local COB button

Press COB button.

Expected:

```text
COB local button pressed.
COB state applied from local_button: enabled=...
Local output change queued for Laravel state sync.
HardwareOutputs COB GPIO26 -> ...
Syncing local button output change to Laravel...
State HTTP status: 200
State synced.
```

### 4. Local pump button

Press pump button.

Expected:

```text
Pump local button pressed.
Pump state applied from local_button: enabled=...
Local output change queued for Laravel state sync.
HardwareOutputs pump GPIO25 -> ...
State HTTP status: 200
State synced.
```

If water_low is true and pump was requested ON:

```text
Local pump button ignored because water_low=true.
```

### 5. Dashboard command

Send an output command from Laravel dashboard.

Expected:

```text
GET /api/device/commands
Command HTTP status: 200
Processing command #... type=output_set
ACK command ... as acknowledged
ACK HTTP status: 200
... state applied from dashboard ...
ACK command ... as executed
ACK HTTP status: 200
POST /api/device/state
State HTTP status: 200
State synced.
```

### 6. Status indicator LEDs

Check the physical status LEDs.

Network/server LED on GPIO23:

```text
[ ] Wi-Fi + Laravel online -> solid ON
[ ] Wi-Fi disconnected or setup portal active -> fast blink
[ ] Wi-Fi connected but Laravel/API offline -> slow blink
```

Water safety LED on GPIO13:

```text
[ ] water OK -> OFF
[ ] water low / float switch active -> solid ON
[ ] pump ON request blocked/forced OFF while water safety LED is ON
[ ] LED turns OFF after water returns OK and debounce passes
```

### 7. API offline mode

Stop Laravel.

Expected after repeated failures:

```text
API server-offline mode entered after 3 repeated failure(s).
Cloud mode: OFFLINE - API unavailable, local buttons/water safety active, keeping last trusted saved output state.
State sync skipped: API offline mode.
```

Also check:

```text
Network/server status LED slow blinks.
```

### 8. Local controls while offline

Press pump/COB buttons while Laravel is stopped.

Expected:

```text
Pump local button pressed.
... applied from local_button ...
Local output change queued for Laravel state sync.

COB local button pressed.
... applied from local_button ...
Local output change queued for Laravel state sync.
```

### 9. API recovery

Restart Laravel.

Expected:

```text
API offline mode: probing Laravel before local state sync...
Config HTTP status: 200
API recovered. request=config_probe
Cloud mode: ONLINE - Laravel reachable, dashboard/API control active.
Syncing local button output change to Laravel...
POST /api/device/state
State HTTP status: 200
State synced.
```

Also check:

```text
Network/server status LED returns to solid ON.
```

## Daily timeline test checklist

Use `docs/DAILY_TIMELINE_TESTING.md` for details.

Minimum daily timeline tests:

```text
[ ] config contains daily_timeline ranges
[ ] manual Laravel schedule command creates scene_apply
[ ] ESP32 applies scheduled scene_apply
[ ] ESP32 ACKs executed
[ ] ESP32 syncs state after scheduled scene
[ ] offline cached timeline applies range while Laravel is down
[ ] final actual state syncs after Laravel recovers
[ ] water-low blocks pump during scheduled/offline timeline apply
[ ] water safety LED turns ON during water-low timeline safety condition
```

## Regression risk areas

Be extra careful when touching:

```text
FountainApp::begin
FountainApp::update
postState
pollCommands
probeApiRecovery
fetchConfig
OfflineTimeline
FountainOutputs water safety
StatusIndicators
DeviceClock / RTC logic
```

## Before merging firmware changes

At minimum:

```text
[ ] pio run passes
[ ] online boot state sync passes
[ ] one local button sync passes
[ ] one dashboard command passes
[ ] network/server LED status passes
[ ] water safety LED status passes
```

For schedule/timeline changes:

```text
[ ] online scheduled scene command passes
[ ] offline timeline fallback passes
[ ] recovery final state sync passes
```
