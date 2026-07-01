#pragma once

#include <Arduino.h>
#include <MQTT.h>
#include <WiFi.h>

class MqttCommandRuntime
{
public:
  MqttCommandRuntime();

  void begin();
  void loop(bool wifiConnected);
  bool takePendingCommand(String &topic, String &payload);
  bool isEnabled() const;
  bool isConnected();
  const String &commandsTopic() const;

private:
  WiFiClient networkClient;
  MQTTClient mqttClient;
  bool initialized = false;
  bool subscribed = false;
  bool pendingCommand = false;
  unsigned long lastConnectAttemptAt = 0;
  String commandTopic = "";
  String pendingTopic = "";
  String pendingPayload = "";

  void connectIfDue(unsigned long now);
  void disconnect();
  void handleMessage(String &topic, String &payload);
};
