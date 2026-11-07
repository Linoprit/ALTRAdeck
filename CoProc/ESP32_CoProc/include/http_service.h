#pragma once

#include <ArduinoJson.h>

void http_init();
void http_start_server();
void http_loop();
void http_handle_response(JsonObject response);
