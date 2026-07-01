#pragma once

// Copy this file to include/DeviceSecrets.h and fill in real local values.
// Do not commit include/DeviceSecrets.h.
//
// Non-secret local runtime defaults live in include/DeviceRuntimeConfig.h:
// - API_BASE_URL
// - MQTT_ENABLED
// - MQTT_HOST
// - MQTT_PORT
// - MQTT_USERNAME
// - MQTT_CLIENT_ID
// - MQTT_TOPIC_PREFIX
// - FIRMWARE_VERSION

#define WIFI_SSID "YOUR_WIFI_NAME"
#define WIFI_PASSWORD "YOUR_WIFI_SECRET"

#define DEVICE_UUID "YOUR_SMART_FOUNTAIN_DEVICE_UUID"
#define DEVICE_API_KEY "YOUR_SMART_FOUNTAIN_DEVICE_SECRET"

#define MQTT_PASSWORD "YOUR_DEVICE_MQTT_SECRET"

#include "DeviceRuntimeConfig.h"
