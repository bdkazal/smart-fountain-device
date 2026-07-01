# MQTT Command Runtime

Smart Fountain V1 supports MQTT for dashboard commands.

REST remains active and is still used for:

```text
GET /api/device/config
POST /api/device/commands/{command}/ack
POST /api/device/state
GET /api/device/commands fallback polling
```

## Command direction

```text
Laravel dashboard
-> Laravel creates DeviceCommand
-> Laravel publishes MQTT command
-> ESP32 receives command topic
-> ESP32 validates payload
-> ESP32 ACKs acknowledged through REST
-> ESP32 applies command locally
-> ESP32 ACKs executed or failed through REST
-> ESP32 posts actual state through REST
```

## Topic

```text
biztola/v1/devices/{DEVICE_UUID}/commands
```

## Config files

MQTT config is split across two files.

Committed non-secret defaults:

```text
include/DeviceRuntimeConfig.h
```

Current local lab defaults in that file:

```cpp
#define MQTT_ENABLED true
#define MQTT_HOST "192.168.0.113"
#define MQTT_PORT 1883
#define MQTT_USERNAME DEVICE_UUID
#define MQTT_CLIENT_ID DEVICE_UUID
#define MQTT_TOPIC_PREFIX "biztola/v1"
```

Local secret only:

```text
include/DeviceSecrets.h
```

The secret file should contain:

```cpp
#define MQTT_PASSWORD "YOUR_DEVICE_MQTT_SECRET"

#include "DeviceRuntimeConfig.h"
```

From ESP32, do not use `127.0.0.1` for `MQTT_HOST`. Use the Mac or broker LAN IP.

## Expected serial messages

```text
MQTT command runtime ready. Broker: 192.168.0.113:1883
MQTT command topic: biztola/v1/devices/{DEVICE_UUID}/commands
MQTT connected.
MQTT subscribed: biztola/v1/devices/{DEVICE_UUID}/commands
MQTT command received. bytes=...
MQTT command topic: biztola/v1/devices/{DEVICE_UUID}/commands
Processing command #... type=output_set
ACK command ... as acknowledged
ACK command ... as executed
State synced.
```

## If MQTT shows disabled

If the serial monitor shows:

```text
MQTT command runtime disabled.
```

Check that `include/DeviceSecrets.h` includes the committed runtime config file:

```cpp
#include "DeviceRuntimeConfig.h"
```

Also check that `include/DeviceRuntimeConfig.h` has:

```cpp
#define MQTT_ENABLED true
```

## Safety behavior

MQTT does not bypass existing safety logic.

The command is applied through the same command flow used by REST polling, so water-low pump protection, ACK handling, actual state sync, and timeline range satisfaction still run through the existing firmware path.

## Fallback behavior

REST command polling remains enabled. If MQTT is unavailable, the device can still receive commands through the existing polling path as long as Laravel API is reachable.
