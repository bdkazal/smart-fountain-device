#include "SetupPortal.h"

#include <Arduino.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <WiFi.h>

#include "DeviceStorage.h"
#include "SetupPortalPage.h"
#include "WifiConfig.h"

namespace
{
WebServer setupServer(80);
DNSServer dnsServer;

bool setupPortalActive = false;

String networksJson = "{\"networks\":[]}";
String pendingSsid = "";
String pendingPassword = "";
String setupMessage = "Waiting for Wi-Fi details.";

unsigned long credentialTestStartedAt = 0;
unsigned long restartAt = 0;

enum class SetupScanState
{
  Idle,
  Scanning,
  Ready,
  Failed
};

enum class SetupCredentialState
{
  Idle,
  Connecting,
  Failed,
  Success
};

SetupScanState scanState = SetupScanState::Idle;
SetupCredentialState credentialState = SetupCredentialState::Idle;

String jsonEscape(const String &value)
{
  String escaped = value;
  escaped.replace("\\", "\\\\");
  escaped.replace("\"", "\\\"");
  escaped.replace("\n", "\\n");
  escaped.replace("\r", "\\r");
  return escaped;
}

const char *credentialStateName()
{
  switch (credentialState)
  {
  case SetupCredentialState::Idle:
    return "idle";
  case SetupCredentialState::Connecting:
    return "connecting";
  case SetupCredentialState::Failed:
    return "failed";
  case SetupCredentialState::Success:
    return "success";
  }

  return "unknown";
}

void handleRoot()
{
  setupServer.send_P(200, "text/html", SetupPortalPage);
}

String buildNetworksJson(int networkCount)
{
  String json = "{\"status\":\"ready\",\"networks\":[";
  bool first = true;

  for (int i = 0; i < networkCount; i++)
  {
    const String ssid = WiFi.SSID(i);

    if (ssid.length() == 0)
    {
      continue;
    }

    bool duplicate = false;

    for (int previous = 0; previous < i; previous++)
    {
      if (WiFi.SSID(previous) == ssid)
      {
        duplicate = true;
        break;
      }
    }

    if (duplicate)
    {
      continue;
    }

    if (!first)
    {
      json += ',';
    }

    json += "{\"ssid\":\"";
    json += jsonEscape(ssid);
    json += "\",\"rssi\":";
    json += String(WiFi.RSSI(i));
    json += '}';
    first = false;
  }

  json += "]}";
  return json;
}

void startAsyncScan()
{
  WiFi.scanDelete();
  scanState = SetupScanState::Scanning;
  networksJson = "{\"networks\":[]}";

  const int result = WiFi.scanNetworks(true, true);

  if (result == WIFI_SCAN_FAILED)
  {
    scanState = SetupScanState::Failed;
    Serial.println("Wi-Fi scan could not start.");
    return;
  }

  Serial.println("Asynchronous Wi-Fi scan started.");
}

void updateAsyncScan()
{
  if (scanState != SetupScanState::Scanning)
  {
    return;
  }

  const int result = WiFi.scanComplete();

  if (result == WIFI_SCAN_RUNNING)
  {
    return;
  }

  if (result == WIFI_SCAN_FAILED)
  {
    scanState = SetupScanState::Failed;
    networksJson = "{\"networks\":[]}";
    Serial.println("Wi-Fi scan failed.");
    return;
  }

  networksJson = buildNetworksJson(result);
  scanState = SetupScanState::Ready;
  WiFi.scanDelete();

  Serial.print("Wi-Fi scan completed. Networks found: ");
  Serial.println(result);
}

void handleNetworks()
{
  if (setupServer.hasArg("refresh"))
  {
    startAsyncScan();
    setupServer.send(202, "application/json", "{\"status\":\"scanning\"}");
    return;
  }

  if (scanState == SetupScanState::Idle || scanState == SetupScanState::Failed)
  {
    startAsyncScan();
  }

  if (scanState == SetupScanState::Scanning)
  {
    setupServer.send(202, "application/json", "{\"status\":\"scanning\"}");
    return;
  }

  setupServer.send(200, "application/json", networksJson);
}

void startCredentialTest(const String &ssid, const String &password)
{
  pendingSsid = ssid;
  pendingPassword = password;
  credentialState = SetupCredentialState::Connecting;
  setupMessage = "Testing Wi-Fi credentials.";
  credentialTestStartedAt = millis();
  restartAt = 0;

  WiFi.mode(WIFI_AP_STA);
  WiFi.disconnect(false, false);
  WiFi.begin(pendingSsid.c_str(), pendingPassword.c_str());

  Serial.println();
  Serial.print("Testing submitted Wi-Fi SSID: ");
  Serial.println(pendingSsid);
}

void failCredentialTest(const String &message)
{
  credentialState = SetupCredentialState::Failed;
  setupMessage = message;
  pendingPassword = "";

  WiFi.disconnect(false, false);
  WiFi.mode(WIFI_AP_STA);

  Serial.println(message);
}

void updateCredentialTest()
{
  if (credentialState != SetupCredentialState::Connecting)
  {
    return;
  }

  if (WiFi.status() == WL_CONNECTED)
  {
    if (!saveWifiCredentials(pendingSsid, pendingPassword))
    {
      failCredentialTest("Wi-Fi connected, but credentials could not be saved.");
      return;
    }

    credentialState = SetupCredentialState::Success;
    setupMessage = "Wi-Fi saved successfully. Device is restarting.";
    restartAt = millis() + WifiConfig::PortalRestartDelayMs;
    pendingPassword = "";

    Serial.println("Submitted Wi-Fi credentials worked and were saved.");
    Serial.print("Temporary station IP: ");
    Serial.println(WiFi.localIP());
    return;
  }

  if (millis() - credentialTestStartedAt >= WifiConfig::CredentialTestTimeoutMs)
  {
    failCredentialTest("Could not connect. Check the SSID/password and try again.");
  }
}

void handleSave()
{
  if (credentialState == SetupCredentialState::Connecting)
  {
    setupServer.send(409, "application/json", "{\"ok\":false,\"message\":\"A Wi-Fi test is already running.\"}");
    return;
  }

  String ssid = setupServer.arg("ssid");
  String password = setupServer.arg("password");
  ssid.trim();

  if (ssid.length() == 0 || ssid.length() > 32)
  {
    setupServer.send(400, "application/json", "{\"ok\":false,\"message\":\"Enter a valid Wi-Fi SSID.\"}");
    return;
  }

  if (password.length() > 63)
  {
    setupServer.send(400, "application/json", "{\"ok\":false,\"message\":\"Wi-Fi password is too long.\"}");
    return;
  }

  startCredentialTest(ssid, password);
  setupServer.send(202, "application/json", "{\"ok\":true,\"status\":\"connecting\"}");
}

String setupStatusJson()
{
  String json = "{\"status\":\"";
  json += credentialStateName();
  json += "\",\"message\":\"";
  json += jsonEscape(setupMessage);
  json += "\"}";
  return json;
}

void handleSetupStatus()
{
  setupServer.send(200, "application/json", setupStatusJson());
}

void handleCaptivePortalRedirect()
{
  String location = "http://";
  location += WiFi.softAPIP().toString();
  location += "/";

  setupServer.sendHeader("Location", location, true);
  setupServer.send(302, "text/plain", "");
}

void registerRoutes()
{
  setupServer.on("/", HTTP_GET, handleRoot);
  setupServer.on("/networks", HTTP_GET, handleNetworks);
  setupServer.on("/save", HTTP_POST, handleSave);
  setupServer.on("/setup-status", HTTP_GET, handleSetupStatus);

  setupServer.on("/generate_204", HTTP_GET, handleCaptivePortalRedirect);
  setupServer.on("/hotspot-detect.html", HTTP_GET, handleCaptivePortalRedirect);
  setupServer.on("/ncsi.txt", HTTP_GET, handleCaptivePortalRedirect);
  setupServer.on("/connecttest.txt", HTTP_GET, handleCaptivePortalRedirect);
  setupServer.on("/favicon.ico", HTTP_GET, []()
                 { setupServer.send(204, "text/plain", ""); });
  setupServer.onNotFound(handleCaptivePortalRedirect);
}
} // namespace

void startSetupPortal()
{
  if (setupPortalActive)
  {
    return;
  }

  Serial.println();
  Serial.println("Starting setup portal...");

  WiFi.persistent(false);
  WiFi.setAutoReconnect(false);
  WiFi.disconnect(false, false);
  WiFi.softAPdisconnect(true);
  delay(100);
  WiFi.mode(WIFI_AP_STA);
  delay(100);

  const bool apStarted = WiFi.softAP(
      WifiConfig::SetupApSsid,
      WifiConfig::SetupApPassword,
      WifiConfig::SetupApChannel,
      WifiConfig::SetupApHidden,
      WifiConfig::SetupApMaxConnections);

  if (!apStarted)
  {
    Serial.println("Setup portal failed: hotspot could not start.");
    setupPortalActive = false;
    return;
  }

  const IPAddress portalIp = WiFi.softAPIP();
  dnsServer.start(WifiConfig::DnsPort, "*", portalIp);

  registerRoutes();
  setupServer.begin();

  credentialState = SetupCredentialState::Idle;
  scanState = SetupScanState::Idle;
  setupMessage = "Waiting for Wi-Fi details.";
  restartAt = 0;
  setupPortalActive = true;

  Serial.println();
  Serial.println("Wi-Fi setup portal started.");
  Serial.print("Setup hotspot SSID: ");
  Serial.println(WifiConfig::SetupApSsid);
  Serial.print("Setup hotspot password: ");
  Serial.println(WifiConfig::SetupApPassword);
  Serial.print("Setup hotspot channel: ");
  Serial.println(WifiConfig::SetupApChannel);
  Serial.print("Setup portal IP: ");
  Serial.println(portalIp);
  Serial.println("Captive DNS redirect enabled.");
}

void handleSetupPortal()
{
  if (!setupPortalActive)
  {
    return;
  }

  dnsServer.processNextRequest();
  setupServer.handleClient();
  updateAsyncScan();
  updateCredentialTest();

  if (restartAt > 0 && millis() >= restartAt)
  {
    Serial.println("Restarting with saved Wi-Fi credentials...");
    delay(100);
    ESP.restart();
  }
}

bool isSetupPortalActive()
{
  return setupPortalActive;
}

const char *setupPortalCredentialStateName()
{
  return credentialStateName();
}
