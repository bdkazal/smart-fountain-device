# Biztola Smart Fountain Device Firmware

ESP32 firmware for the Biztola Smart Fountain product.

This repository contains the device-side firmware that talks to the Laravel Biztola IoT Platform backend.

Backend repository:

```text
bdkazal/biztola-iot-platform
```

## Current firmware status

First skeleton stage:

```text
[x] PlatformIO project
[x] Wi-Fi connection
[x] GET /api/device/config
[x] print server_time_utc
[x] print Smart Fountain daily_timeline ranges
[x] POST /api/device/state
[x] GET /api/device/commands
[x] ACK command as acknowledged
[x] ACK command as executed/failed
[x] handle output_set in local state
[x] handle scene_apply in local state
[x] local water_low pump safety placeholder
[ ] hardware pump pin
[ ] hardware COB PWM pin
[ ] hardware RGB module/pins
[ ] real water-level sensor pin/calibration
[ ] cached config storage
[ ] offline daily timeline fallback
[ ] RTC UTC restore
```

## Local setup

Copy the example secrets file:

```bash
cp include/DeviceSecrets.example.h include/DeviceSecrets.h
```

Edit `include/DeviceSecrets.h` with your local values:

```cpp
#define WIFI_SSID "YOUR_WIFI_NAME"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"
#define API_BASE_URL "http://192.168.0.xxx:8000"
#define DEVICE_UUID "YOUR_SMART_FOUNTAIN_DEVICE_UUID"
#define DEVICE_API_KEY "YOUR_SMART_FOUNTAIN_DEVICE_API_KEY"
```

Do not use `127.0.0.1` for `API_BASE_URL` from ESP32. Use the Mac/Laravel server LAN IP.

Run Laravel backend like this:

```bash
php artisan serve --host=0.0.0.0 --port=8000
```

## Build and upload

From this repository:

```bash
pio run
pio run --target upload
pio device monitor
```

Or use PlatformIO inside VS Code.

## Expected first boot behavior

Serial monitor should show:

```text
Biztola Smart Fountain ESP32 starting...
Wi-Fi connected
GET /api/device/config
server_time_utc: ...
device_type: smart_fountain
Daily timeline repeat: daily
POST /api/device/state
GET /api/device/commands
```

## Safety rule

The pump must be protected locally.

If `water_low=true`:

```text
pump OFF
speed_percent = 0
pump ON commands ignored locally
lights may still work
```

Current skeleton has the safety logic placeholder. Real water sensor hardware/calibration will be added later.
