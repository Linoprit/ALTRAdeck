#include "bluetooth_service.h"
#include "http_service.h"
#include "keyboard_service.h"
#include "mqtt_service.h"
#include "rest_service.h"
#include "uart_comm.h"
#include "wifi_credentials.h"
#include "wifi_manager.h"
#include <Arduino.h>
#include <ArduinoJson.h>

#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"

// #ifndef ENABLE_WIFI
// #define ENABLE_WIFI 1
// #endif
// #ifndef ENABLE_MQTT
// #define ENABLE_MQTT 1
// #endif
// #ifndef ENABLE_REST
// #define ENABLE_REST 1
// #endif
// #ifndef ENABLE_HTTP
// #define ENABLE_HTTP 1
// #endif

void ledinitialize() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
}
void ledOn() { digitalWrite(LED_BUILTIN, HIGH); }
void ledOff() { digitalWrite(LED_BUILTIN, LOW); }

// the loop function runs over and over again forever
void loop() {
  Serial.println("LED on");
  ledOn();
  delay(1000);
  // wait for a second
  Serial.println("LED off");
  ledOff();
  delay(1000); // wait for a second

  DynamicJsonDocument response(512);
  response["service"] = "message";
  response["message"] = "Unknown service";
  uart_send_json(response);
}

void setup() {
  Serial.begin(115200);

  ledinitialize();

  const unsigned long usbWaitStart = millis();
  while (!Serial && millis() - usbWaitStart < 5000) {
    delay(10);
  }
  Serial.println("Starting Services...");
  uart_init();
#if ENABLE_WIFI
  wifi_init(WIFI_SSID, WIFI_PASSWORD);
  Serial.println("WiFi initialized.");
#endif

#if ENABLE_MQTT
  mqtt_init();
#endif
#if ENABLE_REST
  rest_init();
#endif
#if ENABLE_HTTP
  http_init();
#endif
#if ENABLE_BLE
  bt_init();
#endif
#if ENABLE_USBHOST
  keyboard_service_init();
#endif

  Serial.println("Setup complete.");
}

// void loop() {

//   DynamicJsonDocument doc(2048);
//   if (uart_read_json(doc)) {
//     JsonObject request = doc.as<JsonObject>();
//     const char *service = request["service"];
//     if (service) {
// #if ENABLE_WIFI
//       if (strcmp(service, "wifi") == 0) {
//         Serial1.println("wifi_handle_request");
//         wifi_handle_request(request);
//       } else
// #endif
// #if ENABLE_MQTT
//           if (strcmp(service, "mqtt") == 0) {
//         mqtt_handle_request(request);
//       } else
// #endif
// #if ENABLE_REST
//           if (strcmp(service, "rest") == 0) {
//         rest_handle_request(request);
//       } else
// #endif
// #if ENABLE_BLE
//           if (strcmp(service, "bt") == 0) {
//         bt_handle_request(request);
//       } else
// #endif
// #if ENABLE_HTTP
//           if (strcmp(service, "http") == 0) {
//         http_handle_response(request);
//       } else
// #endif
// {
//   DynamicJsonDocument response(512);
//   response["service"] = "error";
//   response["message"] = "Unknown service";
//   uart_send_json(response);
// }
// }
// }
// #if ENABLE_MQTT
// mqtt_loop();
// #endif
// #if ENABLE_HTTP
// http_loop();
// #endif
// #if ENABLE_USBHOST
//   keyboard_service_task();
// #endif

//   delay(500);
//}

/*
Beispiel-UART-Nachrichten
Konfigurieren:
{"service":"wifi","action":"config","ssid":"MeinNetz","password":"Geheim"}
Verbinden:
{"service":"wifi","action":"connect"}
Trennen:
{"service":"wifi","action":"disconnect"}
Status:
{"service":"wifi","action":"status"}

MQTT konfigurieren:
{"service":"mqtt","action":"config","broker_ip":"192.168.1.100"}
{"service":"mqtt","action":"config","broker_ip":"192.168.1.100,"user":"geheim","password":"geheim"}
MQTT Status abfragen:
{"service":"mqtt","action":"status"}
MQTT abonnieren:
{"service":"mqtt","action":"subscribe","topic":"sensors/temperature","qos":1}
MQQT abbestellen:
{"service":"mqtt","action":"unsubscribe","topic":"sensors/temperature"}
MQTT veröffentlichen:
{"service":"mqtt","action":"publish","topic":"sensors/temperature","payload":"22.5","qos":1,"retain":false}

REST-Anfrage bearbeiten:
Anfrage vom Browser an den ESP32:
{"service":"rest","type":"request","endpoint":"/api","method":"GET","request_id":"295237:3","query":{}}
Antwort vom ESP32 an den Browser:
{"service":"rest","type":"response","request_id":"295237:3","status":200,"content_type":"text/plain","payload":"Hallo
Browser"}

http-Anfrage weiterleiten (GET):
{"service":"rest","type":"request","endpoint":"/favicon.ico","method":"GET","request_id":"306885:4","query":{}}
{"service":"rest","type":"request","endpoint":"/","method":"GET","request_id":"367969:5","query":{}}
Antwort vom Master an den ESP32:
{"service":"rest","type":"response","request_id":"367969:5","status":200,"content_type":"text/plain","payload":"Hallo
Browser"}


Scan anstoßen:
{"service":"bt","action":"scan"}
Bluetooth-Gerät verbinden:
{"service":"bt","action":"connect","address":"AA:BB:CC:DD:EE:FF"}
Bluetooth-Gerät trennen:
{"service":"bt","action":"disconnect"}
Bluetooth-Gerät auflisten:
{"service":"bt","action":"list"}
Modifier-Änderung:
{"service":"bt","type":"event","event":"modifier","device":"AA:BB:CC:DD:EE:FF","modifiers":2}
Key down:
{"service":"bt","type":"event","event":"key_down","device":"AA:BB:CC:DD:EE:FF","key":{"usage":4,"label":"a"}}
Key up:
{"service":"bt","type":"event","event":"key_up","device":"AA:BB:CC:DD:EE:FF","key":{"usage":4,"label":"a"}}
*/
