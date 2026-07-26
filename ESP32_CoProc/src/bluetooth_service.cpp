// #include <Arduino.h>
// #include <ArduinoJson.h>
// #include "bluetooth_service.h"
// #include "uart_comm.h"

// #include <NimBLEDevice.h>
// #include <vector>

// #define MAX_FOUND_DEVICES 20

// struct FoundDevice {
//     String address;
//     String name;
//     bool keyboard;
//     bool connectable;
// };

// enum KeyboardLayout {
//     LAYOUT_US,
//     LAYOUT_DE
// };

// enum BtEventMode {
//     EVENT_MODE_PARSED,
//     EVENT_MODE_TEXT,
//     EVENT_MODE_RAW
// };

// static FoundDevice foundDevices[MAX_FOUND_DEVICES];
// static int foundCount = 0;
// static NimBLEScan* pScan = nullptr;
// static NimBLEClient* pClient = nullptr;
// static NimBLERemoteCharacteristic* notifyChar = nullptr;
// static String connectedAddress = "";
// static KeyboardLayout keyboardLayout = LAYOUT_US;
// static BtEventMode eventMode = EVENT_MODE_PARSED;
// static bool scanOnlyKeyboardDevices = true;
// static String textBuffer = "";
// static unsigned long lastTextAt = 0;
// static const unsigned long textTimeoutMsDefault = 800;
// static unsigned long textTimeoutMs = textTimeoutMsDefault;
// static uint8_t prevModifiers = 0;
// static uint8_t prevKeys[6] = {0, 0, 0, 0, 0, 0};

// static void send_device_list() {
//     DynamicJsonDocument doc(2048);
//     doc["service"] = "bt";
//     doc["type"] = "response";
//     doc["action"] = "scan_results";
//     JsonArray arr = doc.createNestedArray("devices");
//     for (int i = 0; i < foundCount; i++) {
//         JsonObject d = arr.createNestedObject();
//         d["address"] = foundDevices[i].address;
//         d["name"] = foundDevices[i].name;
//         d["keyboard"] = foundDevices[i].keyboard;
//         d["connectable"] = foundDevices[i].connectable;
//     }
//     doc["keyboard_only"] = scanOnlyKeyboardDevices;
//     doc["layout"] = keyboardLayout == LAYOUT_DE ? "DE" : "US";
//     doc["event_mode"] = eventMode == EVENT_MODE_RAW ? "raw" : eventMode == EVENT_MODE_TEXT ? "text" : "parsed";
//     doc["text_timeout_ms"] = textTimeoutMs;
//     uart_send_json(doc);
// }

// static bool is_keyboard_candidate(const NimBLEAdvertisedDevice* dev) {
//     NimBLEUUID hidService((uint16_t)0x1812);
//     if (dev->haveAppearance() && dev->getAppearance() == 961) {
//         return true;
//     }
//     if (dev->haveServiceUUID() && dev->isAdvertisingService(hidService)) {
//         return true;
//     }
//     if (dev->haveName()) {
//         std::string name = dev->getName();
//         for (auto &c : name) {
//             c = tolower(c);
//         }
//         if (name.find("keyboard") != std::string::npos) {
//             return true;
//         }
//     }
//     return false;
// }

// static void flush_text_buffer() {
//     if (textBuffer.length() == 0 || connectedAddress.length() == 0) {
//         return;
//     }
//     DynamicJsonDocument doc(512);
//     doc["service"] = "bt";
//     doc["type"] = "event";
//     doc["event"] = "text";
//     doc["device"] = connectedAddress;
//     doc["text"] = textBuffer;
//     uart_send_json(doc);
//     textBuffer = "";
// }

// static bool is_printable_usage(uint8_t usage) {
//     return (usage >= 4 && usage <= 29) || (usage >= 30 && usage <= 56) || usage == 44 || usage == 57;
// }

// static char usage_to_char(uint8_t usage, KeyboardLayout layout, uint8_t modifiers) {
//     bool shift = modifiers & 0x02;
//     if (usage >= 4 && usage <= 29) {
//         char c = 'a' + (usage - 4);
//         return shift ? (char)toupper(c) : c;
//     }
//     if (usage >= 30 && usage <= 39) {
//         static const char normal[] = "1234567890";
//         static const char shiftUS[] = "!@#$%^&*()";
//         static const char shiftDE[] = "!\"§$%&/()=";
//         int idx = usage - 30;
//         if (idx < 0 || idx > 9) return 0;
//         return shift ? (layout == LAYOUT_DE ? shiftDE[idx] : shiftUS[idx]) : normal[idx];
//     }
//     if (usage == 44) {
//         return ' ';
//     }
//     if (usage >= 45 && usage <= 56) {
//         static const char normalUS[] = "-=[]\\;',./";
//         static const char shiftUS[] = "_+{}|:\\\"<>?";
//         static const char normalDE[] = "ß´ü+öä#,'-.";
//         static const char shiftDE[] = "?`Ü*ÖÄ'_;:";
//         int idx = usage - 45;
//         if (idx < 0 || idx >= 12) return 0;
//         if (layout == LAYOUT_DE) {
//             return shift ? shiftDE[idx] : normalDE[idx];
//         }
//         return shift ? shiftUS[idx] : normalUS[idx];
//     }
//     return 0;
// }

// static const char *usage_to_label(uint8_t usage, KeyboardLayout layout, uint8_t modifiers) {
//     static char label[8];
//     char c = usage_to_char(usage, layout, modifiers);
//     if (c != 0) {
//         label[0] = c;
//         label[1] = '\0';
//         return label;
//     }
//     switch (usage) {
//         case 1: return "ERR";
//         case 2: return "ESC";
//         case 3: return "BACKSPACE";
//         case 4: return "TAB";
//         case 40: return "ENTER";
//         case 41: return "ESC";
//         case 42: return "BACKSPACE";
//         case 43: return "TAB";
//         case 44: return "SPACE";
//         case 45: return layout == LAYOUT_DE ? "ß" : "-";
//         case 46: return layout == LAYOUT_DE ? "´" : "=";
//         case 47: return layout == LAYOUT_DE ? "ü" : "[";
//         case 48: return layout == LAYOUT_DE ? "+" : "]";
//         case 49: return layout == LAYOUT_DE ? "#" : "\\";
//         case 50: return layout == LAYOUT_DE ? "´" : "`";
//         case 51: return layout == LAYOUT_DE ? "ö" : ";";
//         case 52: return layout == LAYOUT_DE ? "ä" : "'";
//         case 53: return layout == LAYOUT_DE ? "^" : ",";
//         case 54: return layout == LAYOUT_DE ? ";" : ".";
//         case 55: return layout == LAYOUT_DE ? ":" : "/";
//         case 56: return layout == LAYOUT_DE ? "_" : ";";
//         case 57: return layout == LAYOUT_DE ? "#" : "\"";
//         case 58: return "CAPS_LOCK";
//         case 59: return "F1";
//         case 60: return "F2";
//         case 61: return "F3";
//         case 62: return "F4";
//         case 63: return "F5";
//         case 64: return "F6";
//         case 65: return "F7";
//         case 66: return "F8";
//         case 67: return "F9";
//         case 68: return "F10";
//         case 69: return "F11";
//         case 70: return "F12";
//         case 71: return "PRINT";
//         case 72: return "SCROLL_LOCK";
//         case 73: return "PAUSE";
//         case 74: return "INSERT";
//         case 75: return "HOME";
//         case 76: return "PAGE_UP";
//         case 77: return "DELETE";
//         case 78: return "END";
//         case 79: return "PAGE_DOWN";
//         case 80: return "RIGHT";
//         case 81: return "LEFT";
//         case 82: return "DOWN";
//         case 83: return "UP";
//         case 224: return "LCTRL";
//         case 225: return "LSHIFT";
//         case 226: return "LALT";
//         case 227: return "LMETA";
//         case 228: return "RCTRL";
//         case 229: return "RSHIFT";
//         case 230: return "RALT";
//         case 231: return "RMETA";
//         default: return "UNK";
//     }
// }

// static void handle_key_event(uint8_t usage, uint8_t modifiers, bool pressed) {
//     if (eventMode == EVENT_MODE_RAW) {
//         DynamicJsonDocument doc(512);
//         doc["service"] = "bt";
//         doc["type"] = "event";
//         doc["event"] = pressed ? "key_down_raw" : "key_up_raw";
//         doc["device"] = connectedAddress;
//         JsonObject keyObj = doc.createNestedObject("key");
//         keyObj["usage"] = usage;
//         keyObj["label"] = usage_to_label(usage, keyboardLayout, modifiers);
//         uart_send_json(doc);
//         return;
//     }

//     if (eventMode == EVENT_MODE_TEXT && is_printable_usage(usage)) {
//         char c = usage_to_char(usage, keyboardLayout, modifiers);
//         if (c != 0) {
//             if (millis() - lastTextAt > textTimeoutMs && textBuffer.length() > 0) {
//                 flush_text_buffer();
//             }
//             textBuffer += c;
//             lastTextAt = millis();
//             if (textBuffer.length() >= 16) {
//                 flush_text_buffer();
//             }
//             return;
//         }
//     }

//     if (eventMode == EVENT_MODE_TEXT) {
//         flush_text_buffer();
//     }

//     DynamicJsonDocument doc(512);
//     doc["service"] = "bt";
//     doc["type"] = "event";
//     doc["event"] = pressed ? "key_down" : "key_up";
//     doc["device"] = connectedAddress;
//     JsonObject keyObj = doc.createNestedObject("key");
//     keyObj["usage"] = usage;
//     keyObj["label"] = usage_to_label(usage, keyboardLayout, modifiers);
//     if (is_printable_usage(usage)) {
//         char c = usage_to_char(usage, keyboardLayout, modifiers);
//         if (c != 0) {
//             keyObj["char"] = String(c);
//         }
//     }
//     uart_send_json(doc);
// }

// static void handle_hid_report(uint8_t *pData, size_t length) {
//     if (length < 8) {
//         DynamicJsonDocument doc(1024);
//         doc["service"] = "bt";
//         doc["type"] = "event";
//         doc["event"] = "key_raw";
//         doc["device"] = connectedAddress;
//         JsonArray arr = doc.createNestedArray("data");
//         for (size_t i = 0; i < length; i++) arr.add(pData[i]);
//         uart_send_json(doc);
//         return;
//     }

//     uint8_t modifiers = pData[0];
//     uint8_t keys[6];
//     for (int i = 0; i < 6; i++) keys[i] = pData[2 + i];

//     if (modifiers != prevModifiers) {
//         DynamicJsonDocument doc(512);
//         doc["service"] = "bt";
//         doc["type"] = "event";
//         doc["event"] = "modifier";
//         doc["device"] = connectedAddress;
//         doc["modifiers"] = modifiers;
//         uart_send_json(doc);
//         prevModifiers = modifiers;
//     }

//     for (int i = 0; i < 6; i++) {
//         uint8_t k = keys[i];
//         if (k == 0) continue;
//         bool found = false;
//         for (int j = 0; j < 6; j++) {
//             if (prevKeys[j] == k) {
//                 found = true;
//                 break;
//             }
//         }
//         if (!found) {
//             handle_key_event(k, modifiers, true);
//         }
//     }

//     for (int i = 0; i < 6; i++) {
//         uint8_t pk = prevKeys[i];
//         if (pk == 0) continue;
//         bool stillPresent = false;
//         for (int j = 0; j < 6; j++) {
//             if (keys[j] == pk) {
//                 stillPresent = true;
//                 break;
//             }
//         }
//         if (!stillPresent) {
//             handle_key_event(pk, prevModifiers, false);
//         }
//     }

//     memcpy(prevKeys, keys, 6);
// }

// static void notifyCallback(NimBLERemoteCharacteristic* pRemoteCharacteristic, uint8_t* pData, size_t length, size_t /*mtu*/) {
//     handle_hid_report(pData, length);
// }

// void bt_init() {
//     NimBLEDevice::init("ESP32_BT_Client");
//     pScan = NimBLEDevice::getScan();
//     pScan->setActiveScan(true);
// }

// void bt_loop() {
//     if (eventMode == EVENT_MODE_TEXT && textBuffer.length() > 0 && millis() - lastTextAt >= textTimeoutMs) {
//         flush_text_buffer();
//     }
// }

// static bool ensureClient() {
//     if (!pClient) pClient = NimBLEDevice::createClient();
//     return pClient != nullptr;
// }

// void bt_handle_request(JsonObject request) {
//     const char *action = request["action"];
//     if (!action) return;

//     if (strcmp(action, "scan") == 0) {
//         if (request.containsKey("keyboard_only")) {
//             scanOnlyKeyboardDevices = request["keyboard_only"];
//         }
//         foundCount = 0;
//         pScan->clearResults();
//         NimBLEScanResults results = pScan->getResults(5, false);
//         int cnt = results.getCount();
//         for (int i = 0; i < cnt && foundCount < MAX_FOUND_DEVICES; i++) {
//             const NimBLEAdvertisedDevice* dev = results.getDevice(i);
//             if (!dev) continue;
//             bool keyboard = is_keyboard_candidate(dev);
//             if (scanOnlyKeyboardDevices && !keyboard) {
//                 continue;
//             }
//             String addr = dev->getAddress().toString().c_str();
//             String name = dev->getName().c_str();
//             foundDevices[foundCount].address = addr;
//             foundDevices[foundCount].name = name;
//             foundDevices[foundCount].keyboard = keyboard;
//             foundDevices[foundCount].connectable = dev->isConnectable();
//             foundCount++;
//         }
//         pScan->clearResults();
//         send_device_list();
//         return;
//     }

//     if (strcmp(action, "config") == 0) {
//         const char *layout = request["layout"];
//         const char *mode = request["mode"];
//         if (layout) {
//             keyboardLayout = (strcmp(layout, "DE") == 0 || strcmp(layout, "de") == 0) ? LAYOUT_DE : LAYOUT_US;
//         }
//         if (mode) {
//             if (strcmp(mode, "raw") == 0) {
//                 eventMode = EVENT_MODE_RAW;
//             } else if (strcmp(mode, "text") == 0) {
//                 eventMode = EVENT_MODE_TEXT;
//             } else {
//                 eventMode = EVENT_MODE_PARSED;
//             }
//         }
//         if (request.containsKey("keyboard_only")) {
//             scanOnlyKeyboardDevices = request["keyboard_only"];
//         }
//         if (request.containsKey("text_timeout_ms")) {
//             textTimeoutMs = request["text_timeout_ms"] | textTimeoutMsDefault;
//         }
//         DynamicJsonDocument doc(256);
//         doc["service"] = "bt";
//         doc["type"] = "response";
//         doc["action"] = "config";
//         doc["layout"] = keyboardLayout == LAYOUT_DE ? "DE" : "US";
//         doc["event_mode"] = eventMode == EVENT_MODE_RAW ? "raw" : eventMode == EVENT_MODE_TEXT ? "text" : "parsed";
//         doc["keyboard_only"] = scanOnlyKeyboardDevices;
//         doc["text_timeout_ms"] = textTimeoutMs;
//         uart_send_json(doc);
//         return;
//     }

//     if (strcmp(action, "status") == 0) {
//         DynamicJsonDocument doc(256);
//         doc["service"] = "bt";
//         doc["type"] = "response";
//         doc["action"] = "status";
//         doc["connected"] = pClient && pClient->isConnected();
//         doc["address"] = connectedAddress;
//         doc["layout"] = keyboardLayout == LAYOUT_DE ? "DE" : "US";
//         doc["event_mode"] = eventMode == EVENT_MODE_RAW ? "raw" : eventMode == EVENT_MODE_TEXT ? "text" : "parsed";
//         doc["keyboard_only"] = scanOnlyKeyboardDevices;
//         doc["text_timeout_ms"] = textTimeoutMs;
//         uart_send_json(doc);
//         return;
//     }

//     if (strcmp(action, "connect") == 0) {
//         const char *address = request["address"];
//         if (!address) return;
//         if (!ensureClient()) return;
//         if (pClient->isConnected()) {
//             pClient->disconnect();
//         }
//         NimBLEAddress addr(std::string(address), 0);
//         if (!pClient->connect(addr)) {
//             DynamicJsonDocument doc(256);
//             doc["service"] = "bt";
//             doc["type"] = "response";
//             doc["action"] = "connect";
//             doc["status"] = "failed";
//             uart_send_json(doc);
//             return;
//         }
//         connectedAddress = String(address);
//         std::vector<NimBLERemoteService*> services = pClient->getServices(true);
//         for (auto s : services) {
//             auto chars = s->getCharacteristics(true);
//             for (auto c : chars) {
//                 if (c->canNotify() || c->canIndicate()) {
//                     notifyChar = c;
//                     notifyChar->subscribe(true, notifyCallback);
//                     break;
//                 }
//             }
//             if (notifyChar) {
//                 break;
//             }
//         }
//         DynamicJsonDocument doc(256);
//         doc["service"] = "bt";
//         doc["type"] = "response";
//         doc["action"] = "connect";
//         doc["status"] = pClient->isConnected() ? "connected" : "failed";
//         doc["address"] = address;
//         uart_send_json(doc);
//         return;
//     }

//     if (strcmp(action, "disconnect") == 0) {
//         if (pClient && pClient->isConnected()) {
//             pClient->disconnect();
//         }
//         connectedAddress = "";
//         DynamicJsonDocument doc(256);
//         doc["service"] = "bt";
//         doc["type"] = "response";
//         doc["action"] = "disconnect";
//         doc["status"] = "ok";
//         uart_send_json(doc);
//         return;
//     }

//     if (strcmp(action, "list") == 0) {
//         send_device_list();
//         return;
//     }
// }
