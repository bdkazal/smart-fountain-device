#pragma once

#include <Arduino.h>

// ESP32 Preferences/NVS is good for small key-value data, not large documents.
// This module stores a compact offline cache, not the full Laravel config.
// The full /api/device/config response includes fields that change often or are
// useful only for the dashboard, so firmware should cache only what it needs.
void beginConfigCache();

String loadCachedConfigJson();
bool hasCachedConfigJson();
bool saveCachedConfigJsonIfChanged(const String &configJson);
void clearCachedConfigJson();
