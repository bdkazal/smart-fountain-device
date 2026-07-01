#pragma once

// Copy this file to include/DeviceSecrets.h and fill in real local values.
// Do not commit include/DeviceSecrets.h.

#define WIFI_SSID "YOUR_WIFI_NAME"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"

// From ESP32, do not use 127.0.0.1.
// Use your Mac/Laravel server LAN IP, for example: http://192.168.0.105:8000
#define API_BASE_URL "http://192.168.0.xxx:8000"

#define DEVICE_UUID "YOUR_SMART_FOUNTAIN_DEVICE_UUID"
#define DEVICE_API_KEY "YOUR_SMART_FOUNTAIN_DEVICE_API_KEY"

// MQTT command channel. Keep disabled until your broker, device user, secret, and ACL are ready.
// From ESP32, MQTT_HOST must also be a LAN IP, not 127.0.0.1.
#define MQTT_ENABLED false
#define MQTT_HOST "192.168.0.xxx"
#define MQTT_PORT 1883
#define MQTT_USERNAME DEVICE_UUID
#define MQTT_PASSWORD "YOUR_DEVICE_MQTT_SECRET"
#define MQTT_CLIENT_ID DEVICE_UUID
#define MQTT_TOPIC_PREFIX "biztola/v1"

#define FIRMWARE_VERSION "smart-fountain-dev-0.1"
