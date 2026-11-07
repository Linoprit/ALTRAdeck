#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <WebServer.h>
#include "http_service.h"
#include "uart_comm.h"

static WebServer server(80);
static bool serverStarted = false;

static String build_request_id() {
    static unsigned long counter = 0;
    counter++;
    return String(millis()) + ":" + String(counter);
}

static void send_master_response_error(int httpStatus, const char *message) {
    server.send(httpStatus, "application/json", String("{\"error\":\"") + message + "\"}");
}

static void send_default_rest_response() {
    DynamicJsonDocument response(512);
    response["service"] = "rest";
    response["status"] = 200;
    response["message"] = "request received";
    response["endpoint"] = server.uri();
    response["method"] = server.method() == HTTP_GET ? "GET" : "POST";

    String body;
    serializeJson(response, body);
    server.send(200, "application/json", body);
}

static void handleRestRequest() {
    String requestId = build_request_id();
    DynamicJsonDocument request(2048);
    request["service"] = "rest";
    request["type"] = "request";
    request["endpoint"] = server.uri();
    request["method"] = server.method() == HTTP_GET ? "GET" : "POST";
    request["request_id"] = requestId;

    DynamicJsonDocument queryDoc(256);
    JsonObject queryObj = queryDoc.to<JsonObject>();
    for (int i = 0; i < server.args(); i++) {
        const String &name = server.argName(i);
        const String &value = server.arg(i);
        if (name == "plain") {
            continue;
        }
        queryObj[name] = value;
    }
    request["query"] = queryObj;
    if (server.hasArg("plain")) {
        request["body"] = server.arg("plain");
    }

    uart_send_json(request);

    DynamicJsonDocument masterResponse(4096);
    if (!uart_read_json_timeout(masterResponse, 10000)) {
        send_default_rest_response();
        return;
    }

    const char *service = masterResponse["service"];
    const char *type = masterResponse["type"];
    const char *responseId = masterResponse["request_id"];
    if (!service || strcmp(service, "rest") != 0 || !type || strcmp(type, "response") != 0 || !responseId || requestId != responseId) {
        send_master_response_error(502, "invalid_master_response");
        return;
    }

    int statusCode = masterResponse["status"] | 200;
    const char *contentType = masterResponse["content_type"];
    if (!contentType || strlen(contentType) == 0) {
        contentType = "application/json";
    }

    if (masterResponse.containsKey("body")) {
        String body;
        serializeJson(masterResponse["body"], body);
        server.send(statusCode, contentType, body);
        return;
    }
    if (masterResponse.containsKey("payload")) {
        if (masterResponse["payload"].is<const char*>()) {
            server.send(statusCode, contentType, String((const char *)masterResponse["payload"]));
            return;
        }
        String body;
        serializeJson(masterResponse["payload"], body);
        server.send(statusCode, contentType, body);
        return;
    }

    String responseText;
    serializeJson(masterResponse, responseText);
    server.send(statusCode, contentType, responseText);
}

static void handleProxyRequest() {
    DynamicJsonDocument request(1024);
    request["service"] = "http";
    request["type"] = "request";
    request["request_id"] = build_request_id();
    request["method"] = server.method() == HTTP_GET ? "GET" : "POST";
    request["path"] = server.uri();
    if (server.hasArg("plain")) {
        request["body"] = server.arg("plain");
    }
    uart_send_json(request);

    DynamicJsonDocument response(1024);
    response["status"] = "accepted";
    response["path"] = server.uri();
    server.send(202, "application/json", "{\"status\":\"accepted\"}");
}

static void handleNotFound() {
    handleRestRequest();
}

void http_init() {
    server.on("/api", HTTP_ANY, handleRestRequest);
    server.on("/api/", HTTP_ANY, handleRestRequest);
    server.on("/status", HTTP_GET, handleProxyRequest);
    server.on("/logs", HTTP_GET, handleProxyRequest);
    server.on("/", HTTP_GET, handleRestRequest);
    server.onNotFound(handleNotFound);
}

void http_start_server() {
    if (!serverStarted && WiFi.status() == WL_CONNECTED) {
        server.begin();
        serverStarted = true;
    }
}

void http_loop() {
    if (serverStarted) {
        server.handleClient();
    }
}

void http_handle_response(JsonObject response) {
    // Not used for REST API response forwarding in this implementation.
}
