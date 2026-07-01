#pragma once

// Committed local-development runtime config.
//
// This file is safe to keep in Git because it contains only non-secret runtime defaults.
// Keep real secrets in include/DeviceSecrets.h, which is ignored by Git.
//
// Current local lab values from 2026-07-01:
// - Mac/Laravel/MQTT broker LAN IP: 192.168.0.113
// - Laravel dev server port: 8000
// - MQTT broker port: 1883
//
// If your Mac IP changes, update API_BASE_URL and MQTT_HOST here.

#ifndef API_BASE_URL
// #define API_BASE_URL "http://192.168.0.113:8000"
#define API_BASE_URL "https://iot.biztola.com"
#endif

#ifndef MQTT_ENABLED
#define MQTT_ENABLED true
#endif

#ifndef MQTT_HOST
// #define MQTT_HOST "192.168.0.113"
#define MQTT_HOST "server.biztola.com"
#endif

#ifndef MQTT_PORT
#define MQTT_PORT 1883
#endif

#ifndef MQTT_USERNAME
#define MQTT_USERNAME DEVICE_UUID
#endif

#ifndef MQTT_CLIENT_ID
#define MQTT_CLIENT_ID DEVICE_UUID
#endif

#ifndef MQTT_TOPIC_PREFIX
#define MQTT_TOPIC_PREFIX "biztola/v1"
#endif

#ifndef FIRMWARE_VERSION
// #define FIRMWARE_VERSION "smart-fountain-dev-0.1"
#define FIRMWARE_VERSION "smart-fountain-live-0.1"
#endif
