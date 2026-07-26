#pragma once

#include <ArduinoJson.h>

void mqtt_init();
void mqtt_loop();
void mqtt_handle_request(JsonObject request);
