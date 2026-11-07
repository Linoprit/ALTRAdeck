#include "keyboard_service.h"

#if defined(ALTRA_RP2040) && ENABLE_USBHOST

#include <Adafruit_TinyUSB.h>
#include <ArduinoJson.h>
#include <pio_usb.h>

#include "uart_comm.h"

namespace {
Adafruit_USBH_Host usb_host;
uint8_t previous_keys[6] = {};
uint8_t previous_modifiers = 0;
bool keyboard_mounted = false;

bool contains_key(const uint8_t *keys, uint8_t key) {
  for (uint8_t index = 0; index < 6; ++index) {
    if (keys[index] == key) {
      return true;
    }
  }
  return false;
}

void send_key_event(const char *action, uint8_t key, uint8_t modifiers,
                    uint8_t device_address, uint8_t instance) {
  DynamicJsonDocument response(384);
  response["service"] = "keyboard";
  response["type"] = "response";
  response["action"] = action;
  response["scan_code"] = key;
  response["modifiers"] = modifiers;
  response["device_address"] = device_address;
  response["interface"] = instance;
  uart_send_json(response);
}

void send_modifier_event(uint8_t modifiers, uint8_t device_address,
                         uint8_t instance) {
  DynamicJsonDocument response(384);
  response["service"] = "keyboard";
  response["type"] = "response";
  response["action"] = "modifier";
  response["modifiers"] = modifiers;
  response["device_address"] = device_address;
  response["interface"] = instance;
  uart_send_json(response);
}
}  // namespace

void keyboard_service_init() {
  Serial.println("USB keyboard host enabled on GP2 (D+) / GP3 (D-).");
}

void keyboard_service_task() {
}

void setup1() {
  pio_usb_configuration_t pio_config = PIO_USB_DEFAULT_CONFIG;
  pio_config.pin_dp = PIN_USB_HOST_DP;
#if defined(ARDUINO_RASPBERRY_PI_PICO_W)
  pio_config.sm_tx = 3;
  pio_config.sm_rx = 2;
  pio_config.sm_eop = 3;
  pio_config.pio_rx_num = 0;
  pio_config.pio_tx_num = 1;
  pio_config.tx_ch = 9;
#endif
  usb_host.configure_pio_usb(1, &pio_config);
  usb_host.begin(1);
}

void loop1() {
  usb_host.task();
}

extern "C" {
void tuh_hid_mount_cb(uint8_t device_address, uint8_t instance,
                      uint8_t const *report_descriptor,
                      uint16_t report_descriptor_length) {
  (void) report_descriptor;
  (void) report_descriptor_length;
  keyboard_mounted = true;
  previous_modifiers = 0;
  for (uint8_t &key : previous_keys) {
    key = 0;
  }

  tuh_hid_set_protocol(device_address, instance, HID_PROTOCOL_BOOT);
  tuh_hid_receive_report(device_address, instance);
}

void tuh_hid_umount_cb(uint8_t device_address, uint8_t instance) {
  (void) device_address;
  (void) instance;
  keyboard_mounted = false;
  previous_modifiers = 0;
  for (uint8_t &key : previous_keys) {
    key = 0;
  }
}

void tuh_hid_report_received_cb(uint8_t device_address, uint8_t instance,
                                uint8_t const *report, uint16_t length) {
  if (keyboard_mounted && length >= 8) {
    const uint8_t modifiers = report[0];
    const uint8_t *keys = report + 2;

    if (modifiers != previous_modifiers) {
      send_modifier_event(modifiers, device_address, instance);
    }

    for (uint8_t key : previous_keys) {
      if (key != 0 && !contains_key(keys, key)) {
        send_key_event("key_up", key, modifiers, device_address, instance);
      }
    }
    for (uint8_t key_index = 0; key_index < 6; ++key_index) {
      const uint8_t key = keys[key_index];
      if (key != 0 && !contains_key(previous_keys, key)) {
        send_key_event("key_down", key, modifiers, device_address, instance);
      }
    }

    previous_modifiers = modifiers;
    for (uint8_t index = 0; index < 6; ++index) {
      previous_keys[index] = keys[index];
    }
  }

  tuh_hid_receive_report(device_address, instance);
}
}

#else

void keyboard_service_init() {}
void keyboard_service_task() {}

#endif
