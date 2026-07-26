#include <Arduino.h>
#include <ArduinoJson.h>
#include "rest_service.h"
#include "uart_comm.h"

void rest_init() {
}

void rest_loop() {
}

void rest_handle_request(JsonObject request) {
	const char *endpoint = request["endpoint"];
	if (!endpoint) {
		return;
	}
	DynamicJsonDocument response(1024);
	response["service"] = "rest";
	response["endpoint"] = endpoint;
	response["status"] = "forwarded";
	response["payload"] = request["payload"];
	uart_send_json(response);
}
