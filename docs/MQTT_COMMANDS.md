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

## Device settings

Add these to local `include/DeviceSecrets.h` after copying the example file:

```cpp
#define MQTT_ENABLED true
#define MQTT_HOST "YOUR_BROKER_LAN_IP"
#define MQTT_PORT 1883
#define MQTT_USERNAME DEVICE_UUID
#define MQTT_PASSWORD "YOUR_DEVICE_MQTT_SECRET"
#define MQTT_CLIENT_ID DEVICE_UUID
#define MQTT_TOPIC_PREFIX "biztola/v1"
```

From ESP32, do not use `127.0.0.1` for `MQTT_HOST`. Use the Mac or broker LAN IP.

## Expected serial messages

```text
MQTT command runtime ready. Broker: YOUR_BROKER_LAN_IP:1883
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

## Safety behavior

MQTT does not bypass existing safety logic.

The command is applied through the same command flow used by REST polling, so water-low pump protection, ACK handling, actual state sync, and timeline range satisfaction still run through the existing firmware path.

## Fallback behavior

REST command polling remains enabled. If MQTT is unavailable, the device can still receive commands through the existing polling path as long as Laravel API is reachable.
