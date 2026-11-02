"use strict";

/*
 * The driving page of Mini Omni Bot. Served by the robot's ESP32, which also
 * runs the WebSocket at /ws.
 *
 * Page -> robot
 *   {type:"drive", x, y, r, spin, limit}
 *       Everything the robot needs to drive, every time: x right and y forward on
 *       the unit disc, r = held turn (+1 left/anticlockwise .. -1 right),
 *       spin = latched turn (1 left, 0 off, -1 right), limit = speed limit in %.
 *       Sent on every change, every 50 ms while the stick or a key is held, and
 *       every 100 ms otherwise while Joystick mode is on, the page is visible and
 *       the robot's telemetry is fresh. A lost message is replaced by the next;
 *       the robot stops and drops the spin when they stop for 0.5 s.
 *   {type:"enable_joy"} / {type:"disable_joy"}, {type:"estop"}, {type:"ping"}
 * Robot -> page
 *   {type:"telemetry", joy_enabled, spin, angle, speed, turn, limit, net, rssi}
 *       ten per second; net is "sta" (on a WiFi network, rssi in dBm) or "ap"
 *       (the robot's own hotspot, rssi null)
 *   {type:"pong"}, {type:"busy"} (one controller at a time; others are closed)
 */

// ---------- Settings ----------

const DRIVE_HELD_MS = 50; // drive state while the stick or a key is held
const DRIVE_IDLE_MS = 100; // and otherwise; the robot stops after 500 ms without one
const BACKLOG_BYTES = 1024; // a stalled link: skip periodic sends rather than queue stale ones
const PING_INTERVAL_MS = 2000;
const TELEMETRY_STALE_MS = 1500;
const RECONNECT_MS = 1000;
const KNOB_TRAVEL = 0.7; // knob centre travel as a fraction of the pad radius (visual only)
const RING_CIRCUMFERENCE = 2 * Math.PI * 95;
const LIMIT_STORAGE_KEY = "omnibotsmini.speedLimit";
const DEFAULT_LIMIT = 50;

const SPIN_TEXT = { 1: "Turning left", 0: "Off", "-1": "Turning right" };

const KEY_DIRS = {
  KeyW: [0, 1],
  ArrowUp: [0, 1],
  KeyS: [0, -1],
  ArrowDown: [0, -1],
  KeyA: [-1, 0],
  ArrowLeft: [-1, 0],
  KeyD: [1, 0],
  ArrowRight: [1, 0],
};
const KEY_TURN = { KeyQ: 1, KeyE: -1 };
// macOS swallows the keyup of a letter released while Cmd is down, so any
// modifier activity drops keyboard driving rather than risk a stuck key.
const MODIFIER_KEYS = new Set(["Meta", "Control", "Alt", "OS"]);

// ---------- Elements ----------

const $ = (id) => document.getElementById(id);
const appEl = $("app");
const statusTextEl = $("statusText");
const serverChipEl = $("serverChip");
const latencyEl = $("latency");
const wifiChipEl = $("wifiChip");
const rssiEl = $("rssi");
const noticeEl = $("notice");
const angleValueEl = $("angleValue");
const dirArrowEl = $("dirArrow");
const speedValueEl = $("speedValue");
const joystickEl = $("joystick");
const stickEl = $("stick");
const speedRingEl = $("speedRing");
const vecLineEl = $("vecLine");
const dirMarkerEl = $("dirMarker");
const padLockTextEl = $("padLockText");
const spinSegEl = $("spinSeg");
const spinCaptionEl = $("spinCaption");
const limitSliderEl = $("limitSlider");
const limitValueEl = $("limitValue");
const driveBtn = $("driveBtn");
const driveSubEl = $("driveSub");
const stopBtn = $("stopBtn");
const toastEl = $("toast");

// ---------- State ----------

const state = {
  wsOpen: false,
  openedMs: 0,
  everConnected: false,
  busy: false,
  lastTelemetryMs: -Infinity,
  joy: false,
  angle: 0,
  speed: 0,
  spin: 0,
  turn: 0,
  net: "",
  rssi: null,
  rttMs: null,
  limit: DEFAULT_LIMIT,
};

// After a local change, ignore telemetry for that field briefly so a frame that
// was already in flight does not flick the control back.
const localHold = { joy: 0, spin: 0 };

let ws = null;
let reconnectTimer = null;
let pingTimer = null;
let pingSentAt = 0;
let activePointerId = null;
let lastDriveMs = -Infinity;
const input = { x: 0, y: 0, r: 0 }; // stick or keyboard vector on the unit disc, and held turn
const keysDown = new Set();

// ---------- Small helpers ----------

function setText(el, text) {
  if (el.textContent !== text) el.textContent = text;
}

function setAttr(el, name, value) {
  const v = String(value);
  if (el.getAttribute(name) !== v) el.setAttribute(name, v);
}

function vibrate(pattern) {
  try {
    if (navigator.vibrate) navigator.vibrate(pattern);
  } catch (_err) {
    // Not supported (iOS Safari); vibration is optional.
  }
}

let toastTimer = null;
function showToast(text, tone = "") {
  toastEl.textContent = text;
  toastEl.dataset.tone = tone;
  toastEl.classList.add("show");
  clearTimeout(toastTimer);
  toastTimer = setTimeout(() => toastEl.classList.remove("show"), 1700);
}

function restartAnimation(el, cls) {
  el.classList.remove(cls);
  void el.offsetWidth;
  el.classList.add(cls);
}

// Fire on pointerup as well as click. A second finger lifting while the first
// is still on the joystick does not produce a click on phones, so click alone
// would make these controls dead while driving. The click that follows a
// handled pointerup is dropped.
function onTap(el, handler) {
  let downId = null;
  let lastFired = -Infinity;

  el.addEventListener("pointerdown", (event) => {
    downId = event.pointerId;
  });
  // A mouse keeps pointerId 1 forever: forget a press that was dragged off, or
  // a later release over this control would fire it.
  el.addEventListener("pointerleave", () => {
    downId = null;
  });
  el.addEventListener("pointercancel", () => {
    downId = null;
  });
  el.addEventListener("pointerup", (event) => {
    if (event.pointerId !== downId) return;
    downId = null;
    const r = el.getBoundingClientRect();
    const inside =
      event.clientX >= r.left && event.clientX <= r.right && event.clientY >= r.top && event.clientY <= r.bottom;
    if (!inside || el.disabled) return;
    lastFired = performance.now();
    handler(event);
  });
  el.addEventListener("click", (event) => {
    if (performance.now() - lastFired < 600) return;
    if (el.disabled) return;
    handler(event);
  });
}

// ---------- Sending ----------

function sendWsMessage(payload) {
  if (state.busy) return false;
  if (!ws || ws.readyState !== WebSocket.OPEN) return false;
  ws.send(JSON.stringify(payload));
  return true;
}

// The whole driving state in one message, so a lost one is simply replaced by the
// next. The robot applies the deadzone and the speed limit, to driving and turning alike.
function sendDrive() {
  const sent = sendWsMessage({ type: "drive", x: input.x, y: input.y, r: input.r, spin: state.spin, limit: state.limit });
  if (sent) lastDriveMs = performance.now();
  return sent;
}

function inputActive() {
  return activePointerId !== null || keysDown.size > 0;
}

// The page keeps the robot going only while someone can see it and it can see
// the robot. Otherwise it goes quiet, and the robot stops on its own after 0.5 s.
function canDrive() {
  return state.joy && state.wsOpen && !state.busy && telemetryFresh() && !document.hidden;
}

function driveTick() {
  if (!canDrive()) return;
  if (!inputActive() && performance.now() - lastDriveMs < DRIVE_IDLE_MS - 10) return;
  if (ws.bufferedAmount > BACKLOG_BYTES) return;
  sendDrive();
}

function releaseJoystick() {
  input.x = 0;
  input.y = 0;
  input.r = 0;
  moveStickVisual(0, 0);
  sendDrive();
}

// Drop every source of motion input. `force` sends the zero even if nothing
// was held (E-stop, page unload); otherwise only when there was input to undo.
function releaseAllInput(force) {
  const hadInput = inputActive() || input.x !== 0 || input.y !== 0 || input.r !== 0;
  const pointerId = activePointerId;
  activePointerId = null;
  if (pointerId !== null && joystickEl.hasPointerCapture && joystickEl.hasPointerCapture(pointerId)) {
    try {
      joystickEl.releasePointerCapture(pointerId);
    } catch (_err) {
      // Already released.
    }
  }
  keysDown.clear();
  joystickEl.classList.remove("dragging", "keying");
  if (force || hadInput) {
    releaseJoystick();
  } else {
    moveStickVisual(0, 0);
  }
}

// Spin is latched on the robot: a zero vector leaves it turning. Used whenever
// nobody can be watching the page any more.
function stopSpin() {
  if (state.spin === 0) return;
  state.spin = 0;
  localHold.spin = performance.now() + 400;
  if (state.joy) sendDrive();
  render();
}

// ---------- Joystick visuals ----------

function moveStickVisual(x, y) {
  const radiusPx = joystickEl.clientWidth / 2;
  const px = x * radiusPx * KNOB_TRAVEL;
  const py = -y * radiusPx * KNOB_TRAVEL;
  stickEl.style.transform = `translate(calc(-50% + ${px}px), calc(-50% + ${py}px))`;
  vecLineEl.setAttribute("x2", (x * 100 * KNOB_TRAVEL).toFixed(2));
  vecLineEl.setAttribute("y2", (-y * 100 * KNOB_TRAVEL).toFixed(2));
}

function buildTicks() {
  const ns = "http://www.w3.org/2000/svg";
  const group = $("ticks");
  for (let deg = 0; deg < 360; deg += 15) {
    if (deg % 90 === 0) continue; // the chevrons mark the four main directions
    const mid = deg % 45 === 0;
    const a = (deg * Math.PI) / 180;
    const r1 = mid ? 80 : 83;
    const r2 = 88;
    const line = document.createElementNS(ns, "line");
    line.setAttribute("x1", (Math.sin(a) * r1).toFixed(2));
    line.setAttribute("y1", (-Math.cos(a) * r1).toFixed(2));
    line.setAttribute("x2", (Math.sin(a) * r2).toFixed(2));
    line.setAttribute("y2", (-Math.cos(a) * r2).toFixed(2));
    if (mid) line.setAttribute("class", "mid");
    group.appendChild(line);
  }
  speedRingEl.style.strokeDasharray = `${RING_CIRCUMFERENCE}`;
  speedRingEl.style.strokeDashoffset = `${RING_CIRCUMFERENCE}`;
}

// ---------- Rendering ----------

function telemetryFresh() {
  return performance.now() - state.lastTelemetryMs < TELEMETRY_STALE_MS;
}

function padEnabled() {
  return state.wsOpen && !state.busy && state.joy && telemetryFresh();
}

function overallState() {
  if (state.busy) return ["busy", "Another device is in control"];
  if (!state.wsOpen) return [state.everConnected ? "offline" : "connecting", state.everConnected ? "Reconnecting…" : "Connecting…"];
  if (!telemetryFresh()) return ["waiting", "Waiting for the robot…"];
  if (!state.joy) return ["off", "Joystick mode is off"];
  if (state.speed > 0) return ["driving", "Driving"];
  if (state.turn !== 0) return ["driving", state.spin !== 0 ? "Spinning" : "Turning"];
  return ["ready", "Ready to drive"];
}

function renderSegmented(segEl, index, disabled) {
  const buttons = segEl.querySelectorAll("button");
  segEl.style.setProperty("--n", String(buttons.length));
  segEl.style.setProperty("--i", String(index));
  setAttr(segEl, "aria-disabled", disabled);
  buttons.forEach((btn, i) => {
    setAttr(btn, "aria-checked", i === index);
    btn.tabIndex = i === index ? 0 : -1;
    btn.disabled = disabled;
  });
}

function wifiLevel(rssi) {
  if (rssi >= -67) return "ok";
  if (rssi >= -80) return "warn";
  return "bad";
}

function render() {
  const fresh = telemetryFresh();
  const live = state.wsOpen && fresh && !state.busy;
  const [key, text] = overallState();
  appEl.dataset.state = key;
  setText(statusTextEl, text);

  // Chips
  let serverLevel = "warn";
  if (state.busy) serverLevel = "bad";
  else if (state.wsOpen && fresh) serverLevel = "ok";
  else if (!state.wsOpen && state.everConnected) serverLevel = "bad";
  serverChipEl.dataset.level = serverLevel;
  setText(latencyEl, state.wsOpen && state.rttMs !== null ? `${Math.round(state.rttMs)} ms` : "");

  setAttr(serverChipEl, "aria-label", `Robot connection${latencyEl.textContent ? `, ${latencyEl.textContent}` : ""}`);

  // WiFi: signal strength on a network; on its own hotspot the robot has no reading.
  let wifiText = "";
  let wifiTitle = "The robot's WiFi signal";
  let wifiLevelNow = "warn";
  if (live && state.net === "ap") {
    wifiText = "Hotspot";
    wifiTitle = "Connected to the robot's own hotspot";
    wifiLevelNow = "ok";
  } else if (live && Number.isFinite(state.rssi)) {
    wifiText = `${state.rssi} dBm`;
    wifiTitle = `The robot's WiFi signal: ${state.rssi} dBm`;
    wifiLevelNow = wifiLevel(state.rssi);
  }
  wifiChipEl.dataset.level = wifiLevelNow;
  setText(rssiEl, wifiText);
  wifiChipEl.title = wifiTitle;
  setAttr(wifiChipEl, "aria-label", wifiText ? `WiFi, ${wifiText}` : "WiFi");

  // Busy notice
  if (state.busy) {
    noticeEl.hidden = false;
    noticeEl.innerHTML = "Another device is driving the robot.<small>You'll get control when it disconnects.</small>";
  } else {
    noticeEl.hidden = true;
  }

  // Readouts: what the robot is actually doing. Blank them when that is
  // unknown rather than leave the last frame on screen.
  const speed = live ? Math.max(0, Math.min(100, Number(state.speed) || 0)) : 0;
  const moving = speed > 0;
  setText(speedValueEl, live ? String(speed) : "–");
  setText(angleValueEl, moving ? `${state.angle}°` : "—");
  dirArrowEl.classList.toggle("idle", !moving);
  // The robot counts angles anticlockwise (90 = left); CSS rotates clockwise.
  dirArrowEl.style.transform = moving ? `rotate(${-state.angle}deg)` : "";
  speedRingEl.style.strokeDashoffset = `${RING_CIRCUMFERENCE * (1 - speed / 100)}`;
  dirMarkerEl.classList.toggle("show", moving);
  if (moving) dirMarkerEl.style.transform = `rotate(${-state.angle}deg)`;

  // Pad
  const enabled = padEnabled();
  setAttr(joystickEl, "aria-disabled", !enabled);
  if (state.busy) setText(padLockTextEl, "Another device is driving");
  else if (!state.wsOpen) setText(padLockTextEl, "Connecting…");
  else if (!fresh) setText(padLockTextEl, "Waiting for the robot…");
  else if (!state.joy) setText(padLockTextEl, "Turn on Joystick mode");

  // Spin: the buttons run Left (1), Off (0), Right (-1)
  renderSegmented(spinSegEl, 1 - state.spin, !enabled);
  setAttr(spinSegEl, "data-active", state.spin !== 0);
  setText(spinCaptionEl, SPIN_TEXT[state.spin] || "Off");

  // Actions
  setAttr(driveBtn, "aria-checked", state.joy);
  driveBtn.disabled = !state.wsOpen || state.busy;
  setText(driveSubEl, state.joy ? "On" : "Off");
  // A device that is not in control cannot stop the robot, so the button is disabled.
  stopBtn.disabled = state.busy;
}

function renderLimit() {
  const min = Number(limitSliderEl.min);
  const max = Number(limitSliderEl.max);
  const pct = ((state.limit - min) / (max - min)) * 100;
  limitSliderEl.value = String(state.limit);
  limitSliderEl.style.setProperty("--pct", `${pct}%`);
  setText(limitValueEl, `${state.limit}%`);
}

// Runs every 500 ms: keeps the staleness indicators correct between frames and
// lets go of the stick if the pad stopped being usable mid-drive.
function tick() {
  if (inputActive() && !padEnabled()) releaseAllInput(false);
  // An open socket that has received nothing for 5 s (for example, the robot missed this page
  // connecting) starts over.
  const heardMs = Math.max(state.lastTelemetryMs, state.openedMs);
  if (state.wsOpen && !state.busy && performance.now() - heardMs > 5000) ws.close();
  render();
}

// ---------- WebSocket ----------

function scheduleReconnect() {
  if (reconnectTimer) clearTimeout(reconnectTimer);
  reconnectTimer = setTimeout(connectWebSocket, RECONNECT_MS);
}

function startPing() {
  if (pingTimer) clearInterval(pingTimer);
  // A hidden page goes completely quiet, so it lets go of the robot after 6 s.
  const ping = () => {
    if (document.hidden) return;
    if (sendWsMessage({ type: "ping" })) pingSentAt = performance.now();
  };
  ping();
  pingTimer = setInterval(ping, PING_INTERVAL_MS);
}

function onTelemetry(msg) {
  const now = performance.now();
  state.lastTelemetryMs = now;
  // Telemetry only reaches the client that holds control.
  state.busy = false;
  state.angle = Number(msg.angle) || 0;
  state.speed = Number(msg.speed) || 0;
  state.turn = Number(msg.turn) || 0;
  state.net = msg.net === "ap" || msg.net === "sta" ? msg.net : "";
  state.rssi = Number.isFinite(msg.rssi) ? msg.rssi : null;

  if (now > localHold.joy) {
    const joy = !!msg.joy_enabled;
    if (joy !== state.joy && !joy) releaseAllInput(false);
    state.joy = joy;
  }
  // The robot clears a latched spin by itself (silent page, stop typed in the Serial
  // Monitor), so the page follows what it reports.
  if (now > localHold.spin) {
    const spin = Number(msg.spin);
    state.spin = spin === -1 || spin === 1 ? spin : 0;
  }
  render();
}

function connectWebSocket() {
  const proto = window.location.protocol === "https:" ? "wss" : "ws";
  const url = `${proto}://${window.location.host}/ws`;

  ws = new WebSocket(url);

  ws.addEventListener("open", () => {
    state.wsOpen = true;
    state.openedMs = performance.now();
    state.everConnected = true;
    // A new socket starts afresh; the robot says "busy" again if it still is.
    state.busy = false;
    startPing();
    render();
  });

  ws.addEventListener("close", () => {
    state.wsOpen = false;
    state.rttMs = null;
    if (pingTimer) clearInterval(pingTimer);
    pingTimer = null;
    releaseAllInput(false);
    render();
    scheduleReconnect();
  });

  ws.addEventListener("message", (event) => {
    let msg;
    try {
      msg = JSON.parse(event.data);
    } catch (_err) {
      return; // ignore malformed payloads
    }
    if (msg.type === "telemetry") {
      onTelemetry(msg);
    } else if (msg.type === "busy") {
      state.busy = true;
      releaseAllInput(false);
      render();
    } else if (msg.type === "pong") {
      if (pingSentAt) {
        state.rttMs = performance.now() - pingSentAt;
        pingSentAt = 0;
      }
    }
  });
}

setInterval(driveTick, DRIVE_HELD_MS);

// ---------- Joystick input (pointer) ----------

function updateFromPointer(clientX, clientY) {
  const rect = joystickEl.getBoundingClientRect();
  const radius = rect.width / 2;
  if (!(radius > 0)) return; // pad not laid out (hidden page): no meaningful vector
  let nx = (clientX - (rect.left + radius)) / radius;
  let ny = (rect.top + radius - clientY) / radius;
  if (!Number.isFinite(nx) || !Number.isFinite(ny)) return;
  const mag = Math.hypot(nx, ny);
  if (mag > 1) {
    nx /= mag;
    ny /= mag;
  }
  input.x = nx;
  input.y = ny;
  moveStickVisual(nx, ny);
  // driveTick sends it within 50 ms; one message per pointer move would flood the robot
}

function nudgeDrive() {
  restartAnimation(driveBtn, "nudge");
  showToast("Turn on Joystick mode to drive");
  vibrate(20);
}

joystickEl.addEventListener("pointerdown", (event) => {
  event.preventDefault();
  if (!padEnabled()) {
    if (state.wsOpen && !state.busy && !state.joy) nudgeDrive();
    return;
  }
  if (activePointerId !== null) return; // one finger drives; ignore a second one
  keysDown.clear();
  input.r = 0;
  joystickEl.classList.remove("keying");
  activePointerId = event.pointerId;
  try {
    joystickEl.setPointerCapture(event.pointerId);
  } catch (_err) {
    // Pointer already gone; the pointerup/cancel handlers still end the drag.
  }
  joystickEl.classList.add("dragging");
  updateFromPointer(event.clientX, event.clientY);
  sendDrive();
});

joystickEl.addEventListener("pointermove", (event) => {
  if (event.pointerId !== activePointerId) return;
  event.preventDefault();
  updateFromPointer(event.clientX, event.clientY);
});

function onPointerEnd(event) {
  if (event.pointerId !== activePointerId) return;
  activePointerId = null;
  joystickEl.classList.remove("dragging");
  releaseJoystick();
}

joystickEl.addEventListener("pointerup", onPointerEnd);
joystickEl.addEventListener("pointercancel", onPointerEnd);
joystickEl.addEventListener("lostpointercapture", onPointerEnd);
joystickEl.addEventListener("contextmenu", (event) => event.preventDefault());

// ---------- Joystick input (keyboard) ----------

function applyKeys() {
  let x = 0;
  let y = 0;
  let r = 0;
  keysDown.forEach((code) => {
    if (KEY_DIRS[code]) {
      x += KEY_DIRS[code][0];
      y += KEY_DIRS[code][1];
    } else {
      r += KEY_TURN[code];
    }
  });
  const mag = Math.hypot(x, y);
  input.x = mag > 0 ? x / mag : 0;
  input.y = mag > 0 ? y / mag : 0;
  input.r = Math.max(-1, Math.min(1, r));
  joystickEl.classList.toggle("keying", keysDown.size > 0);
  moveStickVisual(input.x, input.y);
  if (keysDown.size > 0) sendDrive();
  else releaseJoystick();
}

function dropKeyboardDrive() {
  if (keysDown.size === 0) return;
  keysDown.clear();
  applyKeys();
}

window.addEventListener("keydown", (event) => {
  // Stop first, whatever else is held.
  if (event.code === "Space" || event.code === "Escape") {
    event.preventDefault();
    if (!event.repeat) emergencyStop();
    return;
  }

  if (event.metaKey || event.ctrlKey || event.altKey || MODIFIER_KEYS.has(event.key)) {
    dropKeyboardDrive();
    return;
  }

  if (!KEY_DIRS[event.code] && !KEY_TURN[event.code]) return;
  // Driving takes priority over a focused slider: arrow keys must never raise the
  // limit without the operator noticing.
  if (document.activeElement === limitSliderEl) limitSliderEl.blur();
  event.preventDefault();
  if (event.repeat || keysDown.has(event.code)) return;
  if (!padEnabled()) {
    if (state.wsOpen && !state.busy && !state.joy) nudgeDrive();
    return;
  }
  if (activePointerId !== null) return; // the pointer has priority
  keysDown.add(event.code);
  applyKeys();
});

window.addEventListener("keyup", (event) => {
  // Browsers click a focused button on Space *keyup*; Space is Stop here.
  if (event.code === "Space") event.preventDefault();
  if (MODIFIER_KEYS.has(event.key)) {
    dropKeyboardDrive();
    return;
  }
  if (!keysDown.has(event.code)) return;
  keysDown.delete(event.code);
  applyKeys();
});

// ---------- Controls ----------

function emergencyStop() {
  state.spin = 0;
  localHold.spin = performance.now() + 400;
  releaseAllInput(true); // sends the zeroed drive state
  // ...and the estop brakes at once instead of ramping down, dropping any spin.
  const sent = sendWsMessage({ type: "estop" });
  restartAnimation(stopBtn, "flash");
  vibrate([30, 40, 30]);
  if (sent && telemetryFresh()) showToast("Stopped", "danger");
  else if (sent) showToast("Stop sent · no reply from the robot", "danger");
  else if (state.busy) showToast("Another device is in control", "danger");
  else showToast("Not connected · robot stops on its own when the link drops", "danger");
  render();
}

let lastStopMs = -Infinity;
stopBtn.addEventListener("pointerdown", (event) => {
  if (event.button !== undefined && event.button > 0) return;
  if (stopBtn.disabled) return;
  lastStopMs = performance.now();
  emergencyStop();
});
stopBtn.addEventListener("click", () => {
  if (performance.now() - lastStopMs < 800) return; // already handled on pointerdown
  if (stopBtn.disabled) return;
  emergencyStop();
});

onTap(driveBtn, () => {
  if (!state.wsOpen || state.busy) return;
  const next = !state.joy;
  if (!next) releaseAllInput(true);
  if (sendWsMessage({ type: next ? "enable_joy" : "disable_joy" })) {
    state.joy = next;
    state.spin = 0;
    localHold.joy = performance.now() + 400;
    localHold.spin = localHold.joy;
    vibrate(12);
    render();
  }
});

spinSegEl.querySelectorAll("button").forEach((btn) => {
  onTap(btn, () => {
    if (!padEnabled()) return;
    state.spin = Number(btn.dataset.value);
    localHold.spin = performance.now() + 400;
    sendDrive();
    vibrate(8);
    render();
  });
});

function loadLimit() {
  let stored = null;
  try {
    stored = window.localStorage.getItem(LIMIT_STORAGE_KEY);
  } catch (_err) {
    // Storage blocked; keep the default.
  }
  const v = Number(stored);
  if (stored !== null && Number.isFinite(v) && v >= 10 && v <= 100) state.limit = Math.round(v / 5) * 5;
}

limitSliderEl.addEventListener("input", () => {
  state.limit = Number(limitSliderEl.value);
  renderLimit();
  try {
    window.localStorage.setItem(LIMIT_STORAGE_KEY, String(state.limit));
  } catch (_err) {
    // Storage blocked; the setting lasts for this page only.
  }
  if (state.joy) sendDrive();
});
// Hand the arrow keys back to driving after a mouse or touch drag.
limitSliderEl.addEventListener("pointerup", () => limitSliderEl.blur());

// ---------- Page lifecycle and safety ----------

// Nobody is watching once the page is hidden or loses focus: let go of the
// stick and stop any latched spin.
document.addEventListener("visibilitychange", () => {
  if (!document.hidden) return;
  releaseAllInput(false);
  stopSpin();
});
window.addEventListener("blur", () => {
  releaseAllInput(false);
  stopSpin();
});
window.addEventListener("pagehide", () => {
  releaseAllInput(false);
  stopSpin();
});
window.addEventListener("beforeunload", () => releaseAllInput(true));
// Rotating the phone moves the pad under a finger that is still down; the next
// move would be measured against the new layout, so end the drag instead.
window.addEventListener("resize", () => {
  if (activePointerId !== null) releaseAllInput(false);
  else moveStickVisual(input.x, input.y);
});

// iOS ignores user-scalable=no; stop pinch-zoom from hijacking two-finger driving.
["gesturestart", "gesturechange"].forEach((type) => {
  document.addEventListener(type, (event) => event.preventDefault());
});

// ---------- Start ----------

loadLimit();
renderLimit();
buildTicks();
render();
connectWebSocket();
setInterval(tick, 500);
