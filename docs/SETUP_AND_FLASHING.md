# Setup, Build, Upload, and Monitor

This document explains how to set up and run the Smart Fountain ESP32 firmware locally.

## Requirements

Install:

```text
VS Code
PlatformIO extension
Git
USB driver for your ESP32 board if macOS does not detect it automatically
```

This firmware is built with PlatformIO.

## Clone repository

```bash
git clone https://github.com/bdkazal/smart-fountain-device.git
cd smart-fountain-device
```

## Secrets file

Copy the example secrets file:

```bash
cp include/DeviceSecrets.example.h include/DeviceSecrets.h
```

Edit:

```text
include/DeviceSecrets.h
```

Required values:

```cpp
#define WIFI_SSID "YOUR_WIFI_NAME"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"
#define API_BASE_URL "http://192.168.0.xxx:8000"
#define DEVICE_UUID "YOUR_SMART_FOUNTAIN_DEVICE_UUID"
#define DEVICE_API_KEY "YOUR_SMART_FOUNTAIN_DEVICE_API_KEY"
```

Important:

```text
Do not use 127.0.0.1 for API_BASE_URL.
The ESP32 must use the Laravel server LAN IP.
```

Correct example:

```cpp
#define API_BASE_URL "http://192.168.0.113:8000"
```

Wrong example:

```cpp
#define API_BASE_URL "http://127.0.0.1:8000"
```

## Laravel server

From the Laravel repo, run:

```bash
php artisan serve --host=0.0.0.0 --port=8000
```

The ESP32 must be on the same network and must be able to reach the Mac/server LAN IP.

## Build

```bash
pio run
```

## Upload

```bash
pio run -t upload
```

## Serial monitor

```bash
pio device monitor -b 115200
```

If monitor is already open during upload, close it first.

## Normal online boot checklist

Expected log sections:

```text
Biztola Smart Fountain ESP32 starting...
Device storage initialized.
HardwareOutputs ... ready
WaterLevelSensor ... ready
Pump local button initialized
COB local button initialized
All outputs forced OFF: safe boot hardware default
Output state untrusted...
Loading cached Laravel config from flash. bytes=...
Trying stored Wi-Fi credentials...
Wi-Fi connected. IP: ...
GET /api/device/config
Config HTTP status: 200
server_time_utc: ...
DeviceClock synced from Laravel UTC.
Daily timeline enabled: yes
Timeline range count: 3
POST /api/device/state
State HTTP status: 200
State synced.
Cloud mode: ONLINE - Laravel reachable, dashboard/API control active.
```

## First boot with empty cache

Expected:

```text
No cached Laravel config found.
Output state untrusted: no cached Laravel config; safe boot OFF must not sync to Laravel
GET /api/device/config
Config HTTP status: 200
Config cache saved to flash. bytes=...
POST /api/device/state
State HTTP status: 200
State synced.
```

## Wi-Fi setup/reset behavior

The firmware has a Wi-Fi reset button.

Current pin:

```text
GPIO33
```

Expected boot message:

```text
Hold Wi-Fi reset button to GND for 3 seconds to clear saved Wi-Fi and start setup hotspot.
```

## Useful PlatformIO commands

```bash
pio run
pio run -t upload
pio device monitor -b 115200
pio run -t clean
```

## Common issues

### Config HTTP status is -1

Check:

```text
Laravel server is running
API_BASE_URL uses LAN IP
ESP32 and Mac/server are on same Wi-Fi/LAN
firewall is not blocking port 8000
DEVICE_UUID is correct
DEVICE_API_KEY is correct
```

### HTTP 401

Usually means:

```text
missing X-DEVICE-KEY
wrong DEVICE_API_KEY
wrong DEVICE_UUID/API key pair
```

### HTTP 409

Usually means posted `device_type` does not match Laravel's stored device type.

Expected device type:

```text
smart_fountain
```

### Local buttons seem delayed

The firmware uses Arduino `HTTPClient`, which is blocking. During an active HTTP request, local button response can pause briefly. This is known and should be fixed later in a separate responsiveness task.

## Before changing firmware

Run:

```bash
pio run
```

Then after changes:

```bash
pio run -t upload
pio device monitor -b 115200
```

Use `docs/TESTING_CHECKLIST.md` for the full smoke test.
