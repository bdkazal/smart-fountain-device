#pragma once

#include <Arduino.h>

// Stores the latest valid Laravel config object in ESP32 Preferences/NVS.
//
// Important: this module stores only the inner `config` object, not the full
// /api/device/config response. The full response contains server_time_utc, which
// changes every request and would cause unnecessary flash writes.
void beginConfigCache();

String loadCachedConfigJson();
bool hasCachedConfigJson();
bool saveCachedConfigJsonIfChanged(const String &configJson);
void clearCachedConfigJson();
