#include "http_service.h"
#include "uart_comm.h"
#include "wifi_manager.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>

static String wifiSsid = "";
static String wifiPassword = "";
static bool wifiConfigured = false;

static void wifi_event_handler(WiFiEvent_t event, WiFiEventInfo_t info) {
  switch (event) {
  case ARDUINO_EVENT_WIFI_STA_START:
    Serial.println("[wifi] event: STA_START");
    break;
  case ARDUINO_EVENT_WIFI_STA_CONNECTED:
    Serial.printf("[wifi] event: STA_CONNECTED to %s\n",
                  info.wifi_sta_connected.ssid);
    break;
  case ARDUINO_EVENT_WIFI_STA_GOT_IP:
    Serial.printf("[wifi] event: GOT_IP %s\n",
                  IPAddress(info.got_ip.ip_info.ip.addr).toString().c_str());
    break;
  case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
    Serial.printf("[wifi] event: STA_DISCONNECTED reason=%d\n",
                  info.wifi_sta_disconnected.reason);
    break;
  case ARDUINO_EVENT_WIFI_STA_AUTHMODE_CHANGE:
    Serial.printf("[wifi] event: AUTHMODE_CHANGE old=%d new=%d\n",
                  info.wifi_sta_authmode_change.old_mode,
                  info.wifi_sta_authmode_change.new_mode);
    break;
  default:
    Serial.printf("[wifi] event: %d\n", event);
    break;
  }
}

static std::string get_wifi_status_string(wl_status_t status) {
  switch (status) {
  case WL_NO_SHIELD:
    return "no_shield";
  case WL_IDLE_STATUS:
    return "idle";
  case WL_NO_SSID_AVAIL:
    return "no_ssid_available";
  case WL_SCAN_COMPLETED:
    return "scan_completed";
  case WL_CONNECTED:
    return "connected";
  case WL_CONNECT_FAILED:
    return "connect_failed";
  case WL_CONNECTION_LOST:
    return "connection_lost";
  case WL_DISCONNECTED:
    return "disconnected";
  default:
    return "unknown_status";
  }
}

static void send_wifi_status(const char *status) {
  DynamicJsonDocument result(256);
  result["service"] = "wifi";
  result["status"] = status;
  result["configured"] = wifiConfigured;
  result["wifi_status"] = get_wifi_status_string(WiFi.status());
  result["ssid"] = wifiSsid;
  if (WiFi.status() == WL_CONNECTED) {
    result["ip"] = WiFi.localIP().toString();
  }
  uart_send_json(result);
}

static bool connect_wifi() {
  Serial.println("[wifi] Starting connection sequence...");
  Serial.printf("[wifi] SSID: %s\n", wifiSsid.c_str());
  Serial.printf("[wifi] Board: %s\n", ARDUINO_BOARD);
  Serial.printf("[wifi] SDK: %s\n", ESP.getSdkVersion());

  for (int attempt = 1; attempt <= 3; ++attempt) {
    Serial.printf("[wifi] connect attempt %d/3\n", attempt);

    WiFi.mode(WIFI_OFF);
    delay(100);
    WiFi.mode(WIFI_STA);
    WiFi.disconnect(true, true);
    delay(500);

    WiFi.onEvent(wifi_event_handler);
    WiFi.setAutoReconnect(true);
    WiFi.persistent(false);
    WiFi.setSleep(false);
    WiFi.setHostname("alpha-deck");
    WiFi.setTxPower(WIFI_POWER_19_5dBm);

    //Serial.println("[wifi] WiFi mode set to STA");
    Serial.println("[wifi] Scanning for nearby networks...");
    int networkCount = WiFi.scanNetworks();
    delay(1000);
    Serial.printf("[wifi] Scan finished, found %d networks\n", networkCount);
    int targetNetworkIndex = -1;
    for (int i = 0; i < networkCount; ++i) {
      Serial.printf("[wifi] network[%d]: %s (RSSI=%d, CH=%d)\n", i,
                    WiFi.SSID(i).c_str(), WiFi.RSSI(i), WiFi.channel(i));
      if (WiFi.SSID(i) == wifiSsid) {
        targetNetworkIndex = i;
        break;
      }
    }

    Serial.println("[wifi] Starting WiFi.begin...");
    Serial.println("WiFi SSID: " + wifiSsid + ", Password: " + wifiPassword);
    WiFi.begin(wifiSsid.c_str(), wifiPassword.c_str());

    // if (targetNetworkIndex >= 0) {
    //     uint8_t bssid[6];
    //     memcpy(bssid, WiFi.BSSID(targetNetworkIndex), 6);
    //     Serial.printf("[wifi] Connecting to %s on channel %d with BSSID
    //     %02X:%02X:%02X:%02X:%02X:%02X\n",
    //                   WiFi.SSID(targetNetworkIndex).c_str(),
    //                   WiFi.channel(targetNetworkIndex),
    //                   bssid[0], bssid[1], bssid[2], bssid[3], bssid[4],
    //                   bssid[5]);
    //     WiFi.begin(wifiSsid.c_str(), wifiPassword.c_str(),
    //     WiFi.channel(targetNetworkIndex), bssid);
    // } else {
    //     WiFi.begin(wifiSsid.c_str(), wifiPassword.c_str());
    // }

    send_wifi_status("connecting");

    Serial.println("[wifi] Waiting for connection result...");
    int status = WiFi.waitForConnectResult(30000);
    Serial.printf("[wifi] waitForConnectResult returned: %d\n", status);
    Serial.printf("[wifi] current status after wait: %d (%s)\n", WiFi.status(),
                  get_wifi_status_string(WiFi.status()).c_str());

    if (status == WL_CONNECTED) {
      send_wifi_status("connected");
      Serial.println("WiFi connected. IP address: " +
                     WiFi.localIP().toString());
      wifi_ensure_http_server();
      return true;
    }

    Serial.printf(
        "[wifi] attempt %d failed with status: %d (%s)\n", attempt, status,
        get_wifi_status_string(static_cast<wl_status_t>(status)).c_str());
    delay(1000);
  }

  send_wifi_status("failed");
  return false;
}

void wifi_init(const char *ssid, const char *password) {
  if (ssid && strlen(ssid) > 0) {
    wifiSsid = String(ssid);
    wifiPassword = String(password ? password : "");

    Serial.println("WiFi configured with SSID: " + wifiSsid);
    Serial.println("WiFi configured with Password: " + wifiPassword);

    wifiConfigured = true;
    connect_wifi();
  } else {
    send_wifi_status("unconfigured");
  }
}

void wifi_handle_request(JsonObject request) {
  const char *action = request["action"];
  if (!action) {
    send_wifi_status("invalid_request");
    return;
  }

  if (strcmp(action, "config") == 0) {
    const char *ssid = request["ssid"];
    const char *password = request["password"];
    if (ssid && strlen(ssid) > 0) {
      wifiSsid = String(ssid);
      wifiPassword = String(password ? password : "");
      wifiConfigured = true;
      send_wifi_status("configured");
    } else {
      send_wifi_status("missing_ssid");
    }
  } else if (strcmp(action, "connect") == 0) {
    if (!wifiConfigured || wifiSsid.length() == 0) {
      send_wifi_status("unconfigured");
      return;
    }
    connect_wifi();
  } else if (strcmp(action, "disconnect") == 0) {
    WiFi.disconnect(true, true);
    send_wifi_status("disconnected");
  } else if (strcmp(action, "status") == 0) {
    send_wifi_status("status");
  } else {
    send_wifi_status("unknown_action");
  }
}

bool wifi_is_connected() { return WiFi.status() == WL_CONNECTED; }

void wifi_ensure_http_server() {
  if (WiFi.status() == WL_CONNECTED) {
    http_start_server();
  }
}
