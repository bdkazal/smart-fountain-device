#include "SetupPortal.h"

#include <Arduino.h>
#include <WebServer.h>
#include <WiFi.h>

#include "DeviceStorage.h"

static const char *SETUP_AP_SSID = "Fountain-Setup";
static const char *SETUP_AP_PASSWORD = "fountain123";
static const int SETUP_AP_CHANNEL = 6;
static const int SETUP_AP_MAX_CONNECTIONS = 4;
static const bool SETUP_AP_HIDDEN = false;

WebServer setupServer(80);
bool setupPortalActive = false;

String jsonEscape(const String &value)
{
  String escaped = value;
  escaped.replace("\\", "\\\\");
  escaped.replace("\"", "\\\"");
  escaped.replace("\n", "\\n");
  escaped.replace("\r", "\\r");
  return escaped;
}

String setupPageHtml()
{
  return R"rawliteral(
<!DOCTYPE html><html><head><meta name="viewport" content="width=device-width, initial-scale=1"><title>Smart Fountain Setup</title><style>body{font-family:Arial,sans-serif;background:#f3f4f6;padding:20px}.card{max-width:440px;margin:30px auto;background:#fff;padding:24px;border-radius:16px;box-shadow:0 2px 14px rgba(15,23,42,.10)}h1{font-size:24px;margin:0 0 8px;color:#0f172a}p{color:#475569;line-height:1.45}label{display:block;margin-top:16px;font-weight:800;color:#334155}select,input{width:100%;box-sizing:border-box;padding:12px;margin-top:6px;border:1px solid #cbd5e1;border-radius:10px;font-size:16px}button{width:100%;margin-top:22px;padding:13px;background:#0ea5e9;color:white;border:0;border-radius:999px;font-size:16px;font-weight:800;cursor:pointer}button:disabled{background:#94a3b8}.secondary{display:block;text-align:center;margin-top:14px;color:#0284c7;text-decoration:none;font-size:14px;cursor:pointer}.alert{padding:12px;border-radius:10px;margin-bottom:16px;font-size:14px;display:none}.error{background:#fee2e2;color:#991b1b}.success{background:#dcfce7;color:#166534}.info{background:#e0f2fe;color:#075985}.small{font-size:13px;color:#64748b;margin-top:16px}.hint{font-size:13px;color:#64748b;margin-top:6px}.hidden{display:none}</style></head><body><div class="card"><h1>Smart Fountain Setup</h1><p id="intro">Select your home Wi-Fi or type the SSID manually.</p><div id="message" class="alert"></div><form id="wifi-form"><label>Wi-Fi Network</label><select id="ssid" name="ssid"><option value="">Scanning will start automatically...</option></select><label>Manual SSID</label><input id="manual_ssid" name="manual_ssid" type="text" placeholder="Type SSID if needed"><p class="hint">Use manual SSID if scan fails or network is hidden.</p><label>Wi-Fi Key</label><input id="wifi_key" name="wifi_key" type="password"><button id="submit-button" type="submit">Test, Save and Restart</button></form><a id="refresh-link" class="secondary" onclick="loadNetworks()">Refresh Wi-Fi List</a><p class="small">Hotspot: Fountain-Setup<br>URL: http://192.168.4.1</p></div><script>const m=document.getElementById('message'),s=document.getElementById('ssid'),manual=document.getElementById('manual_ssid'),key=document.getElementById('wifi_key'),btn=document.getElementById('submit-button'),form=document.getElementById('wifi-form'),refresh=document.getElementById('refresh-link'),intro=document.getElementById('intro');function msg(t,c){m.textContent=t;m.className='alert '+c;m.style.display='block'}function clearMsg(){m.textContent='';m.style.display='none'}async function loadNetworks(){clearMsg();s.innerHTML='<option value="">Scanning...</option>';try{const r=await fetch('/networks');const d=await r.json();s.innerHTML='<option value="">Use manual SSID or select network</option>';if(!d.networks||d.networks.length===0){msg('No networks found. Type SSID manually.','info');return}d.networks.forEach(n=>{const o=document.createElement('option');o.value=n.ssid;o.textContent=n.ssid+' ('+n.rssi+' dBm)';s.appendChild(o)})}catch(e){s.innerHTML='<option value="">Use manual SSID</option>';msg('Scan failed. Type SSID manually.','info')}}form.addEventListener('submit',async e=>{e.preventDefault();const ssid=(manual.value.trim()||s.value);if(!ssid){msg('Select or type SSID.','error');return}btn.disabled=true;msg('Testing Wi-Fi. Wait up to 15 seconds...','info');const body=new URLSearchParams();body.append('ssid',ssid);body.append('wifi_key',key.value);try{const r=await fetch('/save',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:body.toString()});const d=await r.json();if(!d.ok){msg(d.message||'Could not connect. Check and try again.','error');btn.disabled=false;key.focus();return}msg(d.message||'Saved. Device restarting.','success');intro.textContent='Wi-Fi saved. Device is restarting.';form.className='hidden';refresh.className='hidden'}catch(e){msg('Test failed. Try again.','error');btn.disabled=false;key.focus()}});window.addEventListener('load',()=>setTimeout(loadNetworks,300));</script></body></html>
)rawliteral";
}

void applySetupWifiPower()
{
  WiFi.setTxPower(WIFI_POWER_8_5dBm);
}

void handleRoot()
{
  setupServer.send(200, "text/html", setupPageHtml());
}

void handleNetworks()
{
  Serial.println();
  Serial.println("Scanning Wi-Fi networks for setup page...");
  WiFi.mode(WIFI_AP_STA);
  applySetupWifiPower();
  WiFi.scanDelete();
  delay(100);
  int networkCount = WiFi.scanNetworks(false, true);
  Serial.print("Wi-Fi scan result count: ");
  Serial.println(networkCount);

  String json = "{\"networks\":[";
  bool first = true;
  if (networkCount > 0)
  {
    for (int i = 0; i < networkCount; i++)
    {
      String ssid = WiFi.SSID(i);
      if (ssid.length() == 0) continue;
      if (!first) json += ",";
      json += "{\"ssid\":\"" + jsonEscape(ssid) + "\",\"rssi\":" + String(WiFi.RSSI(i)) + "}";
      first = false;
    }
  }
  json += "]}";
  WiFi.scanDelete();
  setupServer.send(200, "application/json", json);
}

bool testWiFiCredentials(const String &ssid, const String &wifiKey)
{
  Serial.println();
  Serial.println("Testing submitted Wi-Fi credentials...");
  Serial.print("SSID: ");
  Serial.println(ssid);
  WiFi.mode(WIFI_AP_STA);
  applySetupWifiPower();
  WiFi.begin(ssid.c_str(), wifiKey.c_str());

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30)
  {
    delay(500);
    Serial.print(".");
    setupServer.handleClient();
    attempts++;
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println("Submitted Wi-Fi credentials worked.");
    Serial.print("Temporary station IP: ");
    Serial.println(WiFi.localIP());
    return true;
  }

  Serial.println("Submitted Wi-Fi credentials failed.");
  WiFi.disconnect(false, false);
  WiFi.mode(WIFI_AP_STA);
  applySetupWifiPower();
  return false;
}

void handleSave()
{
  String ssid = setupServer.arg("ssid");
  String wifiKey = setupServer.arg("wifi_key");
  ssid.trim();

  if (ssid.length() == 0)
  {
    setupServer.send(400, "application/json", "{\"ok\":false,\"message\":\"Please select or type a Wi-Fi SSID.\"}");
    return;
  }

  if (!testWiFiCredentials(ssid, wifiKey))
  {
    setupServer.send(200, "application/json", "{\"ok\":false,\"message\":\"Could not connect. Check the Wi-Fi key and try again.\"}");
    return;
  }

  if (!saveWifiCredentials(ssid, wifiKey))
  {
    setupServer.send(500, "application/json", "{\"ok\":false,\"message\":\"Connection worked, but saving failed.\"}");
    return;
  }

  setupServer.send(200, "application/json", "{\"ok\":true,\"message\":\"Wi-Fi saved successfully. Device is restarting.\"}");
  delay(1500);
  ESP.restart();
}

void handleNotFound()
{
  setupServer.sendHeader("Location", "/", true);
  setupServer.send(302, "text/plain", "");
}

void startSetupPortal()
{
  Serial.println();
  Serial.println("Starting setup portal...");
  WiFi.persistent(false);
  WiFi.disconnect(true, true);
  WiFi.softAPdisconnect(true);
  delay(250);
  WiFi.mode(WIFI_OFF);
  delay(250);
  WiFi.mode(WIFI_AP);
  delay(100);
  applySetupWifiPower();

  bool apStarted = WiFi.softAP(SETUP_AP_SSID, SETUP_AP_PASSWORD, SETUP_AP_CHANNEL, SETUP_AP_HIDDEN, SETUP_AP_MAX_CONNECTIONS);
  delay(200);
  IPAddress ip = WiFi.softAPIP();

  Serial.print("Setup hotspot started: ");
  Serial.println(apStarted ? "yes" : "no");
  Serial.print("Setup hotspot SSID: ");
  Serial.println(SETUP_AP_SSID);
  Serial.print("Setup hotspot URL: http://");
  Serial.println(ip);

  if (!apStarted)
  {
    Serial.println("Setup portal failed: softAP did not start.");
    setupPortalActive = false;
    return;
  }

  setupServer.on("/", HTTP_GET, handleRoot);
  setupServer.on("/networks", HTTP_GET, handleNetworks);
  setupServer.on("/save", HTTP_POST, handleSave);
  setupServer.on("/favicon.ico", HTTP_GET, []() { setupServer.send(204, "text/plain", ""); });
  setupServer.onNotFound(handleNotFound);
  setupServer.begin();
  setupPortalActive = true;
  Serial.println("Setup portal started.");
}

void handleSetupPortal()
{
  if (setupPortalActive)
  {
    setupServer.handleClient();
  }
}

bool isSetupPortalActive()
{
  return setupPortalActive;
}
