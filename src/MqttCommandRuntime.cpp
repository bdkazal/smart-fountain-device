#include "MqttCommandRuntime.h"

#include <cstring>

#include "DeviceSecrets.h"

#ifndef MQTT_ENABLED
#define MQTT_ENABLED false
#endif

#ifndef MQTT_HOST
#define MQTT_HOST ""
#endif

#ifndef MQTT_PORT
#define MQTT_PORT 1883
#endif

#ifndef MQTT_USERNAME
#define MQTT_USERNAME DEVICE_UUID
#endif

#ifndef MQTT_PASSWORD
#define MQTT_PASSWORD ""
#endif

#ifndef MQTT_CLIENT_ID
#define MQTT_CLIENT_ID DEVICE_UUID
#endif

#ifndef MQTT_TOPIC_PREFIX
#define MQTT_TOPIC_PREFIX "biztola/v1"
#endif

namespace
{
  const unsigned long MQTT_RECONNECT_INTERVAL_MS = 5000;
  const int MQTT_BUFFER_BYTES = 2048;

  String normalizedTopicPrefix()
  {
    String prefix = MQTT_TOPIC_PREFIX;
    prefix.trim();

    while (prefix.startsWith("/"))
    {
      prefix.remove(0, 1);
    }

    while (prefix.endsWith("/"))
    {
      prefix.remove(prefix.length() - 1);
    }

    if (prefix.length() == 0)
    {
      prefix = "biztola/v1";
    }

    return prefix;
  }
}

MqttCommandRuntime::MqttCommandRuntime()
  : mqttClient(MQTT_BUFFER_BYTES)
{
}

bool MqttCommandRuntime::isEnabled() const
{
  return MQTT_ENABLED && strlen(MQTT_HOST) > 0;
}

bool MqttCommandRuntime::isConnected()
{
  return mqttClient.connected();
}

const String &MqttCommandRuntime::commandsTopic() const
{
  return commandTopic;
}

void MqttCommandRuntime::begin()
{
  if (!isEnabled())
  {
    Serial.println("MQTT command runtime disabled.");
    return;
  }

  if (initialized)
  {
    return;
  }

  commandTopic = normalizedTopicPrefix();
  commandTopic += "/devices/";
  commandTopic += DEVICE_UUID;
  commandTopic += "/commands";

  mqttClient.begin(MQTT_HOST, MQTT_PORT, networkClient);
  mqttClient.onMessage([this](String &topic, String &payload) {
    this->handleMessage(topic, payload);
  });

  initialized = true;

  Serial.print("MQTT command runtime ready. Broker: ");
  Serial.print(MQTT_HOST);
  Serial.print(":");
  Serial.println(MQTT_PORT);
  Serial.print("MQTT command topic: ");
  Serial.println(commandTopic);
}

void MqttCommandRuntime::loop(bool wifiConnected)
{
  if (!isEnabled())
  {
    return;
  }

  if (!initialized)
  {
    begin();
  }

  if (!wifiConnected)
  {
    disconnect();
    return;
  }

  if (!mqttClient.connected())
  {
    connectIfDue(millis());
  }

  if (mqttClient.connected())
  {
    mqttClient.loop();
  }
}

bool MqttCommandRuntime::takePendingCommand(String &topic, String &payload)
{
  if (!pendingCommand)
  {
    return false;
  }

  topic = pendingTopic;
  payload = pendingPayload;
  pendingTopic = "";
  pendingPayload = "";
  pendingCommand = false;

  return true;
}

void MqttCommandRuntime::connectIfDue(unsigned long now)
{
  if (lastConnectAttemptAt != 0 && now - lastConnectAttemptAt < MQTT_RECONNECT_INTERVAL_MS)
  {
    return;
  }

  lastConnectAttemptAt = now;
  subscribed = false;

  const char *clientId = strlen(MQTT_CLIENT_ID) > 0 ? MQTT_CLIENT_ID : DEVICE_UUID;

  Serial.print("Connecting to MQTT broker ");
  Serial.print(MQTT_HOST);
  Serial.print(":");
  Serial.println(MQTT_PORT);

  bool connected = false;

  if (strlen(MQTT_USERNAME) > 0)
  {
    connected = mqttClient.connect(clientId, MQTT_USERNAME, MQTT_PASSWORD);
  }
  else
  {
    connected = mqttClient.connect(clientId);
  }

  if (!connected)
  {
    Serial.println("MQTT connect failed. Will retry.");
    return;
  }

  Serial.println("MQTT connected.");

  subscribed = mqttClient.subscribe(commandTopic.c_str());

  if (!subscribed)
  {
    Serial.println("MQTT command topic subscribe failed. Disconnecting.");
    mqttClient.disconnect();
    return;
  }

  Serial.print("MQTT subscribed: ");
  Serial.println(commandTopic);
}

void MqttCommandRuntime::disconnect()
{
  if (!mqttClient.connected())
  {
    return;
  }

  mqttClient.disconnect();
  subscribed = false;
  Serial.println("MQTT disconnected because Wi-Fi is offline.");
}

void MqttCommandRuntime::handleMessage(String &topic, String &payload)
{
  if (topic != commandTopic)
  {
    Serial.print("MQTT ignored unexpected topic: ");
    Serial.println(topic);
    return;
  }

  if (pendingCommand)
  {
    Serial.println("MQTT command arrived while another command was pending. Replacing pending command.");
  }

  pendingTopic = topic;
  pendingPayload = payload;
  pendingCommand = true;

  Serial.print("MQTT command received. bytes=");
  Serial.println(payload.length());
}
