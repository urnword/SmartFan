// webui.h — SmartFan v1.1 Web UI (PROGMEM)
// Changes: AutoCycle (was Night Mode), Settings: defaultSpeed/LED/NTP, fixed slider jitter

const char webUI[] PROGMEM = R"HTMLEOF(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1">
<title>SmartFan</title>
<style>
@import url('https://fonts.googleapis.com/css2?family=Space+Mono:wght@400;700&family=DM+Sans:wght@300;400;500;600&display=swap');

:root {
  --bg:       #0d0f14;
  --surf:     #161920;
  --surf2:    #1e2230;
  --border:   #252a38;
  --accent:   #00d4ff;
  --purple:   #7c3aed;
  --warn:     #f59e0b;
  --danger:   #ef4444;
  --ok:       #22c55e;
  --text:     #e8eaf2;
  --muted:    #6b7280;
  --r:        14px;
}
*{box-sizing:border-box;margin:0;padding:0}
body{background:var(--bg);color:var(--text);font-family:'DM Sans',sans-serif;min-height:100vh;padding-bottom:60px}

header{background:var(--surf);border-bottom:1px solid var(--border);padding:14px 20px;display:flex;align-items:center;justify-content:space-between;position:sticky;top:0;z-index:100}
header h1{font-family:'Space Mono',monospace;font-size:1rem;color:var(--accent);letter-spacing:.06em}
.hdr-right{display:flex;align-items:center;gap:10px}
.time{font-family:'Space Mono',monospace;font-size:.8rem;color:var(--muted)}
.badge{font-size:.65rem;padding:3px 8px;border-radius:999px;font-family:'Space Mono',monospace}
.badge.ap{background:rgba(245,158,11,.12);color:var(--warn);border:1px solid rgba(245,158,11,.3)}
.badge.sta{background:rgba(34,197,94,.1);color:var(--ok);border:1px solid rgba(34,197,94,.2)}

.tabs{display:flex;background:var(--surf);border-bottom:1px solid var(--border);overflow-x:auto;scrollbar-width:none}
.tabs::-webkit-scrollbar{display:none}
.tab{padding:13px 18px;font-size:.8rem;font-weight:500;color:var(--muted);cursor:pointer;white-space:nowrap;border-bottom:2px solid transparent;transition:all .2s;letter-spacing:.03em}
.tab.active{color:var(--accent);border-bottom-color:var(--accent)}

main{max-width:520px;margin:0 auto;padding:20px 14px}
.panel{display:none}.panel.active{display:block}

.card{background:var(--surf);border:1px solid var(--border);border-radius:var(--r);padding:18px;margin-bottom:14px}
.ctitle{font-size:.65rem;text-transform:uppercase;letter-spacing:.12em;color:var(--muted);margin-bottom:14px;font-family:'Space Mono',monospace}

/* Power */
.power-wrap{display:flex;flex-direction:column;align-items:center;gap:18px;padding:8px 0}
.power-btn{width:90px;height:90px;border-radius:50%;border:2px solid var(--border);background:var(--surf2);cursor:pointer;position:relative;transition:all .3s;display:flex;align-items:center;justify-content:center}
.power-btn svg{width:32px;height:32px;transition:all .3s}
.power-btn.on{border-color:var(--accent);background:rgba(0,212,255,.08);box-shadow:0 0 20px rgba(0,212,255,.18),inset 0 0 20px rgba(0,212,255,.04)}
.power-btn.on svg{color:var(--accent);filter:drop-shadow(0 0 5px var(--accent))}
.power-btn.off svg{color:var(--muted)}
.fan-ring{position:absolute;width:106px;height:106px;border-radius:50%;border:1px solid transparent;top:-10px;left:-10px;animation:spin 2.5s linear infinite;opacity:0;transition:opacity .3s}
.power-btn.on .fan-ring{border-top-color:rgba(0,212,255,.5);border-right-color:rgba(0,212,255,.2);opacity:1}
@keyframes spin{to{transform:rotate(360deg)}}
.fan-status{font-family:'Space Mono',monospace;font-size:.82rem;color:var(--muted)}
.fan-status b{color:var(--text)}

/* Speed */
.speed-num{text-align:center;margin-bottom:14px}
.speed-big{font-family:'Space Mono',monospace;font-size:2.8rem;font-weight:700;color:var(--accent);line-height:1}
.speed-lbl{font-size:.7rem;color:var(--muted);margin-top:3px}

.gauge-wrap{display:flex;justify-content:center;margin-bottom:10px}

input[type=range]{-webkit-appearance:none;width:100%;height:6px;border-radius:3px;background:var(--surf2);outline:none;cursor:pointer}
input[type=range]::-webkit-slider-runnable-track{height:6px;border-radius:3px;background:linear-gradient(to right,var(--accent) var(--val,50%),var(--surf2) var(--val,50%))}
input[type=range]::-webkit-slider-thumb{-webkit-appearance:none;width:20px;height:20px;border-radius:50%;background:var(--accent);margin-top:-7px;box-shadow:0 0 8px rgba(0,212,255,.5);cursor:grab}
input[type=range]:active::-webkit-slider-thumb{cursor:grabbing}

.presets{display:flex;gap:6px;margin-top:12px}
.preset{flex:1;padding:7px 4px;border-radius:7px;border:1px solid var(--border);background:var(--surf2);color:var(--muted);font-size:.75rem;font-family:'Space Mono',monospace;cursor:pointer;transition:all .2s;text-align:center}
.preset:hover{border-color:var(--accent);color:var(--accent)}

/* Sleep */
.sleep-grid{display:grid;grid-template-columns:repeat(4,1fr);gap:7px;margin-bottom:10px}
.sleep-btn{padding:9px 4px;border-radius:8px;border:1px solid var(--border);background:var(--surf2);color:var(--muted);font-size:.72rem;font-family:'Space Mono',monospace;cursor:pointer;transition:all .2s;text-align:center}
.sleep-btn:hover{border-color:var(--purple);color:var(--purple)}
.sleep-btn.active{background:rgba(124,58,237,.12);border-color:var(--purple);color:var(--purple)}
.sleep-cd{text-align:center;font-family:'Space Mono',monospace;font-size:1.15rem;color:var(--purple);padding:8px 0;display:none}

/* Forms */
.frow{display:grid;grid-template-columns:1fr 1fr;gap:9px;margin-bottom:10px}
.frow.three{grid-template-columns:1fr 1fr 1fr}
label{display:block;font-size:.65rem;color:var(--muted);margin-bottom:4px;letter-spacing:.06em;text-transform:uppercase}
input[type=number],input[type=text],input[type=password],select{width:100%;background:var(--surf2);border:1px solid var(--border);color:var(--text);border-radius:7px;padding:8px 10px;font-size:.85rem;font-family:'DM Sans',sans-serif;outline:none;transition:border-color .2s}
input:focus,select:focus{border-color:var(--accent)}
select option{background:var(--surf2)}

.btn{display:inline-flex;align-items:center;justify-content:center;gap:6px;padding:9px 16px;border-radius:8px;border:none;font-size:.82rem;font-family:'DM Sans',sans-serif;font-weight:500;cursor:pointer;transition:all .2s;width:100%}
.btn-accent{background:var(--accent);color:#000}.btn-accent:hover{background:#33ddff}
.btn-ghost{background:var(--surf2);color:var(--text);border:1px solid var(--border)}.btn-ghost:hover{border-color:var(--accent)}
.btn-danger{background:rgba(239,68,68,.1);color:var(--danger);border:1px solid rgba(239,68,68,.2)}
.btn-purple{background:var(--purple);color:#fff}.btn-purple:hover{background:#9333ea}

/* Toggle */
.toggle-row{display:flex;align-items:center;justify-content:space-between;margin-bottom:14px}
.toggle-lbl{font-size:.9rem;font-weight:500}
.toggle-sub{font-size:.72rem;color:var(--muted);margin-top:2px}
.toggle{width:44px;height:24px;border-radius:12px;background:var(--surf2);border:1px solid var(--border);position:relative;cursor:pointer;transition:background .2s;flex-shrink:0}
.toggle::after{content:'';position:absolute;width:18px;height:18px;background:var(--muted);border-radius:50%;top:2px;left:2px;transition:all .2s}
.toggle.on{background:rgba(0,212,255,.15);border-color:var(--accent)}
.toggle.on::after{background:var(--accent);transform:translateX(20px)}

/* Schedule list */
.sched-list{display:flex;flex-direction:column;gap:7px;margin-bottom:14px}
.sched-item{background:var(--surf2);border:1px solid var(--border);border-radius:9px;padding:11px 13px;display:flex;align-items:center;gap:11px}
.sched-time{font-family:'Space Mono',monospace;font-size:.95rem;font-weight:700;min-width:48px}
.sched-info{flex:1}
.sched-name{font-size:.83rem;margin-bottom:2px}
.sched-tag{font-size:.68rem;padding:2px 7px;border-radius:999px;font-family:'Space Mono',monospace}
.tag-on{background:rgba(34,197,94,.1);color:var(--ok)}
.tag-off{background:rgba(239,68,68,.1);color:var(--danger)}
.tag-spd{background:rgba(0,212,255,.1);color:var(--accent)}
.del{background:none;border:none;color:var(--muted);cursor:pointer;font-size:1rem;padding:3px;line-height:1}
.del:hover{color:var(--danger)}

/* Collapsible */
.coll-hdr{display:flex;align-items:center;justify-content:space-between;cursor:pointer;padding:2px 0}
.coll-hdr .arr{transition:transform .2s;color:var(--muted);font-size:.7rem}
.coll-hdr.open .arr{transform:rotate(180deg)}
.coll-body{display:none;padding-top:14px}
.coll-body.open{display:block}

/* AutoCycle params */
.ac-p{display:none}.ac-p.show{display:block}

/* WiFi card */
.ip-box{font-family:'Space Mono',monospace;font-size:1rem;color:var(--accent);text-align:center;padding:11px;background:var(--surf2);border-radius:8px;margin-bottom:10px}
.rssi-bar{height:4px;background:var(--surf2);border-radius:2px;overflow:hidden;margin-top:6px}
.rssi-fill{height:100%;background:var(--ok);border-radius:2px;transition:width 1s}

/* Status bar */
.sbar{position:fixed;bottom:0;left:0;right:0;background:var(--surf);border-top:1px solid var(--border);padding:9px 20px;display:flex;align-items:center;justify-content:space-between;font-size:.68rem;color:var(--muted);font-family:'Space Mono',monospace;z-index:100}
.sdot{width:7px;height:7px;border-radius:50%;display:inline-block;margin-right:5px}
.sdot.on{background:var(--ok);box-shadow:0 0 5px var(--ok);animation:pulse 2s infinite}
.sdot.off{background:var(--muted)}
@keyframes pulse{0%,100%{opacity:1}50%{opacity:.4}}

/* Toast */
.toast{position:fixed;bottom:56px;left:50%;transform:translateX(-50%) translateY(12px);background:var(--surf2);border:1px solid var(--border);border-radius:9px;padding:9px 16px;font-size:.78rem;opacity:0;transition:all .25s;pointer-events:none;z-index:200;white-space:nowrap}
.toast.show{opacity:1;transform:translateX(-50%) translateY(0)}

/* Info rows */
.info-grid{display:grid;grid-template-columns:1fr 1fr;gap:10px;font-size:.82rem}
.info-lbl{font-size:.65rem;color:var(--muted);margin-bottom:3px}
.info-val{font-family:'Space Mono',monospace}

.api-list{font-family:'Space Mono',monospace;font-size:.65rem;color:var(--muted);line-height:2}

/* Settings sections */
.settings-section{margin-bottom:6px}

@media(max-width:360px){.sleep-grid{grid-template-columns:repeat(3,1fr)}.speed-big{font-size:2.3rem}}
</style>
</head>
<body>

<header>
  <h1>⚡ SMARTFAN</h1>
  <div class="hdr-right">
    <span class="time" id="hdrTime">--:--:--</span>
    <span class="badge ap" id="wifiBadge">AP</span>
  </div>
</header>

<div class="tabs">
  <div class="tab active"  data-tab="control"   onclick="switchTab('control')">Control</div>
  <div class="tab"         data-tab="schedule"  onclick="switchTab('schedule')">Schedule</div>
  <div class="tab"         data-tab="autocycle" onclick="switchTab('autocycle')">AutoCycle</div>
  <div class="tab"         data-tab="settings"  onclick="switchTab('settings')">Settings</div>
</div>

<main>

<!-- ═══════ CONTROL ═══════ -->
<div class="panel active" id="tab-control">

  <div class="card">
    <div class="power-wrap">
      <button class="power-btn off" id="powerBtn" onclick="togglePower()">
        <div class="fan-ring"></div>
        <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round">
          <path d="M18.36 6.64a9 9 0 1 1-12.73 0"/>
          <line x1="12" y1="2" x2="12" y2="12"/>
        </svg>
      </button>
      <div class="fan-status">Fan is <b id="fanStatusTxt">OFF</b></div>
    </div>
  </div>

  <div class="card">
    <div class="ctitle">Fan Speed</div>
    <div class="speed-num">
      <div class="speed-big" id="speedBig">50</div>
      <div class="speed-lbl">PERCENT</div>
    </div>

    <!-- SVG arc gauge -->
    <div class="gauge-wrap">
      <svg width="150" height="78" viewBox="0 0 150 78" overflow="visible">
        <path d="M 12 75 A 63 63 0 0 1 138 75" stroke="#1e2230" stroke-width="11" fill="none" stroke-linecap="round"/>
        <path id="gArc" d="M 12 75 A 63 63 0 0 1 138 75" stroke="#00d4ff" stroke-width="11" fill="none"
              stroke-linecap="round" stroke-dasharray="0 198" style="transition:stroke-dasharray .3s,stroke .3s"/>
      </svg>
    </div>

    <input type="range" id="speedSlider" min="0" max="100" value="50"
           oninput="sliderInput(this.value)" onchange="sliderCommit(this.value)"
           style="--val:50%">

    <div class="presets">
      <div class="preset" onclick="setPreset(20)">20%</div>
      <div class="preset" onclick="setPreset(40)">40%</div>
      <div class="preset" onclick="setPreset(60)">60%</div>
      <div class="preset" onclick="setPreset(80)">80%</div>
      <div class="preset" onclick="setPreset(100)">MAX</div>
    </div>
  </div>

  <!-- Sleep Timer -->
  <div class="card">
    <div class="ctitle">Sleep Timer</div>
    <div class="sleep-grid">
      <div class="sleep-btn" data-min="15"  onclick="setSleep(15)">15m</div>
      <div class="sleep-btn" data-min="30"  onclick="setSleep(30)">30m</div>
      <div class="sleep-btn" data-min="60"  onclick="setSleep(60)">1h</div>
      <div class="sleep-btn" data-min="120" onclick="setSleep(120)">2h</div>
    </div>
    <div class="sleep-cd" id="sleepCd"></div>
    <button class="btn btn-ghost" id="cancelSleepBtn" style="display:none;margin-top:6px" onclick="cancelSleep()">Cancel Timer</button>
  </div>

</div>

<!-- ═══════ SCHEDULE ═══════ -->
<div class="panel" id="tab-schedule">

  <div class="card">
    <div class="ctitle">Active Schedules</div>
    <div class="sched-list" id="schedList">
      <div style="color:var(--muted);font-size:.83rem;text-align:center;padding:14px">No schedules yet</div>
    </div>
  </div>

  <div class="card">
    <div class="coll-hdr" onclick="toggleColl(this)">
      <div class="ctitle" style="margin:0">Add Schedule</div>
      <span class="arr">▼</span>
    </div>
    <div class="coll-body">
      <div class="frow" style="margin-top:14px">
        <div><label>Hour (0-23)</label><input type="number" id="sHour" min="0" max="23" value="8"></div>
        <div><label>Minute</label><input type="number" id="sMin" min="0" max="59" value="0"></div>
      </div>
      <div class="frow">
        <div>
          <label>Action</label>
          <select id="sAction" onchange="toggleSchedSpd()">
            <option value="1">Turn ON</option>
            <option value="0">Turn OFF</option>
            <option value="2">Set Speed</option>
          </select>
        </div>
        <div id="sSpdWrap" style="display:none"><label>Speed %</label><input type="number" id="sSpeed" min="0" max="100" value="50"></div>
      </div>
      <div><label>Label (optional)</label><input type="text" id="sLabel" placeholder="Morning, bedtime..."></div>
      <button class="btn btn-accent" style="margin-top:12px" onclick="addSchedule()">Add Schedule</button>
    </div>
  </div>

</div>

<!-- ═══════ AUTOCYCLE ═══════ -->
<div class="panel" id="tab-autocycle">

  <div class="card">
    <div class="toggle-row">
      <div>
        <div class="toggle-lbl">AutoCycle</div>
        <div class="toggle-sub">Auto-cycle fan speed on a schedule</div>
      </div>
      <div class="toggle" id="acToggle" onclick="toggleAC()"></div>
    </div>
    <div class="ac-p" id="acP">
      <div class="frow">
        <div><label>Start Time</label><input type="text" id="acStart" placeholder="22:00" value="22:00"></div>
        <div><label>End Time</label><input type="text" id="acEnd" placeholder="07:00" value="07:00"></div>
      </div>
      <p style="font-size:.72rem;color:var(--muted);margin:8px 0 4px;font-family:'Space Mono',monospace">SPEED PHASES</p>
      <div class="frow">
        <div><label>Low speed %</label><input type="number" id="acLow" min="0" max="100" value="30"></div>
        <div><label>High speed %</label><input type="number" id="acHigh" min="0" max="100" value="100"></div>
      </div>
      <p style="font-size:.72rem;color:var(--muted);margin:8px 0 4px;font-family:'Space Mono',monospace">PHASE DURATIONS</p>
      <div class="frow">
        <div><label>Low for (min)</label><input type="number" id="acSlowDur" min="1" value="30"></div>
        <div><label>High for (min)</label><input type="number" id="acHighDur" min="1" value="10"></div>
      </div>
    </div>
    <button class="btn btn-purple" style="margin-top:10px" onclick="saveAC()">Save AutoCycle</button>
  </div>

  <div class="card" id="acStatus" style="display:none">
    <div class="ctitle">AutoCycle Active</div>
    <div style="text-align:center;font-family:'Space Mono',monospace;font-size:.85rem;color:var(--purple)" id="acPhaseTxt">—</div>
  </div>

</div>

<!-- ═══════ SETTINGS ═══════ -->
<div class="panel" id="tab-settings">

  <!-- WiFi Status -->
  <div class="card">
    <div class="ctitle">WiFi</div>
    <div class="ip-box" id="ipBox">---</div>
    <div class="rssi-bar"><div class="rssi-fill" id="rssiFill" style="width:0%"></div></div>
    <p style="font-size:.72rem;color:var(--muted);text-align:center;margin-top:6px" id="wifiInfoTxt">Checking...</p>
    <p style="font-size:.68rem;color:var(--muted);text-align:center;margin-top:4px;font-family:'Space Mono',monospace">
      Also reachable at <span style="color:var(--accent)">http://smartfan.local</span>
    </p>
  </div>

  <!-- Change WiFi -->
  <div class="card">
    <div class="coll-hdr" onclick="toggleColl(this)">
      <div class="ctitle" style="margin:0">Change WiFi Network</div>
      <span class="arr">▼</span>
    </div>
    <div class="coll-body">
      <p style="font-size:.78rem;color:var(--muted);margin:14px 0 12px">Device restarts after saving.</p>
      <div style="margin-bottom:10px"><label>SSID</label><input type="text" id="wSSID" placeholder="Network name"></div>
      <div style="margin-bottom:12px"><label>Password</label><input type="password" id="wPass" placeholder="Password"></div>
      <button class="btn btn-accent" onclick="saveWifi()">Save & Restart</button>
    </div>
  </div>

  <!-- Fan Defaults -->
  <div class="card">
    <div class="ctitle">Fan Defaults</div>
    <div class="settings-section">
      <label>Default Speed % (used on power-on)</label>
      <input type="number" id="setDefSpd" min="1" max="100" value="50" style="margin-top:4px">
      <p style="font-size:.68rem;color:var(--muted);margin-top:4px">Fan will start at this speed when turned on via button, schedule, or API.</p>
    </div>
  </div>

  <!-- LED -->
  <div class="card">
    <div class="ctitle">Status LED</div>
    <div class="toggle-row" style="margin-bottom:0">
      <div>
        <div class="toggle-lbl">LED Blinking</div>
        <div class="toggle-sub">Pin 8 — blinks to indicate fan state</div>
      </div>
      <div class="toggle on" id="ledToggle" onclick="toggleLED()"></div>
    </div>
  </div>

  <!-- NTP Timezone -->
  <div class="card">
    <div class="ctitle">Clock &amp; Timezone</div>
    <div style="margin-bottom:10px">
      <label>POSIX Timezone String</label>
      <input type="text" id="setTz" placeholder="e.g. MYT-8 or EST5EDT,M3.2.0,M11.1.0" style="margin-top:4px">
      <p style="font-size:.68rem;color:var(--muted);margin-top:5px">
        Common: MYT-8 · SGT-8 · CST-8 · JST-9 · UTC0 · EST5EDT,M3.2.0,M11.1.0
      </p>
    </div>
  </div>

  <!-- Save Settings button -->
  <button class="btn btn-accent" onclick="saveSettings()" style="margin-bottom:14px">Save Settings</button>

  <!-- Device Info -->
  <div class="card">
    <div class="ctitle">Device</div>
    <div class="info-grid">
      <div><div class="info-lbl">UPTIME</div><div class="info-val" id="uptimeTxt">—</div></div>
      <div><div class="info-lbl">FIRMWARE</div><div class="info-val">v1.1</div></div>
      <div><div class="info-lbl">mDNS</div><div class="info-val" style="color:var(--accent)">smartfan.local</div></div>
      <div><div class="info-lbl">NTP</div><div class="info-val" id="ntpTxt">—</div></div>
    </div>
  </div>

  <!-- API Reference -->
  <div class="card">
    <div class="ctitle">REST API</div>
    <div class="api-list">
      GET  /api/status<br>
      POST /api/fan       {"on":true}<br>
      POST /api/fan       {"speed":75}<br>
      GET  /api/schedules<br>
      POST /api/schedules<br>
      DELETE /api/schedules?id=N<br>
      GET  /api/autocycle<br>
      POST /api/autocycle<br>
      POST /api/sleep     {"minutes":30}<br>
      GET  /api/wifi/status<br>
      POST /api/wifi      {"ssid":"x","pass":"y"}<br>
      POST /api/settings  {"defaultSpeed":50,"ledEnabled":true,"ntpTz":"MYT-8"}
    </div>
  </div>

</div>
</main>

<!-- Status bar -->
<div class="sbar">
  <div><span class="sdot off" id="sdot"></span><span id="sTxt">Connecting...</span></div>
  <div style="color:var(--accent)" id="sSpd">— %</div>
</div>

<div class="toast" id="toast"></div>

<script>
const BASE = '';

let st = {};
let sleepTick = null;
let acOn = false;

// ── Slider drag state ─────────────────────────────────────────────────────────
// True while user is actively dragging the slider — suppresses poll overwrite
let sliderDragging = false;

// ── API ──────────────────────────────────────────────────────────────────────
async function api(path, method='GET', body=null) {
  const o = { method, headers:{'Content-Type':'application/json'} };
  if (body) o.body = JSON.stringify(body);
  const r = await fetch(BASE + path, o);
  if (!r.ok) throw new Error(r.status);
  return r.json();
}

function toast(msg, ms=2200) {
  const el = document.getElementById('toast');
  el.textContent = msg;
  el.classList.add('show');
  clearTimeout(el._t);
  el._t = setTimeout(() => el.classList.remove('show'), ms);
}

// ── Poll ─────────────────────────────────────────────────────────────────────
async function poll() {
  try {
    const s = await api('/api/status');
    st = s;
    applyStatus(s);
  } catch(e) {
    document.getElementById('sTxt').textContent = 'Offline';
  }
}

function applyStatus(s) {
  // Power button
  const pb = document.getElementById('powerBtn');
  pb.className = 'power-btn ' + (s.fanOn ? 'on' : 'off');
  document.getElementById('fanStatusTxt').textContent = s.fanOn ? 'ON' : 'OFF';

  // Speed display — only update slider/number when user is NOT dragging
  const spd = s.speed;
  if (!sliderDragging) {
    document.getElementById('speedBig').textContent = spd;
    const sl = document.getElementById('speedSlider');
    sl.value = spd;
    sl.style.setProperty('--val', spd + '%');
    updateGauge(spd);  // gauge tracks targetSpeed (stable), not currentSpeed
  }

  // Status bar
  const dot = document.getElementById('sdot');
  dot.className = 'sdot ' + (s.fanOn ? 'on' : 'off');
  document.getElementById('sTxt').textContent =
    s.fanOn ? (s.ramping ? 'Ramping…' : 'Running') : 'Standby';
  document.getElementById('sSpd').textContent = s.currentSpeed + '%';

  // Header time
  if (s.time) document.getElementById('hdrTime').textContent = s.time;

  // WiFi badge
  const b = document.getElementById('wifiBadge');
  b.textContent = s.apMode ? 'AP' : 'STA';
  b.className = 'badge ' + (s.apMode ? 'ap' : 'sta');

  // Uptime
  if (s.uptime !== undefined) {
    const h = Math.floor(s.uptime/3600), m = Math.floor(s.uptime%3600/60);
    document.getElementById('uptimeTxt').textContent = h+'h '+String(m).padStart(2,'0')+'m';
  }
  document.getElementById('ntpTxt').textContent = s.time ? 'Synced' : 'Waiting';

  // Sleep timer
  if (s.sleepRemaining > 0) showSleepCd(s.sleepRemaining);
  else if (!sleepTick) hideSleepCd();

  // AutoCycle status card
  document.getElementById('acStatus').style.display = s.autoCycleActive ? 'block' : 'none';

  // Populate settings fields on first load (only if empty)
  const defSpdEl = document.getElementById('setDefSpd');
  if (defSpdEl && defSpdEl.dataset.loaded !== '1' && s.defaultSpeed !== undefined) {
    defSpdEl.value = s.defaultSpeed;
    defSpdEl.dataset.loaded = '1';
  }
  const tzEl = document.getElementById('setTz');
  if (tzEl && tzEl.dataset.loaded !== '1' && s.ntpTz) {
    tzEl.value = s.ntpTz;
    tzEl.dataset.loaded = '1';
  }
  const lt = document.getElementById('ledToggle');
  if (s.ledEnabled !== undefined && lt.dataset.loaded !== '1') {
    lt.className = 'toggle ' + (s.ledEnabled ? 'on' : '');
    lt.dataset.state = s.ledEnabled ? '1' : '0';
    lt.dataset.loaded = '1';
}
}

// ── Gauge ────────────────────────────────────────────────────────────────────
function updateGauge(pct) {
  const arc = document.getElementById('gArc');
  const total = 198;
  const fill = (pct / 100) * total;
  arc.setAttribute('stroke-dasharray', fill + ' ' + (total - fill));
  arc.setAttribute('stroke', pct > 75 ? '#ef4444' : pct > 50 ? '#f59e0b' : '#00d4ff');
}

// ── Power ─────────────────────────────────────────────────────────────────────
async function togglePower() {
  try {
    const on = !st.fanOn;
    await api('/api/fan','POST',{on});
    toast(on ? '🌀 Starting…' : '⏹ Stopping…');
    setTimeout(poll, 400);
  } catch(e) { toast('Error'); }
}

// ── Slider ───────────────────────────────────────────────────────────────────
// oninput fires continuously while dragging — update display only, no API call
function sliderInput(v) {
  sliderDragging = true;
  document.getElementById('speedBig').textContent = v;
  document.getElementById('speedSlider').style.setProperty('--val', v + '%');
  updateGauge(parseInt(v));
}

// onchange fires once on release — commit to API
let sDebounce = null;
function sliderCommit(v) {
  clearTimeout(sDebounce);
  sDebounce = setTimeout(async () => {
    try {
      await api('/api/fan', 'POST', {speed: parseInt(v)});
      toast('Speed → ' + v + '%');
      // Small delay then re-enable poll updates and refresh
      setTimeout(() => {
        sliderDragging = false;
        poll();
      }, 500);
    } catch(e) {
      sliderDragging = false;
    }
  }, 120);
}

// Also release dragging lock if user lifts finger/mouse without triggering onchange
document.addEventListener('pointerup', () => {
  // Give sliderCommit a chance to fire first
  setTimeout(() => { sliderDragging = false; }, 600);
});

function setPreset(v) {
  sliderDragging = true;
  const sl = document.getElementById('speedSlider');
  sl.value = v; sliderInput(v); sliderCommit(v);
}

// ── Sleep timer ───────────────────────────────────────────────────────────────
async function setSleep(min) {
  try {
    await api('/api/sleep','POST',{minutes:min});
    toast('Sleep timer: '+min+'m');
    showSleepCd(min*60);
    document.querySelectorAll('.sleep-btn').forEach(b=>b.classList.remove('active'));
    document.querySelector(`.sleep-btn[data-min="${min}"]`).classList.add('active');
  } catch(e){toast('Error');}
}

async function cancelSleep() {
  await api('/api/sleep','POST',{minutes:0});
  hideSleepCd(); toast('Timer cancelled');
}

function showSleepCd(secs) {
  const el = document.getElementById('sleepCd');
  const btn = document.getElementById('cancelSleepBtn');
  el.style.display='block'; btn.style.display='block';
  if (sleepTick) clearInterval(sleepTick);
  let rem = secs;
  const render = () => {
    const m=Math.floor(rem/60), s=rem%60;
    el.textContent = String(m).padStart(2,'0')+':'+String(s).padStart(2,'0');
    if(--rem<0){clearInterval(sleepTick);sleepTick=null;hideSleepCd();}
  };
  render();
  sleepTick = setInterval(render,1000);
}

function hideSleepCd() {
  document.getElementById('sleepCd').style.display='none';
  document.getElementById('cancelSleepBtn').style.display='none';
  document.querySelectorAll('.sleep-btn').forEach(b=>b.classList.remove('active'));
  if(sleepTick){clearInterval(sleepTick);sleepTick=null;}
}

// ── Schedules ─────────────────────────────────────────────────────────────────
async function loadSchedules() {
  const list = await api('/api/schedules');
  const el = document.getElementById('schedList');
  const active = list.filter(s=>s.active);
  if(!active.length){
    el.innerHTML='<div style="color:var(--muted);font-size:.83rem;text-align:center;padding:14px">No schedules yet</div>';
    return;
  }
  const actions=['OFF','ON','SPD']; const tags=['tag-off','tag-on','tag-spd'];
  el.innerHTML = active.map(s=>`
    <div class="sched-item">
      <div class="sched-time">${String(s.hour).padStart(2,'0')}:${String(s.minute).padStart(2,'0')}</div>
      <div class="sched-info">
        <div class="sched-name">${s.label||'Schedule'}</div>
        <span class="sched-tag ${tags[s.action]}">${actions[s.action]}${s.action===2?' '+s.speed+'%':''}</span>
      </div>
      <button class="del" onclick="delSchedule(${s.id})">✕</button>
    </div>`).join('');
}

function toggleSchedSpd() {
  document.getElementById('sSpdWrap').style.display =
    document.getElementById('sAction').value==='2' ? 'block' : 'none';
}

async function addSchedule() {
  const action = parseInt(document.getElementById('sAction').value);
  try {
    await api('/api/schedules','POST',{
      hour:   parseInt(document.getElementById('sHour').value),
      minute: parseInt(document.getElementById('sMin').value),
      action, speed: parseInt(document.getElementById('sSpeed').value),
      label:  document.getElementById('sLabel').value, active:true
    });
    toast('Schedule added ✓'); loadSchedules();
  } catch(e){toast('Error');}
}

async function delSchedule(id) {
  await fetch(BASE+'/api/schedules?id='+id,{method:'DELETE'});
  toast('Deleted'); loadSchedules();
}

// ── AutoCycle ─────────────────────────────────────────────────────────────────
async function loadAC() {
  const ac = await api('/api/autocycle');
  acOn = ac.enabled;
  updateACUI();
  const p = (n,v)=>String(n).padStart(2,'0')+':'+String(v).padStart(2,'0');
  document.getElementById('acStart').value = p(ac.startHour,ac.startMinute);
  document.getElementById('acEnd').value   = p(ac.endHour,ac.endMinute);
  document.getElementById('acLow').value   = ac.lowSpeed;
  document.getElementById('acHigh').value  = ac.highSpeed;
  document.getElementById('acSlowDur').value = ac.slowDuration;
  document.getElementById('acHighDur').value = ac.highDuration;
}

function toggleAC() { acOn=!acOn; updateACUI(); }
function updateACUI() {
  document.getElementById('acToggle').className='toggle '+(acOn?'on':'');
  document.getElementById('acP').className='ac-p '+(acOn?'show':'');
}

async function saveAC() {
  const [sh,sm]=document.getElementById('acStart').value.split(':').map(Number);
  const [eh,em]=document.getElementById('acEnd').value.split(':').map(Number);
  try {
    await api('/api/autocycle','POST',{
      enabled:acOn, startHour:sh, startMinute:sm, endHour:eh, endMinute:em,
      lowSpeed:parseInt(document.getElementById('acLow').value),
      highSpeed:parseInt(document.getElementById('acHigh').value),
      slowDuration:parseInt(document.getElementById('acSlowDur').value),
      highDuration:parseInt(document.getElementById('acHighDur').value)
    });
    toast('AutoCycle saved ✓');
  } catch(e){toast('Error');}
}

// ── LED toggle ────────────────────────────────────────────────────────────────
function toggleLED() {
  const lt = document.getElementById('ledToggle');
  const newState = lt.dataset.state !== '1';
  lt.dataset.state = newState ? '1' : '0';
  lt.className = 'toggle ' + (newState ? 'on' : '');
}

// ── WiFi settings ─────────────────────────────────────────────────────────────
async function loadWifi() {
  const w = await api('/api/wifi/status');
  document.getElementById('ipBox').textContent = w.ip;
  document.getElementById('wifiInfoTxt').textContent =
    (w.apMode?'📡 AP: ':'🔗 ') + w.ssid;
  document.getElementById('wifiBadge').textContent = w.apMode?'AP':'STA';
  document.getElementById('wifiBadge').className='badge '+(w.apMode?'ap':'sta');
  const pct = w.rssi ? Math.max(0,Math.min(100,(w.rssi+90)/60*100)) : 0;
  document.getElementById('rssiFill').style.width=pct+'%';
}

async function saveWifi() {
  const ssid=document.getElementById('wSSID').value.trim();
  const pass=document.getElementById('wPass').value;
  if(!ssid){toast('Enter SSID');return;}
  try{await api('/api/wifi','POST',{ssid,pass});toast('Restarting…');}
  catch(e){toast('Error');}
}

// ── Save Settings (defaultSpeed + LED + NTP TZ) ───────────────────────────────
async function saveSettings() {
  const defSpd = parseInt(document.getElementById('setDefSpd').value);
  const tz     = document.getElementById('setTz').value.trim();
  const lt     = document.getElementById('ledToggle');
  const ledEn  = lt.dataset.state === '1';

  if (!defSpd || defSpd < 1 || defSpd > 100) { toast('Default speed must be 1–100'); return; }
  if (!tz) { toast('Enter a timezone string'); return; }

  try {
    await api('/api/settings', 'POST', {
      defaultSpeed: defSpd,
      ledEnabled: ledEn,
      ntpTz: tz
    });
    toast('Settings saved ✓');
    // Reset loaded flags so next poll refreshes displayed values
    document.getElementById('setDefSpd').dataset.loaded = '';
    document.getElementById('setTz').dataset.loaded = '';
    setTimeout(poll, 300);
  } catch(e) { toast('Error saving settings'); }
}

// ── Tabs ──────────────────────────────────────────────────────────────────────
function switchTab(name) {
  document.querySelectorAll('.tab').forEach(t=>{
    t.className='tab'+(t.dataset.tab===name?' active':'');
  });
  document.querySelectorAll('.panel').forEach(p=>{
    p.className='panel'+(p.id==='tab-'+name?' active':'');
  });
  if(name==='schedule')  loadSchedules();
  if(name==='autocycle') loadAC();
  if(name==='settings')  loadWifi();
}

// ── Collapsible ───────────────────────────────────────────────────────────────
function toggleColl(hdr) {
  hdr.classList.toggle('open');
  hdr.nextElementSibling.classList.toggle('open');
}

// ── Init ──────────────────────────────────────────────────────────────────────
poll();
setInterval(poll, 2000);
</script>
</body>
</html>
)HTMLEOF";