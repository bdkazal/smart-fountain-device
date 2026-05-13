#include "ConfigCache.h"

#include <Preferences.h>

namespace
{
Preferences preferences;

// Separate namespace from Plant Bed so future devices can coexist safely if code
// is reused or tested on the same ESP32 during development.
static const char *NAMESPACE = "fountain";
static const char *KEY_CONFIG_JSON = "cfg_json";

// ESP32 NVS strings are not suitable for large JSON documents. Keep a safety
// margin below the practical 4000-byte string limit, including null terminator.
static const size_t MAX_SAFE_CONFIG_JSON_BYTES = 3800;
}

void beginConfigCache()
{
  preferences.begin(NAMESPACE, false);
}

String loadCachedConfigJson()
{
  if (!preferences.isKey(KEY_CONFIG_JSON))
  {
    return "";
  }

  return preferences.getString(KEY_CONFIG_JSON, "");
}

bool hasCachedConfigJson()
{
  return loadCachedConfigJson().length() > 0;
}

bool saveCachedConfigJsonIfChanged(const String &configJson)
{
  if (configJson.length() == 0)
  {
    Serial.println("Cannot cache config: JSON is empty.");
    return false;
  }

  if (configJson.length() >= MAX_SAFE_CONFIG_JSON_BYTES)
  {
    Serial.print("Config cache skipped: JSON too large for NVS. length=");
    Serial.println(configJson.length());
    return false;
  }

  String existingConfigJson = loadCachedConfigJson();

  if (existingConfigJson == configJson)
  {
    Serial.println("Config cache unchanged. Flash write skipped.");
    return false;
  }

  size_t written = preferences.putString(KEY_CONFIG_JSON, configJson);

  if (written == 0)
  {
    Serial.println("Config cache write failed.");
    return false;
  }

  Serial.print("Config cache saved to flash. bytes=");
  Serial.println(written);

  return true;
}

void clearCachedConfigJson()
{
  preferences.remove(KEY_CONFIG_JSON);
  Serial.println("Cached Laravel config cleared.");
}
