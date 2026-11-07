#pragma once

#include <ArduinoJson.h>

#ifdef ESP32C3
#define RX_PIN 6
#define TX_PIN 7
#endif
#ifdef ESP32DEV
#define RX_PIN 16
#define TX_PIN 17
#endif
#ifdef ALTRA_RP2040
#define RX_PIN 9
#define TX_PIN 8
#endif

void uart_init();
bool uart_read_json(JsonDocument &doc);
bool uart_read_json_timeout(JsonDocument &doc, unsigned long timeoutMs);
void uart_send_json(const JsonDocument &doc);

void debug_loop();
void debug_send(const char *message);
