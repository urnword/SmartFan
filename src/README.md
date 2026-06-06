# SmartFan Controller — ESP32-C3 Super Mini
## Firmware v1.0

---

## Hardware

| Pin | Function |
|-----|----------|
| GPIO5 | PWM out → MOSFET gate (fan) |
| GPIO9 | Physical button (boot pin, active LOW) |
| GPIO8 | Status LED (onboard, active LOW) |

**Fan wiring:** Use an N-channel MOSFET (e.g. IRLZ44N, AO3400) between GND and the fan's negative wire. Gate → GPIO3 through a 100Ω resistor. Add a flyback diode across the fan.

---

## Required Libraries (Arduino IDE / PlatformIO)

```
ArduinoJson          @ ^6.21.x    (Benoit Blanchon)
```

### Built-in (ESP32 Arduino core ≥ 2.0):
- `WiFi.h`
- `WebServer.h`
- `Preferences.h`
- `time.h`
- `esp_task_wdt.h`

Install ESP32 Arduino core via Boards Manager:
`https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`

Select board: **ESP32C3 Dev Module**

---

## Board Settings (Arduino IDE)

| Setting | Value |
|---------|-------|
| Board | ESP32C3 Dev Module |
| USB CDC On Boot | Enabled |
| CPU Frequency | 160MHz |
| Flash Mode | QIO |
| Flash Size | 4MB |
| Partition Scheme | Default 4MB with spiffs |
| Upload Speed | 921600 |

---

## Feature Summary

### Soft Start
- PWM ramps from current % → target % over `SOFT_START_MS` (2000ms default)
- Uses ease-in-out cubic curve — smooth, no jerk
- Configure in firmware: `#define SOFT_START_MS 2000`

### Kickstart
- If target speed < 40% and motor is stopped, it briefly boosts to 40% for 800ms
- Then ramps back down to actual target
- Prevents stall on low-duty PWM

### Minimum Speed
- `MIN_RUN_SPEED 15` — below this, fan won't try to start
- Prevents coil buzz at near-zero PWM

### Watchdog
- Hardware WDT: 30s timeout → auto-reset
- Command timeout: if no HTTP command received in 5min → stays at fallback speed
- Configure: `CMD_TIMEOUT_MS`, `FALLBACK_SPEED`

### Physical Button
| Action | Result |
|--------|--------|
| Short press | Toggle fan ON/OFF |
| Long press (>800ms) | Cycle speed: 30% → 60% → 100% |
| Double click (<400ms gap) | Toggle WiFi on/off |

### State Memory (NVS Flash)
Saves on every change: `fanOn`, `speed`, WiFi credentials, all schedules, night mode config

### WiFi
1. On boot: tries saved SSID
2. If no saved SSID or connection fails → starts AP `SmartFan-Setup` / `smartfan123`
3. Connect to AP, go to `192.168.4.1`, enter your WiFi credentials
4. Device restarts and connects

### Night Mode
- Time window (e.g. 22:00–07:00)
- Cycles between low speed (e.g. 30%) and high speed (e.g. 100%)
- `slowDuration`: minutes at low before switching high
- `highDuration`: minutes at high before switching low
- Automatically activates/deactivates at start/end time

### Sleep Timer
- Turn fan off after N minutes
- Configurable from web UI (15m / 30m / 1h / 2h)
- Cancelable

### Schedules
- Up to 10 schedules
- Time-only (no date) — fires daily at HH:MM
- Actions: Turn ON, Turn OFF, Set Speed %
- NTP-synced time required for schedules

---

## REST API (for CYD or any HTTP client)

```
GET  /api/status
GET  /api/schedules
GET  /api/nightmode
GET  /api/wifi/status

POST /api/fan           {"on": true}
POST /api/fan           {"speed": 75}
POST /api/schedules     {"hour":8,"minute":0,"action":1,"speed":0,"label":"Morning","active":true}
DELETE /api/schedules?id=2
POST /api/nightmode     {"enabled":true,"startHour":22,"startMinute":0,...}
POST /api/sleep         {"minutes": 30}
POST /api/wifi          {"ssid":"MyNet","pass":"password"}
```

### Action values for schedules:
- `0` = Turn OFF
- `1` = Turn ON
- `2` = Set speed (requires `speed` field)

---

## NTP Timezone

Change `NTP_TZ` in firmware for your region:
```cpp
#define NTP_TZ "MYT-8"          // Malaysia
#define NTP_TZ "UTC0"           // UTC
#define NTP_TZ "EST5EDT"        // US Eastern
#define NTP_TZ "SGT-8"          // Singapore
```

---

## Future: Temperature Sensor

The architecture is ready — add your sensor read in `loop()` and use the value to:
- Override speed in `setFanSpeed()`
- Add a `/api/temperature` endpoint
- Add temp-based schedule actions

Suggested: DS18B20 on GPIO4, NTC thermistor on ADC, or SHT31 over I2C.

---

## File Structure
```
smart_fan/
├── smart_fan.ino   — main firmware
├── webui.h         — web UI stored in PROGMEM
└── README.md       — this file
```
