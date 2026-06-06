/*
 * SmartFan Controller v1.1
 * Hardware: ESP32-C3 Super Mini
 *
 * Changes over v1.0:
 *  - WiFi always enabled on boot (no ROM persistence of wifi on/off)
 *    STA→AP fallback is near-immediate (3s timeout) for faster init
 *  - "Night Mode" renamed to "AutoCycle" everywhere
 *  - Default speed saved to ROM (used every time fan turns on)
 *  - LED blink can be disabled via settings (saved to ROM)
 *  - NTP timezone configurable via settings (saved to ROM)
 *  - mDNS: device accessible at http://smartfan.local
 */

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include <time.h>

// ─── PINS ────────────────────────────────────────────────────────────────────
#define FAN_PWM_PIN      5
#define BUTTON_PIN       9
#define STATUS_LED_PIN   8

// ─── PWM ─────────────────────────────────────────────────────────────────────
#define PWM_FREQ         500
#define PWM_RESOLUTION   8
#define PWM_MAX          255
#define PHYSICAL_FLOOR   50   // 50% duty = user "0%" (minimum to spin)

// ─── SOFT-START ──────────────────────────────────────────────────────────────
#define SOFT_START_MS    2000
#define MIN_RUN_SPEED    15
#define KICKSTART_SPEED  40   // brief boost % for low-speed start
#define KICKSTART_MS     800  // hold duration at kickstart speed

// ─── DEFAULTS ────────────────────────────────────────────────────────────────
#define DEFAULT_SPEED    50
#define MAX_SCHEDULES    10
#define AP_SSID          "SmartFan-Setup"
#define AP_PASS          "smartfan123"
#define NTP_SERVER       "pool.ntp.org"
#define NTP_TZ_DEFAULT   "MYT-8"
#define MDNS_NAME        "smartfan"

// ─── STRUCTURES ──────────────────────────────────────────────────────────────
struct Schedule {
  bool    active;
  uint8_t hour, minute;
  uint8_t action;   // 0=off  1=on  2=set_speed
  uint8_t speed;
  char    label[32];
};

struct AutoCycle {
  bool     enabled;
  uint8_t  startHour, startMinute;
  uint8_t  endHour,   endMinute;
  uint8_t  lowSpeed,  highSpeed;
  uint16_t slowDuration;  // min at low speed
  uint16_t highDuration;  // min at high speed
};

// ─── GLOBALS ─────────────────────────────────────────────────────────────────
WebServer   server(80);
Preferences prefs;

bool    fanOn        = false;
uint8_t targetSpeed  = DEFAULT_SPEED;
uint8_t currentPct   = 0;
uint8_t defaultSpeed = DEFAULT_SPEED;  // user-configurable default
bool    ledEnabled   = true;           // LED blink on/off
char    ntpTz[48]    = NTP_TZ_DEFAULT; // configurable timezone
bool    apMode       = false;

Schedule  schedules[MAX_SCHEDULES];
AutoCycle autoCycle = {false, 22, 0, 7, 0, 30, 100, 30, 10};

// Sleep timer — overflow-safe: start + duration
uint32_t sleepStart    = 0;
uint32_t sleepDuration = 0;  // ms; 0 = disabled

// Soft-start — 3-phase async state machine
enum RampPhase { RAMP_NONE, RAMP_MAIN, RAMP_KICKSTART_UP, RAMP_KICKSTART_HOLD };
RampPhase rampPhase  = RAMP_NONE;
uint32_t  rampStart_ = 0;
uint8_t   rampFrom   = 0;
uint8_t   rampTo_    = 0;

// AutoCycle
bool     autoCycleActive = false;
bool     acPhaseHigh     = false;
uint32_t acPhaseStart    = 0;

// Button — proper 3-state machine
enum BtnState { BTN_IDLE, BTN_PRESSED, BTN_WAIT_DOUBLE };
BtnState btnState     = BTN_IDLE;
uint32_t btnStateTime = 0;
#define  BTN_LONG_MS   800
#define  BTN_DBL_MS    350

// ─── FORWARD DECLARATIONS ─────────────────────────────────────────────────────
void  loadState(); void saveState();
void  saveSchedules(); void saveAutoCycle();
void  startFan(uint8_t speed); void stopFan(); void setFanSpeed(uint8_t pct);
void  applyPwm(uint8_t logicalPct);
void  beginRamp(uint8_t fromPct, uint8_t toPct, bool kickstart);
void  tickRamp();
void  handleSchedules(); void handleAutoCycle(); void handleSleepTimer();
void  handleButton(); void setupWiFi(); void setupRoutes();
void  sendJson(int code, JsonDocument& doc); void handleCors();
bool  getTimeSafe(struct tm* out);

// ═══════════════════════════════════════════════════════════════════════════════
void setup() {
  Serial.begin(115200);
  delay(400);
  Serial.println("\n=== SmartFan v1.1 ===");

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(STATUS_LED_PIN, OUTPUT);
  digitalWrite(STATUS_LED_PIN, HIGH);  // active-low on C3

  // PWM — core 3.x API
  ledcAttach(FAN_PWM_PIN, PWM_FREQ, PWM_RESOLUTION);
  ledcWrite(FAN_PWM_PIN, 0);

  prefs.begin("smartfan", false);
  loadState();

  setupWiFi();

  // Start mDNS: http://smartfan.local
  if (MDNS.begin(MDNS_NAME)) {
    MDNS.addService("http", "tcp", 80);
    Serial.println("mDNS: http://smartfan.local");
  }

  if (!apMode) configTzTime(ntpTz, NTP_SERVER);

  setupRoutes();
  server.begin();

  Serial.printf("Free heap: %u\n", ESP.getFreeHeap());
  if (fanOn) startFan(targetSpeed);
  Serial.println("Ready.");
}

void loop() {
  server.handleClient();
  // MDNS.update();
  tickRamp();
  handleButton();
  handleSchedules();
  handleAutoCycle();
  handleSleepTimer();

  // LED blink — only if enabled
  if (ledEnabled) {
    static uint32_t ledTick = 0;
    uint32_t blink = apMode ? 150 : (fanOn ? 900 : 400);
    if (millis() - ledTick > blink) {
      ledTick = millis();
      digitalWrite(STATUS_LED_PIN, !digitalRead(STATUS_LED_PIN));
    }
  } else {
    digitalWrite(STATUS_LED_PIN, HIGH);  // active-low: HIGH = off
  }

  delay(5);
}

// ─── TIME ────────────────────────────────────────────────────────────────────
bool getTimeSafe(struct tm* out) {
  time_t now = time(nullptr);
  localtime_r(&now, out);
  return (out->tm_year >= (2020 - 1900));
}

// ─── PWM ─────────────────────────────────────────────────────────────────────
void applyPwm(uint8_t logicalPct) {
  currentPct = logicalPct;
  uint32_t duty = 0;
  if (logicalPct > 0) {
    float phys = PHYSICAL_FLOOR + (logicalPct * (100.0f - PHYSICAL_FLOOR) / 100.0f);
    duty = constrain((uint32_t)(phys * PWM_MAX / 100.0f), 0u, (uint32_t)PWM_MAX);
  }
  ledcWrite(FAN_PWM_PIN, duty);
}

// ─── FAN CONTROL ─────────────────────────────────────────────────────────────
void startFan(uint8_t speed) {
  speed = constrain(speed, MIN_RUN_SPEED, 100);
  targetSpeed = speed;
  fanOn = true;
  bool kick = (targetSpeed < KICKSTART_SPEED) && (currentPct == 0);
  beginRamp(currentPct, kick ? KICKSTART_SPEED : targetSpeed, kick);
  saveState();
}

void stopFan() {
  fanOn = false;
  targetSpeed = 0;
  beginRamp(currentPct, 0, false);
  saveState();
}

void setFanSpeed(uint8_t pct) {
  pct = constrain(pct, 0, 100);
  if (pct == 0)          { stopFan();    return; }
  if (!fanOn && pct > 0) { startFan(pct); return; }
  targetSpeed = pct;
  beginRamp(currentPct, targetSpeed, false);
  saveState();
}

// ─── ASYNC SOFT-START ────────────────────────────────────────────────────────
void beginRamp(uint8_t fromPct, uint8_t toPct, bool kickstart) {
  rampFrom   = fromPct;
  rampTo_    = toPct;
  rampStart_ = millis();
  if (fromPct == toPct && !kickstart) { applyPwm(toPct); rampPhase = RAMP_NONE; return; }
  rampPhase = kickstart ? RAMP_KICKSTART_UP : RAMP_MAIN;
}

void tickRamp() {
  if (rampPhase == RAMP_NONE) return;
  uint32_t elapsed = millis() - rampStart_;

  auto eased = [](float t) -> float {
    return t < 0.5f ? 4*t*t*t : 1.0f - (-2*t+2)*(-2*t+2)*(-2*t+2)/2.0f;
  };

  switch (rampPhase) {
    case RAMP_MAIN:
      if (elapsed >= SOFT_START_MS) {
        applyPwm(rampTo_);
        rampPhase = RAMP_NONE;
      } else {
        float e = eased((float)elapsed / SOFT_START_MS);
        int delta  = (int)rampTo_ - (int)rampFrom;
        int nextPct = (int)rampFrom + (int)(delta * e);
        applyPwm((uint8_t)constrain(nextPct, 0, 100));
      }
      break;

    case RAMP_KICKSTART_UP:
      if (elapsed >= SOFT_START_MS) {
        applyPwm(KICKSTART_SPEED);
        rampStart_ = millis();
        rampPhase  = RAMP_KICKSTART_HOLD;
      } else {
        float e = eased((float)elapsed / SOFT_START_MS);
        int delta  = (int)KICKSTART_SPEED - (int)rampFrom;
        int nextPct = (int)rampFrom + (int)(delta * e);
        applyPwm((uint8_t)constrain(nextPct, 0, 100));
      }
      break;

    case RAMP_KICKSTART_HOLD:
      if (elapsed >= KICKSTART_MS) {
        rampFrom   = KICKSTART_SPEED;
        rampTo_    = targetSpeed;
        rampStart_ = millis();
        rampPhase  = RAMP_MAIN;
      }
      break;

    default: break;
  }
}

// ─── SCHEDULES ───────────────────────────────────────────────────────────────
void handleSchedules() {
  static uint32_t lastCheck   = 0;
  static uint8_t  lastFiredMn = 255;
  if (millis() - lastCheck < 15000) return;
  lastCheck = millis();

  struct tm t;
  if (!getTimeSafe(&t)) return;
  uint8_t nowMn = t.tm_hour * 60 + t.tm_min;
  if (nowMn == lastFiredMn) return;

  for (int i = 0; i < MAX_SCHEDULES; i++) {
    if (!schedules[i].active) continue;
    if (schedules[i].hour == (uint8_t)t.tm_hour && schedules[i].minute == (uint8_t)t.tm_min) {
      lastFiredMn = nowMn;
      Serial.printf("Schedule[%d] fired action=%d\n", i, schedules[i].action);
      if      (schedules[i].action == 0) stopFan();
      else if (schedules[i].action == 1) startFan(defaultSpeed);
      else if (schedules[i].action == 2) setFanSpeed(schedules[i].speed);
    }
  }
}

// ─── AUTOCYCLE ───────────────────────────────────────────────────────────────
bool timeInRange(uint8_t sh, uint8_t sm, uint8_t eh, uint8_t em, uint8_t h, uint8_t m) {
  int s = sh*60+sm, e = eh*60+em, n = h*60+m;
  return (s <= e) ? (n >= s && n < e) : (n >= s || n < e);
}

void handleAutoCycle() {
  if (!autoCycle.enabled) { autoCycleActive = false; return; }
  static uint32_t lastCheck = 0;
  if (millis() - lastCheck < 10000) return;
  lastCheck = millis();

  struct tm t;
  if (!getTimeSafe(&t)) return;
  bool inW = timeInRange(autoCycle.startHour, autoCycle.startMinute,
                         autoCycle.endHour,   autoCycle.endMinute,
                         t.tm_hour, t.tm_min);
  if (!inW) {
    if (autoCycleActive) { autoCycleActive = false; if (fanOn) setFanSpeed(autoCycle.highSpeed); }
    return;
  }
  if (!autoCycleActive) {
    autoCycleActive = true; acPhaseHigh = false; acPhaseStart = millis();
    if (!fanOn) startFan(autoCycle.lowSpeed); else setFanSpeed(autoCycle.lowSpeed);
    return;
  }
  uint32_t dur = acPhaseHigh
    ? (uint32_t)autoCycle.highDuration * 60000UL
    : (uint32_t)autoCycle.slowDuration * 60000UL;
  if (millis() - acPhaseStart >= dur) {
    acPhaseHigh  = !acPhaseHigh;
    acPhaseStart = millis();
    setFanSpeed(acPhaseHigh ? autoCycle.highSpeed : autoCycle.lowSpeed);
  }
}

// ─── SLEEP TIMER ─────────────────────────────────────────────────────────────
void handleSleepTimer() {
  if (sleepDuration == 0) return;
  if (millis() - sleepStart >= sleepDuration) {
    sleepDuration = 0;
    Serial.println("Sleep timer fired");
    stopFan();
  }
}

// ─── BUTTON STATE MACHINE ────────────────────────────────────────────────────
void handleButton() {
  bool pressed = (digitalRead(BUTTON_PIN) == LOW);

  switch (btnState) {
    case BTN_IDLE:
      if (pressed) {
        btnState = BTN_PRESSED;
        btnStateTime = millis();
      }
      break;

    case BTN_PRESSED:
      if (pressed) {
        if (millis() - btnStateTime >= BTN_LONG_MS) {
          Serial.println("BTN: Hold → Toggle Motor");
          if (fanOn) stopFan();
          else startFan(defaultSpeed);
          while (digitalRead(BUTTON_PIN) == LOW) delay(5);
          btnState = BTN_IDLE;
        }
      } else {
        btnState = BTN_WAIT_DOUBLE;
        btnStateTime = millis();
      }
      break;

    case BTN_WAIT_DOUBLE:
      if (pressed) {
        // Double click: toggle WiFi (runtime only, not saved)
        Serial.println("BTN: Double Click → Toggle WiFi");
        while (digitalRead(BUTTON_PIN) == LOW) delay(5);
        if (WiFi.getMode() != WIFI_OFF) {
          server.stop();
          WiFi.disconnect(true);
          WiFi.mode(WIFI_OFF);
          Serial.println("WiFi Disabled (runtime only)");
        } else {
          setupWiFi();
          if (!apMode) configTzTime(ntpTz, NTP_SERVER);
          server.begin();
          Serial.println("WiFi Enabled");
        }
        btnState = BTN_IDLE;
      }
      else if (millis() - btnStateTime > BTN_DBL_MS) {
        Serial.println("BTN: Single Click → Cycle Preset");
        if (!fanOn) {
          setFanSpeed(20);
        } else {
          uint8_t nextSpeed = targetSpeed + 20;
          if (nextSpeed > 100) stopFan();
          else setFanSpeed(nextSpeed);
        }
        btnState = BTN_IDLE;
      }
      break;
  }
}

// ─── WIFI ────────────────────────────────────────────────────────────────────
// WiFi always starts on boot. STA with short timeout (3s), then AP.
void setupWiFi() {
  char ssid[64] = {}, pass[64] = {};
  prefs.getString("ssid", ssid, 64);
  prefs.getString("pass", pass, 64);

  if (strlen(ssid) > 0) {
    Serial.printf("WiFi → %s\n", ssid);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, pass);
    uint32_t t0 = millis();
    // Short 3-second timeout for faster boot
    while (WiFi.status() != WL_CONNECTED && millis() - t0 < 3000) {
      delay(100); Serial.print(".");
    }
    if (WiFi.status() == WL_CONNECTED) {
      apMode = false;
      Serial.printf("\nIP: %s\n", WiFi.localIP().toString().c_str());
      return;
    }
    Serial.println("\nNo STA connection — starting AP");
  }
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASS);
  apMode = true;
  Serial.printf("AP: %s  %s\n", AP_SSID, WiFi.softAPIP().toString().c_str());
}

// ─── PERSISTENCE ─────────────────────────────────────────────────────────────
void loadState() {
  fanOn        = prefs.getBool("fanOn",  false);
  targetSpeed  = prefs.getUChar("speed", DEFAULT_SPEED);
  defaultSpeed = prefs.getUChar("defSpd", DEFAULT_SPEED);
  ledEnabled   = prefs.getBool("ledEn",  true);
  prefs.getString("ntpTz", ntpTz, sizeof(ntpTz));
  if (strlen(ntpTz) == 0) strlcpy(ntpTz, NTP_TZ_DEFAULT, sizeof(ntpTz));

  for (int i = 0; i < MAX_SCHEDULES; i++) {
    char key[12]; sprintf(key, "sched%d", i);
    size_t sz = prefs.getBytesLength(key);
    if (sz == sizeof(Schedule)) prefs.getBytes(key, &schedules[i], sz);
    else memset(&schedules[i], 0, sizeof(Schedule));
  }
  size_t ns = prefs.getBytesLength("autoCycle");
  if (ns == sizeof(AutoCycle)) prefs.getBytes("autoCycle", &autoCycle, ns);

  Serial.printf("Loaded: fanOn=%d speed=%d defSpd=%d ledEn=%d tz=%s\n",
                fanOn, targetSpeed, defaultSpeed, ledEnabled, ntpTz);
}

void saveState()     { prefs.putBool("fanOn", fanOn); prefs.putUChar("speed", targetSpeed); }
void saveSchedules() {
  for (int i = 0; i < MAX_SCHEDULES; i++) {
    char key[12]; sprintf(key, "sched%d", i);
    prefs.putBytes(key, &schedules[i], sizeof(Schedule));
  }
}
void saveAutoCycle() { prefs.putBytes("autoCycle", &autoCycle, sizeof(AutoCycle)); }

// ─── HTTP HELPERS ────────────────────────────────────────────────────────────
void sendJson(int code, JsonDocument& doc) {
  String out;
  serializeJson(doc, out);
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET,POST,DELETE,OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
  server.send(code, "application/json", out);
}

void handleCors() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET,POST,DELETE,OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
  server.send(204);
}

// ─── ROUTES ──────────────────────────────────────────────────────────────────
void setupRoutes() {

  server.onNotFound([]() {
    if (server.method() == HTTP_OPTIONS) { handleCors(); return; }
    server.send(404, "text/plain", "Not found");
  });

  // GET /api/status
  server.on("/api/status", HTTP_GET, []() {
    StaticJsonDocument<384> doc;
    doc["fanOn"]            = fanOn;
    doc["speed"]            = targetSpeed;
    doc["currentSpeed"]     = currentPct;
    doc["ramping"]          = (rampPhase != RAMP_NONE);
    doc["apMode"]           = apMode;
    doc["autoCycleActive"]  = autoCycleActive;
    doc["defaultSpeed"]     = defaultSpeed;
    doc["ledEnabled"]       = ledEnabled;
    doc["ntpTz"]            = ntpTz;
    if (sleepDuration > 0) {
      uint32_t el = millis() - sleepStart;
      doc["sleepRemaining"] = el < sleepDuration ? (int32_t)((sleepDuration - el) / 1000) : 0;
    } else {
      doc["sleepRemaining"] = 0;
    }
    struct tm t;
    if (getTimeSafe(&t)) {
      char buf[9]; strftime(buf, sizeof(buf), "%H:%M:%S", &t);
      doc["time"] = buf;
    }
    doc["uptime"] = millis() / 1000;
    sendJson(200, doc);
  });

  // POST /api/fan
  server.on("/api/fan", HTTP_POST, []() {
    StaticJsonDocument<128> req;
    if (deserializeJson(req, server.arg("plain"))) {
      server.send(400, "application/json", "{\"error\":\"bad json\"}"); return;
    }
    if (req.containsKey("on")) {
      bool on = req["on"].as<bool>();
      if (on) startFan(defaultSpeed);
      else    stopFan();
    } else if (req.containsKey("speed")) {
      setFanSpeed(req["speed"].as<uint8_t>());
    }
    StaticJsonDocument<64> res;
    res["ok"] = true; res["fanOn"] = fanOn; res["speed"] = targetSpeed;
    sendJson(200, res);
  });

  // GET /api/schedules
  server.on("/api/schedules", HTTP_GET, []() {
    DynamicJsonDocument doc(2048);
    JsonArray arr = doc.to<JsonArray>();
    for (int i = 0; i < MAX_SCHEDULES; i++) {
      JsonObject o = arr.createNestedObject();
      o["id"]     = i;
      o["active"] = schedules[i].active;
      o["hour"]   = schedules[i].hour;
      o["minute"] = schedules[i].minute;
      o["action"] = schedules[i].action;
      o["speed"]  = schedules[i].speed;
      o["label"]  = schedules[i].label;
    }
    sendJson(200, doc);
  });

  // POST /api/schedules
  server.on("/api/schedules", HTTP_POST, []() {
    StaticJsonDocument<256> req;
    if (deserializeJson(req, server.arg("plain"))) {
      server.send(400, "application/json", "{\"error\":\"bad json\"}"); return;
    }
    int id = req["id"] | -1;
    if (id < 0 || id >= MAX_SCHEDULES) {
      id = -1;
      for (int i = 0; i < MAX_SCHEDULES; i++) { if (!schedules[i].active) { id = i; break; } }
    }
    if (id < 0) { server.send(400, "application/json", "{\"error\":\"full\"}"); return; }
    schedules[id].active = true;
    schedules[id].hour   = req["hour"]   | 0;
    schedules[id].minute = req["minute"] | 0;
    schedules[id].action = req["action"] | 1;
    schedules[id].speed  = req["speed"]  | 50;
    strlcpy(schedules[id].label, req["label"] | "", 32);
    saveSchedules();
    StaticJsonDocument<32> res; res["ok"] = true; res["id"] = id;
    sendJson(200, res);
  });

  // DELETE /api/schedules?id=N
  server.on("/api/schedules", HTTP_DELETE, []() {
    int id = server.arg("id").toInt();
    if (id < 0 || id >= MAX_SCHEDULES) {
      server.send(400, "application/json", "{\"error\":\"invalid id\"}"); return;
    }
    memset(&schedules[id], 0, sizeof(Schedule));
    saveSchedules();
    StaticJsonDocument<16> res; res["ok"] = true;
    sendJson(200, res);
  });

  // GET /api/autocycle
  server.on("/api/autocycle", HTTP_GET, []() {
    StaticJsonDocument<256> doc;
    doc["enabled"]      = autoCycle.enabled;
    doc["startHour"]    = autoCycle.startHour;
    doc["startMinute"]  = autoCycle.startMinute;
    doc["endHour"]      = autoCycle.endHour;
    doc["endMinute"]    = autoCycle.endMinute;
    doc["lowSpeed"]     = autoCycle.lowSpeed;
    doc["highSpeed"]    = autoCycle.highSpeed;
    doc["slowDuration"] = autoCycle.slowDuration;
    doc["highDuration"] = autoCycle.highDuration;
    doc["active"]       = autoCycleActive;
    sendJson(200, doc);
  });

  // POST /api/autocycle
  server.on("/api/autocycle", HTTP_POST, []() {
    StaticJsonDocument<256> req;
    if (deserializeJson(req, server.arg("plain"))) {
      server.send(400, "application/json", "{\"error\":\"bad json\"}"); return;
    }
    if (req.containsKey("enabled"))      autoCycle.enabled      = req["enabled"];
    if (req.containsKey("startHour"))    autoCycle.startHour    = req["startHour"];
    if (req.containsKey("startMinute"))  autoCycle.startMinute  = req["startMinute"];
    if (req.containsKey("endHour"))      autoCycle.endHour      = req["endHour"];
    if (req.containsKey("endMinute"))    autoCycle.endMinute    = req["endMinute"];
    if (req.containsKey("lowSpeed"))     autoCycle.lowSpeed     = req["lowSpeed"];
    if (req.containsKey("highSpeed"))    autoCycle.highSpeed    = req["highSpeed"];
    if (req.containsKey("slowDuration")) autoCycle.slowDuration = req["slowDuration"];
    if (req.containsKey("highDuration")) autoCycle.highDuration = req["highDuration"];
    saveAutoCycle();
    StaticJsonDocument<16> res; res["ok"] = true;
    sendJson(200, res);
  });

  // POST /api/sleep  {"minutes": 30}  — 0 to cancel
  server.on("/api/sleep", HTTP_POST, []() {
    StaticJsonDocument<64> req;
    if (deserializeJson(req, server.arg("plain"))) {
      server.send(400, "application/json", "{\"error\":\"bad json\"}"); return;
    }
    int mins = req["minutes"] | 0;
    if (mins > 0) {
      sleepStart    = millis();
      sleepDuration = (uint32_t)mins * 60000UL;
    } else {
      sleepDuration = 0;
    }
    StaticJsonDocument<32> res; res["ok"] = true; res["minutes"] = mins;
    sendJson(200, res);
  });

  // POST /api/wifi  {"ssid":"x","pass":"y"}
  server.on("/api/wifi", HTTP_POST, []() {
    StaticJsonDocument<200> req;
    if (deserializeJson(req, server.arg("plain"))) {
      server.send(400, "application/json", "{\"error\":\"bad json\"}"); return;
    }
    const char* ssid = req["ssid"] | "";
    if (strlen(ssid) == 0) {
      server.send(400, "application/json", "{\"error\":\"ssid required\"}"); return;
    }
    prefs.putString("ssid", ssid);
    prefs.putString("pass", req["pass"] | "");
    StaticJsonDocument<32> res; res["ok"] = true; res["restarting"] = true;
    sendJson(200, res);
    delay(800);
    ESP.restart();
  });

  // GET /api/wifi/status
  server.on("/api/wifi/status", HTTP_GET, []() {
    StaticJsonDocument<128> doc;
    doc["apMode"]  = apMode;
    doc["ssid"]    = apMode ? AP_SSID : WiFi.SSID();
    doc["ip"]      = apMode ? WiFi.softAPIP().toString() : WiFi.localIP().toString();
    doc["rssi"]    = apMode ? 0 : WiFi.RSSI();
    sendJson(200, doc);
  });

  // POST /api/settings  — default speed, LED, NTP timezone
  server.on("/api/settings", HTTP_POST, []() {
    StaticJsonDocument<200> req;
    if (deserializeJson(req, server.arg("plain"))) {
      server.send(400, "application/json", "{\"error\":\"bad json\"}"); return;
    }
    bool ntpChanged = false;
    if (req.containsKey("defaultSpeed")) {
      defaultSpeed = constrain((uint8_t)(req["defaultSpeed"] | DEFAULT_SPEED), 1, 100);
      prefs.putUChar("defSpd", defaultSpeed);
    }
    if (req.containsKey("ledEnabled")) {
      ledEnabled = req["ledEnabled"].as<bool>();
      prefs.putBool("ledEn", ledEnabled);
    }
    if (req.containsKey("ntpTz")) {
      const char* tz = req["ntpTz"] | NTP_TZ_DEFAULT;
      strlcpy(ntpTz, tz, sizeof(ntpTz));
      prefs.putString("ntpTz", ntpTz);
      ntpChanged = true;
    }
    if (ntpChanged && !apMode) {
      configTzTime(ntpTz, NTP_SERVER);
    }
    StaticJsonDocument<32> res; res["ok"] = true;
    sendJson(200, res);
  });

  // GET / — serve web UI
  server.on("/", HTTP_GET, []() {
    extern const char webUI[] PROGMEM;
    server.send_P(200, "text/html", webUI);
  });
}

#include "webui.h"