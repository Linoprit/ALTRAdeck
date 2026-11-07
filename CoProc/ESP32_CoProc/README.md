# ESP32 Rest API Coprozessor

Dies ist ein Beispielprojekt für einen ESP32 S3 als Coprozessor über USART. Der ESP bietet drei Dienste:
- MQTT-Service
- REST-Service
- HTTP-Server

## Build Flags

Bitte konfigurieren Sie in `platformio.ini` oder beim Builden:
- `-D ENABLE_MQTT=1` oder `0`
- `-D ENABLE_REST=1` oder `0`
- `-D ENABLE_HTTP=1` oder `0`

## UART Kommunikation

Der ESP empfängt JSON-Nachrichten über UART und antwortet ebenfalls mit JSON. Der Master steuert die Services durch ein Top-Level-Feld:
- `service`: `mqtt`, `rest`, `http`, `bt`
- `action`: z.B. `config`, `subscribe`, `publish`, `scan`, `connect`

### Bluetooth JSON-Service (`bt`)

Requests:
- Scan starten:
  `{ "service": "bt", "action": "scan" }`
- Scan nur für Tastaturen:
  `{ "service": "bt", "action": "scan", "keyboard_only": true }`
- Gerät verbinden:
  `{ "service": "bt", "action": "connect", "address": "AA:BB:CC:DD:EE:FF" }`
- Gerät trennen:
  `{ "service": "bt", "action": "disconnect" }`
- Scan-Resultate abrufen:
  `{ "service": "bt", "action": "list" }`
- Status abrufen:
  `{ "service": "bt", "action": "status" }`
- Konfigurieren:
  `{ "service": "bt", "action": "config", "layout": "DE", "mode": "text", "keyboard_only": true, "text_timeout_ms": 500 }`

Response / Event Formate:
- Scan-Ergebnis:
  {
    "service": "bt",
    "type": "response",
    "action": "scan_results",
    "devices": [{ "address": "AA:BB:CC:DD:EE:FF", "name": "My Keyboard", "keyboard": true, "connectable": true }],
    "keyboard_only": true,
    "layout": "US",
    "event_mode": "parsed",
    "text_timeout_ms": 800
  }
- Status:
  `{ "service": "bt", "type": "response", "action": "status", "connected": true, "address": "AA:BB:CC:DD:EE:FF", "layout": "US", "event_mode": "parsed" }`
- Connect / Disconnect:
  `{ "service": "bt", "type": "response", "action": "connect", "status": "connected", "address": "AA:BB:CC:DD:EE:FF" }`
- Tastaturereignisse:
  `{ "service": "bt", "type": "event", "event": "key_down", "device": "AA:BB:CC:DD:EE:FF", "key": { "usage": 4, "label": "a", "char": "a" } }`
  `{ "service": "bt", "type": "event", "event": "key_up", "device": "AA:BB:CC:DD:EE:FF", "key": { "usage": 4, "label": "a" } }`
  `{ "service": "bt", "type": "event", "event": "modifier", "device": "AA:BB:CC:DD:EE:FF", "modifiers": 2 }`
  `{ "service": "bt", "type": "event", "event": "text", "device": "AA:BB:CC:DD:EE:FF", "text": "hello" }`
- Raw-Modus:
  `{ "service": "bt", "type": "event", "event": "key_down_raw", "device": "AA:BB:CC:DD:EE:FF", "key": { "usage": 4, "label": "a" } }`

## Beispiel MQTT-Anfrage

```
{ "service": "mqtt", "action": "config", "broker_ip": "192.168.1.10", "broker_port": 1883 }
```

## Hinweis

Diese Implementierung ist als Startpunkt gedacht. HTTP-Server-Anfragen werden aktuell als Proxy-Anfrage akzeptiert und bestätigen den Eingang.

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
MQTT abonnieren:
{"service":"mqtt","action":"subscribe","topic":"sensors/temperature","qos":1}
MQTT veröffentlichen:
{"service":"mqtt","action":"publish","topic":"sensors/temperature","payload":"22.5","qos":1,"retain":false}   

REST-Anfrage weiterleiten:
{"service":"rest","endpoint":"/api/data","payload":{"value":42}}

http-Anfrage weiterleiten (GET):
{"service":"http","method":"GET","path":"/status"}
http-Anfrage weiterleiten (POST):
{"service":"http","method":"POST","path":"/update","body":"{\"setting\":\"on\"}"} 

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

