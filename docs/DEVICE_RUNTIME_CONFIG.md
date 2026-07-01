# Device Runtime Config

Last updated: 2026-07-01

This document explains the config split used by the Smart Fountain firmware.

## Goal

Make local development easy without committing real secrets.

The firmware uses two config files:

```text
include/DeviceRuntimeConfig.h
include/DeviceSecrets.h
```

## Committed file

`include/DeviceRuntimeConfig.h` is committed to Git.

It contains non-secret local development defaults:

```cpp
#define API_BASE_URL "http://192.168.0.113:8000"
#define MQTT_ENABLED true
#define MQTT_HOST "192.168.0.113"
#define MQTT_PORT 1883
#define MQTT_USERNAME DEVICE_UUID
#define MQTT_CLIENT_ID DEVICE_UUID
#define MQTT_TOPIC_PREFIX "biztola/v1"
#define FIRMWARE_VERSION "smart-fountain-dev-0.1"
```

These values are safe to commit because they do not contain passwords, API keys, or Wi-Fi secrets.

If the Mac/server IP changes, update:

```text
API_BASE_URL
MQTT_HOST
```

## Ignored secret file

`include/DeviceSecrets.h` is local only and ignored by Git.

It should contain real secrets:

```cpp
#define WIFI_SSID "YOUR_WIFI_NAME"
#define WIFI_PASSWORD "YOUR_WIFI_SECRET"

#define DEVICE_UUID "YOUR_SMART_FOUNTAIN_DEVICE_UUID"
#define DEVICE_API_KEY "YOUR_SMART_FOUNTAIN_DEVICE_SECRET"

#define MQTT_PASSWORD "YOUR_DEVICE_MQTT_SECRET"

#include "DeviceRuntimeConfig.h"
```

Do not commit this file.

## Why include order matters

`DeviceSecrets.h` should include `DeviceRuntimeConfig.h` after the secret values.

Reason:

```text
MQTT_USERNAME uses DEVICE_UUID
MQTT_CLIENT_ID uses DEVICE_UUID
```

So `DEVICE_UUID` must exist before those runtime defaults are used.

## Common mistake

If serial monitor shows:

```text
MQTT command runtime disabled.
```

Check:

```cpp
#include "DeviceRuntimeConfig.h"
```

at the bottom of `include/DeviceSecrets.h`.

Also check that `DeviceRuntimeConfig.h` has:

```cpp
#define MQTT_ENABLED true
```

## LAN IP rule

From ESP32, do not use:

```text
127.0.0.1
localhost
```

Use the Mac/server/broker LAN IP, for example:

```text
192.168.0.113
```

## Future device rule

Future firmware projects should use the same split:

```text
committed runtime config = non-secret defaults
ignored secrets file     = Wi-Fi, API key, MQTT secret
```
