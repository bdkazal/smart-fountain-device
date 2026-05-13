#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#include "DeviceSecrets.h"

#include "DevFlags.h"

#ifndef FIRMWARE_VERSION
#define FIRMWARE_VERSION "smart-fountain-dev-0.1"
#endif

#ifndef SIMULATE_WATER_LOW
#define SIMULATE_WATER_LOW false
#endif

// First Smart Fountain firmware skeleton.
// Goal:
// - connect Wi-Fi
// - fetch Laravel config
// - load latest known output state from config
// - print server_time_utc and daily_timeline
// - post basic state
// - poll commands without applying hardware yet

const unsigned long CONFIG_FETCH_INTERVAL_MS = 60000;
const unsigned long STATE_SYNC_INTERVAL_MS = 5000;
const unsigned long COMMAND_POLL_INTERVAL_MS = 5000;
const unsigned long WIFI_RETRY_INTERVAL_MS = 10000;

unsigned long lastConfigFetchAt = 0;
unsigned long lastStateSyncAt = 0;
unsigned long lastCommandPollAt = 0;
unsigned long lastWifiRetryAt = 0;

bool serverReachableRecently = false;
String serverTimeUtc = "";
String deviceType = "";
String timelineRepeat = "";

struct FountainOutputState
{
  bool pumpEnabled = false;
  int pumpSpeedPercent = 0;

  bool cobEnabled = false;
  int cobBrightnessPercent = 0;

  bool rgbEnabled = false;
  int rgbBrightnessPercent = 0;
  String rgbColor = "#000000";
  String rgbEffect = "solid";
};

struct FountainReadings
{
  bool waterLow = false;
  int waterLevelPercent = 50;
  int waterLevelRaw = 0;
};

FountainOutputState outputs;
FountainReadings readings;

String apiUrl(const String &path)
{
  String base = API_BASE_URL;

  if (base.endsWith("/"))
  {
    base.remove(base.length() - 1);
  }

  return base + path;
}

void addDeviceHeaders(HTTPClient &http)
{
  http.addHeader("Accept", "application/json");
  http.addHeader("Content-Type", "application/json");
  http.addHeader("X-DEVICE-KEY", DEVICE_API_KEY);
}

bool isWifiConnected()
{
  return WiFi.status() == WL_CONNECTED;
}

void updateWaterReadings()
{
  bool simulatedWaterLow = SIMULATE_WATER_LOW;

  if (readings.waterLow != simulatedWaterLow)
  {
    Serial.print("Water-low simulation changed: ");
    Serial.println(simulatedWaterLow ? "LOW WATER" : "WATER OK");
  }

  readings.waterLow = simulatedWaterLow;

  if (readings.waterLow)
  {
    readings.waterLevelPercent = 0;
    readings.waterLevelRaw = 0;
  }
  else
  {
    readings.waterLevelPercent = 50;
    readings.waterLevelRaw = 2048;
  }
}

void connectWifi()
{
  Serial.println();
  Serial.print("Connecting Wi-Fi: ");
  Serial.println(WIFI_SSID);

  WiFi.mode(WIFI_STA);
  WiFi.setTxPower(WIFI_POWER_8_5dBm);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  unsigned long startedAt = millis();

  while (!isWifiConnected() && millis() - startedAt < 15000)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println();

  if (isWifiConnected())
  {
    Serial.print("Wi-Fi connected. IP: ");
    Serial.println(WiFi.localIP());
  }
  else
  {
    Serial.println("Wi-Fi connection failed. Device will retry later.");
  }
}

void printTimeline(JsonObject dailyTimeline)
{
  timelineRepeat = dailyTimeline["repeat"] | "";

  Serial.print("Daily timeline enabled: ");
  Serial.println((bool)(dailyTimeline["enabled"] | false) ? "yes" : "no");

  Serial.print("Daily timeline repeat: ");
  Serial.println(timelineRepeat.length() ? timelineRepeat : "missing");

  JsonArray ranges = dailyTimeline["ranges"].as<JsonArray>();

  Serial.print("Timeline range count: ");
  Serial.println(ranges.size());

  for (JsonObject range : ranges)
  {
    const char *period = range["period"] | "unknown";
    const char *startTime = range["start_time"] | "--:--";
    const char *endTime = range["end_time"] | "--:--";
    const char *sceneName = range["scene_name"] | "missing scene";

    Serial.print(" - ");
    Serial.print(period);
    Serial.print(" ");
    Serial.print(startTime);
    Serial.print(" -> ");
    Serial.print(endTime);
    Serial.print(" scene: ");
    Serial.println(sceneName);
  }
}

JsonObject outputStateObject(JsonObject outputConfig)
{
  JsonObject state = outputConfig["state"].as<JsonObject>();

  if (!state.isNull())
  {
    return state;
  }

  return outputConfig;
}

void loadInitialOutputsFromConfig(JsonObject config)
{
  JsonObject configOutputs = config["outputs"].as<JsonObject>();

  if (configOutputs.isNull())
  {
    Serial.println("No config.outputs found. Keeping safe default output state.");
    return;
  }

  JsonObject pumpOutput = configOutputs["pump"].as<JsonObject>();
  if (!pumpOutput.isNull())
  {
    JsonObject state = outputStateObject(pumpOutput);
    outputs.pumpEnabled = state["enabled"] | false;
    outputs.pumpSpeedPercent = constrain((int)(state["speed_percent"] | 0), 0, 100);

    if (!outputs.pumpEnabled)
    {
      outputs.pumpSpeedPercent = 0;
    }
  }

  JsonObject cobOutput = configOutputs["cob_light"].as<JsonObject>();
  if (!cobOutput.isNull())
  {
    JsonObject state = outputStateObject(cobOutput);
    outputs.cobEnabled = state["enabled"] | false;
    outputs.cobBrightnessPercent = constrain((int)(state["brightness_percent"] | 0), 0, 100);

    if (!outputs.cobEnabled)
    {
      outputs.cobBrightnessPercent = 0;
    }
  }

  JsonObject rgbOutput = configOutputs["rgb_light"].as<JsonObject>();
  if (!rgbOutput.isNull())
  {
    JsonObject state = outputStateObject(rgbOutput);
    outputs.rgbEnabled = state["enabled"] | false;
    outputs.rgbBrightnessPercent = constrain((int)(state["brightness_percent"] | 0), 0, 100);
    outputs.rgbColor = String((const char *)(state["color"] | outputs.rgbColor.c_str()));
    outputs.rgbEffect = String((const char *)(state["effect"] | outputs.rgbEffect.c_str()));

    if (!outputs.rgbEnabled)
    {
      outputs.rgbBrightnessPercent = 0;
    }
  }

  Serial.println("Initial output state loaded from Laravel config:");
  Serial.print(" - pump enabled=");
  Serial.print(outputs.pumpEnabled ? "true" : "false");
  Serial.print(" speed=");
  Serial.println(outputs.pumpSpeedPercent);

  Serial.print(" - cob_light enabled=");
  Serial.print(outputs.cobEnabled ? "true" : "false");
  Serial.print(" brightness=");
  Serial.println(outputs.cobBrightnessPercent);

  Serial.print(" - rgb_light enabled=");
  Serial.print(outputs.rgbEnabled ? "true" : "false");
  Serial.print(" brightness=");
  Serial.print(outputs.rgbBrightnessPercent);
  Serial.print(" color=");
  Serial.print(outputs.rgbColor);
  Serial.print(" effect=");
  Serial.println(outputs.rgbEffect);
}

bool fetchConfig()
{
  if (!isWifiConnected())
  {
    Serial.println("Config fetch skipped: Wi-Fi offline.");
    return false;
  }

  String url = apiUrl("/api/device/config?device_uuid=" + String(DEVICE_UUID));

  HTTPClient http;
  http.setTimeout(7000);
  http.begin(url);
  addDeviceHeaders(http);

  Serial.println();
  Serial.print("GET ");
  Serial.println(url);

  int statusCode = http.GET();
  String response = http.getString();
  http.end();

  Serial.print("Config HTTP status: ");
  Serial.println(statusCode);

  if (statusCode < 200 || statusCode >= 300)
  {
    Serial.println(response);
    serverReachableRecently = false;
    return false;
  }

  StaticJsonDocument<12288> doc;
  DeserializationError error = deserializeJson(doc, response);

  if (error)
  {
    Serial.print("Config JSON parse failed: ");
    Serial.println(error.c_str());
    serverReachableRecently = false;
    return false;
  }

  serverReachableRecently = true;
  serverTimeUtc = doc["server_time_utc"] | "";

  JsonObject config = doc["config"].as<JsonObject>();
  deviceType = config["device_type"] | "";

  Serial.print("server_time_utc: ");
  Serial.println(serverTimeUtc.length() ? serverTimeUtc : "missing");

  Serial.print("device_type: ");
  Serial.println(deviceType.length() ? deviceType : "missing");

  Serial.print("timezone_offset_minutes: ");
  Serial.println((int)(config["timezone_offset_minutes"] | 0));

  if (deviceType != "smart_fountain")
  {
    Serial.println("WARNING: config.device_type is not smart_fountain.");
  }

  loadInitialOutputsFromConfig(config);

  JsonObject dailyTimeline = config["daily_timeline"].as<JsonObject>();
  printTimeline(dailyTimeline);

  return true;
}

void enforceWaterSafety()
{
  if (!readings.waterLow)
  {
    return;
  }

  if (outputs.pumpEnabled || outputs.pumpSpeedPercent != 0)
  {
    Serial.println("Water safety active: forcing pump OFF.");
  }

  outputs.pumpEnabled = false;
  outputs.pumpSpeedPercent = 0;
}

String buildStatePayload(const char *source)
{
  StaticJsonDocument<2048> doc;

  doc["device_uuid"] = DEVICE_UUID;
  doc["device_type"] = "smart_fountain";
  doc["firmware_version"] = FIRMWARE_VERSION;
  doc["operation_state"] = "running";

  JsonObject outputJson = doc.createNestedObject("outputs");

  JsonObject pump = outputJson.createNestedObject("pump");
  pump["enabled"] = outputs.pumpEnabled;
  pump["speed_percent"] = outputs.pumpSpeedPercent;
  pump["source"] = readings.waterLow ? "water_safety" : source;

  JsonObject cob = outputJson.createNestedObject("cob_light");
  cob["enabled"] = outputs.cobEnabled;
  cob["brightness_percent"] = outputs.cobBrightnessPercent;
  cob["source"] = source;

  JsonObject rgb = outputJson.createNestedObject("rgb_light");
  rgb["enabled"] = outputs.rgbEnabled;
  rgb["brightness_percent"] = outputs.rgbBrightnessPercent;
  rgb["color"] = outputs.rgbColor;
  rgb["effect"] = outputs.rgbEffect;
  rgb["source"] = source;

  JsonObject readingsJson = doc.createNestedObject("readings");

  JsonObject waterLow = readingsJson.createNestedObject("water_low");
  waterLow["value"] = readings.waterLow ? 1 : 0;
  waterLow["unit"] = "boolean";

  JsonObject waterLevelPercent = readingsJson.createNestedObject("water_level_percent");
  waterLevelPercent["value"] = readings.waterLevelPercent;
  waterLevelPercent["unit"] = "percent";

  JsonObject waterLevelRaw = readingsJson.createNestedObject("water_level_raw");
  waterLevelRaw["value"] = readings.waterLevelRaw;
  waterLevelRaw["unit"] = "adc";

  String payload;
  serializeJson(doc, payload);

  return payload;
}

bool postState(const char *source = "device_state")
{
  if (!isWifiConnected())
  {
    Serial.println("State sync skipped: Wi-Fi offline.");
    return false;
  }

  updateWaterReadings();
  enforceWaterSafety();

  String url = apiUrl("/api/device/state");
  String payload = buildStatePayload(source);

  HTTPClient http;
  http.setTimeout(7000);
  http.begin(url);
  addDeviceHeaders(http);

  Serial.println();
  Serial.print("POST ");
  Serial.println(url);
  Serial.print("State payload: ");
  Serial.println(payload);

  int statusCode = http.POST(payload);
  String response = http.getString();
  http.end();

  Serial.print("State HTTP status: ");
  Serial.println(statusCode);

  if (statusCode < 200 || statusCode >= 300)
  {
    Serial.println(response);
    serverReachableRecently = false;
    return false;
  }

  serverReachableRecently = true;
  Serial.println("State synced.");
  return true;
}

bool ackCommand(int commandId, const char *status, const char *message = nullptr)
{
  if (!isWifiConnected())
  {
    Serial.println("Command ACK skipped: Wi-Fi offline.");
    return false;
  }

  String url = apiUrl("/api/device/commands/" + String(commandId) + "/ack");

  StaticJsonDocument<512> doc;
  doc["device_uuid"] = DEVICE_UUID;
  doc["status"] = status;

  if (message != nullptr)
  {
    doc["message"] = message;
  }

  String payload;
  serializeJson(doc, payload);

  HTTPClient http;
  http.setTimeout(7000);
  http.begin(url);
  addDeviceHeaders(http);

  Serial.print("ACK command ");
  Serial.print(commandId);
  Serial.print(" as ");
  Serial.println(status);

  int statusCode = http.POST(payload);
  String response = http.getString();
  http.end();

  Serial.print("ACK HTTP status: ");
  Serial.println(statusCode);

  if (statusCode < 200 || statusCode >= 300)
  {
    Serial.println(response);
    serverReachableRecently = false;
    return false;
  }

  serverReachableRecently = true;
  return true;
}

void applyPumpState(JsonObject state, const char *source)
{
  updateWaterReadings();

  bool requestedEnabled = state["enabled"] | false;
  int requestedSpeed = constrain((int)(state["speed_percent"] | 0), 0, 100);

  if (!requestedEnabled)
  {
    requestedSpeed = 0;
  }

  if (readings.waterLow && requestedEnabled)
  {
    Serial.println("Pump ON ignored because water_low=true.");
    outputs.pumpEnabled = false;
    outputs.pumpSpeedPercent = 0;
    return;
  }

  outputs.pumpEnabled = requestedEnabled;
  outputs.pumpSpeedPercent = requestedSpeed;

  Serial.print("Pump state applied from ");
  Serial.print(source);
  Serial.print(": enabled=");
  Serial.print(outputs.pumpEnabled ? "true" : "false");
  Serial.print(" speed=");
  Serial.println(outputs.pumpSpeedPercent);
}

void applyCobState(JsonObject state, const char *source)
{
  outputs.cobEnabled = state["enabled"] | false;
  outputs.cobBrightnessPercent = constrain((int)(state["brightness_percent"] | 0), 0, 100);

  if (!outputs.cobEnabled)
  {
    outputs.cobBrightnessPercent = 0;
  }

  Serial.print("COB state applied from ");
  Serial.print(source);
  Serial.print(": enabled=");
  Serial.print(outputs.cobEnabled ? "true" : "false");
  Serial.print(" brightness=");
  Serial.println(outputs.cobBrightnessPercent);
}

void applyRgbState(JsonObject state, const char *source)
{
  outputs.rgbEnabled = state["enabled"] | false;
  outputs.rgbBrightnessPercent = constrain((int)(state["brightness_percent"] | 0), 0, 100);
  outputs.rgbColor = String((const char *)(state["color"] | outputs.rgbColor.c_str()));
  outputs.rgbEffect = String((const char *)(state["effect"] | outputs.rgbEffect.c_str()));

  if (!outputs.rgbEnabled)
  {
    outputs.rgbBrightnessPercent = 0;
  }

  Serial.print("RGB state applied from ");
  Serial.print(source);
  Serial.print(": enabled=");
  Serial.print(outputs.rgbEnabled ? "true" : "false");
  Serial.print(" brightness=");
  Serial.print(outputs.rgbBrightnessPercent);
  Serial.print(" color=");
  Serial.print(outputs.rgbColor);
  Serial.print(" effect=");
  Serial.println(outputs.rgbEffect);
}

bool applyOutput(const char *outputKey, JsonObject state, const char *source)
{
  String key = outputKey;

  if (key == "pump")
  {
    applyPumpState(state, source);
    return true;
  }

  if (key == "cob_light")
  {
    applyCobState(state, source);
    return true;
  }

  if (key == "rgb_light")
  {
    applyRgbState(state, source);
    return true;
  }

  Serial.print("Unknown output key: ");
  Serial.println(key);
  return false;
}

bool handleOutputSet(JsonObject payload)
{
  const char *outputKey = payload["output"] | "";
  JsonObject state = payload["state"].as<JsonObject>();
  const char *source = payload["source"] | "dashboard";

  if (strlen(outputKey) == 0 || state.isNull())
  {
    Serial.println("Invalid output_set payload.");
    return false;
  }

  return applyOutput(outputKey, state, source);
}

bool handleSceneApply(JsonObject payload)
{
  JsonObject sceneOutputs = payload["outputs"].as<JsonObject>();
  const char *source = payload["source"] | "scene_apply";

  if (sceneOutputs.isNull())
  {
    Serial.println("Invalid scene_apply payload: outputs missing.");
    return false;
  }

  bool allApplied = true;

  for (JsonPair outputPair : sceneOutputs)
  {
    const char *outputKey = outputPair.key().c_str();
    JsonObject state = outputPair.value().as<JsonObject>();

    if (!applyOutput(outputKey, state, source))
    {
      allApplied = false;
    }
  }

  enforceWaterSafety();
  return allApplied;
}

void processCommand(JsonObject command)
{
  int commandId = command["id"] | 0;
  const char *commandType = command["command_type"] | "";
  JsonObject payload = command["payload"].as<JsonObject>();

  if (commandId <= 0 || strlen(commandType) == 0)
  {
    Serial.println("Invalid command shape. Ignoring.");
    return;
  }

  Serial.println();
  Serial.print("Processing command #");
  Serial.print(commandId);
  Serial.print(" type=");
  Serial.println(commandType);

  ackCommand(commandId, "acknowledged");

  bool applied = false;
  String type = commandType;

  if (type == "output_set")
  {
    applied = handleOutputSet(payload);
  }
  else if (type == "scene_apply")
  {
    applied = handleSceneApply(payload);
  }
  else
  {
    ackCommand(commandId, "failed", "Unsupported command type.");
    return;
  }

  if (applied)
  {
    ackCommand(commandId, "executed");
  }
  else
  {
    ackCommand(commandId, "failed", "Command could not be applied safely.");
  }

  postState("device_state");
}

bool pollCommands()
{
  if (!isWifiConnected())
  {
    Serial.println("Command poll skipped: Wi-Fi offline.");
    return false;
  }

  String url = apiUrl("/api/device/commands?device_uuid=" + String(DEVICE_UUID));

  HTTPClient http;
  http.setTimeout(7000);
  http.begin(url);
  addDeviceHeaders(http);

  Serial.println();
  Serial.print("GET ");
  Serial.println(url);

  int statusCode = http.GET();
  String response = http.getString();
  http.end();

  Serial.print("Command HTTP status: ");
  Serial.println(statusCode);

  if (statusCode < 200 || statusCode >= 300)
  {
    Serial.println(response);
    serverReachableRecently = false;
    return false;
  }

  serverReachableRecently = true;

  StaticJsonDocument<8192> doc;
  DeserializationError error = deserializeJson(doc, response);

  if (error)
  {
    Serial.print("Command JSON parse failed: ");
    Serial.println(error.c_str());
    return false;
  }

  JsonObject command = doc["command"].as<JsonObject>();

  if (command.isNull())
  {
    Serial.println("No pending command.");
    return true;
  }

  processCommand(command);
  return true;
}

void setup()
{
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("Biztola Smart Fountain ESP32 starting...");
  Serial.print("Firmware version: ");
  Serial.println(FIRMWARE_VERSION);

  updateWaterReadings();
  connectWifi();

  if (isWifiConnected())
  {
    fetchConfig();
    postState("boot");
    pollCommands();

    unsigned long now = millis();
    lastConfigFetchAt = now;
    lastStateSyncAt = now;
    lastCommandPollAt = now;
  }
}

void loop()
{
  unsigned long now = millis();

  updateWaterReadings();
  enforceWaterSafety();

  if (!isWifiConnected())
  {
    if (now - lastWifiRetryAt >= WIFI_RETRY_INTERVAL_MS)
    {
      Serial.println("Wi-Fi offline. Retrying connection...");
      connectWifi();
      lastWifiRetryAt = now;
    }

    enforceWaterSafety();
    delay(20);
    return;
  }

  if (now - lastConfigFetchAt >= CONFIG_FETCH_INTERVAL_MS)
  {
    fetchConfig();
    lastConfigFetchAt = now;
  }

  if (now - lastStateSyncAt >= STATE_SYNC_INTERVAL_MS)
  {
    postState("device_state");
    lastStateSyncAt = now;
  }

  if (now - lastCommandPollAt >= COMMAND_POLL_INTERVAL_MS)
  {
    pollCommands();
    lastCommandPollAt = now;
  }

  enforceWaterSafety();
  delay(20);
}
