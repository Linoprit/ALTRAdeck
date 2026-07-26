#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "mqtt_service.h"
#include "uart_comm.h"

static WiFiClient wifiClient;
static PubSubClient mqttClient(wifiClient);
static String brokerAddress;
static uint16_t brokerPort = 1883;
static String brokerUsername;
static String brokerPassword;
static bool connected = false;

void mqtt_init() {
    mqttClient.setCallback([](char *topic, byte *payload, unsigned int length) {
        DynamicJsonDocument response(1024);
        response["service"] = "mqtt";
        response["type"] = "incoming";
        response["topic"] = topic;
        response["payload"] = String((char *)payload, length);
        uart_send_json(response);
    });
}

static void mqtt_connect() {
    if (!connected && brokerAddress.length() > 0 && WiFi.status() == WL_CONNECTED) {
        mqttClient.setServer(brokerAddress.c_str(), brokerPort);
        bool ok = false;
        if (brokerUsername.length() > 0 || brokerPassword.length() > 0) {
            ok = mqttClient.connect("alpha-deck", brokerUsername.c_str(), brokerPassword.c_str());
        } else {
            ok = mqttClient.connect("alpha-deck");
        }

        if (ok) {
            connected = true;
            Serial.println("[mqtt] connected to broker");
        } else {
            connected = false;
            Serial.println("[mqtt] connect to broker failed");
        }
    }
}

void mqtt_loop() {
    if (!connected) {
        mqtt_connect();
    }
    if (connected) {
        mqttClient.loop();
    }
}

static void send_mqtt_status(const char *status) {
    DynamicJsonDocument result(512);
    result["service"] = "mqtt";
    result["status"] = status;
    result["configured"] = brokerAddress.length() > 0;
    result["broker_ip"] = brokerAddress;
    result["broker_port"] = brokerPort;
    result["broker_user"] = brokerUsername;
    result["broker_password"] = brokerPassword;
    result["connected"] = connected;
    result["mqtt_client_connected"] = mqttClient.connected();
    result["wifi_connected"] = (WiFi.status() == WL_CONNECTED);
    result["wifi_client_connected"] = wifiClient.connected();
    uart_send_json(result);
}

void mqtt_handle_request(JsonObject request) {
    const char *action = request["action"];
    if (!action) {
        return;
    }
    if (strcmp(action, "config") == 0) {
        const char *ip = request["broker_ip"];
        int port = request["broker_port"] | 1883;
        const char *user = request["user"];
        if (!user) {
            user = request["username"];
        }
        if (!user) {
            user = request["broker_user"];
        }
        const char *password = request["password"];
        if (!password) {
            password = request["broker_password"];
        }
        if (ip) {
            brokerAddress = String(ip);
            brokerPort = port;
            brokerUsername = user ? String(user) : "";
            brokerPassword = password ? String(password) : "";
            connected = false;
        }
    } else if (strcmp(action, "status") == 0) {
        send_mqtt_status("status");
    } else if (strcmp(action, "subscribe") == 0) {
        const char *topic = request["topic"];
        int qos = request["qos"] | 0;
        mqtt_connect();
        if (topic && connected) {
            mqttClient.subscribe(topic, qos);
        }
    } else if (strcmp(action, "unsubscribe") == 0) {
        const char *topic = request["topic"];
        mqtt_connect();
        if (topic && connected) {
            mqttClient.unsubscribe(topic);
        }
    } else if (strcmp(action, "publish") == 0) {
        const char *topic = request["topic"];
        const char *payload = request["payload"];
        int qos = request["qos"] | 0;
        bool retain = request["retain"] | false;
        mqtt_connect();
        if (topic && connected) {
            mqttClient.publish(topic, payload ? payload : "", retain);
        }
    }
}
