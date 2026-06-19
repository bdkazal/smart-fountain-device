#pragma once

#include <Arduino.h>

namespace WifiConfig
{
constexpr char SetupApSsid[] = "Fountain-Setup";
constexpr char SetupApPassword[] = "fountain123";
constexpr uint8_t SetupApChannel = 6;
constexpr uint8_t SetupApMaxConnections = 4;
constexpr bool SetupApHidden = false;

constexpr unsigned long ResetHoldMs = 3000;
constexpr unsigned long StationConnectTimeoutMs = 15000;
constexpr unsigned long StationReconnectIntervalMs = 10000;
constexpr unsigned long CredentialTestTimeoutMs = 15000;
constexpr unsigned long PortalRestartDelayMs = 1800;

constexpr uint16_t DnsPort = 53;
}
