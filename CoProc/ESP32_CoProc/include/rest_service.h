#pragma once

#include <ArduinoJson.h>

void rest_init();
void rest_loop();
void rest_handle_request(JsonObject request);
