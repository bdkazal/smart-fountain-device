#include <Arduino.h>
#include <WiFi.h>
#include "esp_bt.h"

void setup()
{
  Serial.begin(115200);
  delay(1000);

  // 1. COMPLETELY DISABLE BLUETOOTH TO REDUCE HEAT
  // This releases the memory used by BT back to the system
  btStop();
  esp_bt_controller_disable();
  esp_bt_controller_mem_release(ESP_BT_MODE_BTDM);

  Serial.println("\n--- ESP32-C3 WiFi Diagnostic Tool ---");
  Serial.println("Bluetooth has been disabled and memory released.");

  // 2. WIFI POWER HACK (The "Stability" Setting)
  // Start with the 'Hack' enabled to prevent overheating/noise.
  // To test "Without Hack", comment this line out or change to 20.
  WiFi.setTxPower(WIFI_POWER_8_5dBm);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
}

void loop()
{
  Serial.println("\nScanning for networks...");
  int n = WiFi.scanNetworks();

  if (n == 0)
  {
    Serial.println("No networks found. Antenna may be disconnected.");
  }
  else
  {
    Serial.printf("Found %d networks:\n", n);
    for (int i = 0; i < n; ++i)
    {
      // Print SSID and RSSI for each network found
      Serial.printf("%-20s | Signal: %ddBm\n", WiFi.SSID(i).c_str(), WiFi.RSSI(i));
      delay(10);
    }
  }

  // Wait 5 seconds before scanning again
  delay(5000);
}