#include "uart_comm.h"
#include <Arduino.h>
#include <ArduinoJson.h>

#define UART_BAUD 115200

#ifdef ALTRA_RP2040
SerialPIO SerialUART(TX_PIN, RX_PIN);
#else
HardwareSerial SerialUART(1);
#endif

void uart_init() {
#ifdef ALTRA_RP2040
  SerialUART.begin(UART_BAUD);
#else
  SerialUART.begin(UART_BAUD, SERIAL_8N1, RX_PIN, TX_PIN);
  #endif
  delay(50);
}

bool uart_read_json(JsonDocument &doc) {
  static String buffer;

  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n') {
      DeserializationError error = deserializeJson(doc, buffer);
      buffer = "";
      return !error;
    }
    buffer += c;
  }

  while (SerialUART.available()) {
    char c = SerialUART.read();
    if (c == '\n') {
      DeserializationError error = deserializeJson(doc, buffer);
      buffer = "";
      return !error;
    }
    buffer += c;
  }

  return false;
}

bool uart_read_json_timeout(JsonDocument &doc, unsigned long timeoutMs) {
  unsigned long start = millis();
  while (millis() - start < timeoutMs) {
    if (uart_read_json(doc)) {
      return true;
    }
    delay(5);
  }
  return false;
}

void uart_send_json(const JsonDocument &doc) {
  String output;
  serializeJson(doc, output);
  output += '\n';
  Serial.print(output);
  Serial.flush();
  SerialUART.print(output);
}
