#pragma once

#include <Arduino.h>

namespace AppConfig {

constexpr uint8_t LED_PIN = 2;
constexpr bool LED_ACTIVE_HIGH = true;
constexpr unsigned long BLINK_INTERVAL_MS = 500;
constexpr unsigned long WIFI_RETRY_INTERVAL_MS = 15000;
constexpr unsigned long OTA_CHECK_INTERVAL_MS = 24UL * 60UL * 60UL * 1000UL;

constexpr char DEVICE_NAME[] = "esp32-ota";
constexpr char FW_VERSION[] = APP_VERSION;

constexpr char AP_SSID[] = "Proyecto";
constexpr char AP_PASSWORD[] = "12345678";
const IPAddress AP_IP(192, 168, 4, 1);
const IPAddress AP_GATEWAY(192, 168, 4, 1);
const IPAddress AP_SUBNET(255, 255, 255, 0);

constexpr char PREF_NAMESPACE[] = "esp-ota";
constexpr char PREF_WIFI_SSID[] = "wifiSsid";
constexpr char PREF_WIFI_PASS[] = "wifiPass";
constexpr char PREF_MANIFEST_URL[] = "manifestUrl";
constexpr char PREF_PORTAL_MODE[] = "portalMode";

}  // namespace AppConfig

