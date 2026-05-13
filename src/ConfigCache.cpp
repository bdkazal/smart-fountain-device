#include "ConfigCache.h"

#include <Preferences.h>

namespace
{
Preferences preferences;

// Separate namespace from Plant Bed so future devices can coexist safely if code
// is reused or tested on the same ESP32 during development.
static const char *NAMESPACE = "fountain";
static const char *KEY_CONFIG_JSON = "cfg_json";
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

  String existingConfigJson = loadCachedConfigJson();

  if (existingConfigJson == configJson)
  {
    Serial.println("Config cache unchanged. Flash write skipped.");
    return false;
  }

  preferences.putString(KEY_CONFIG_JSON, configJson);
  Serial.println("Config cache saved to flash.");

  return true;
}

void clearCachedConfigJson()
{
  preferences.remove(KEY_CONFIG_JSON);
  Serial.println("Cached Laravel config cleared.");
}
