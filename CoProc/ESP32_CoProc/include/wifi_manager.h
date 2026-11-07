#pragma once

#include <ArduinoJson.h>

void wifi_init(const char *ssid, const char *password);
void wifi_handle_request(JsonObject request);
bool wifi_is_connected();
void wifi_ensure_http_server();
