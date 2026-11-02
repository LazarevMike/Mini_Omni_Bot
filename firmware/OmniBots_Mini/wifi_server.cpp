// =====================================================================================
//  wifi_server.cpp - WiFi, the web server and the link to the driving page (ESP32 only)
//  Part of the phone joystick: built only when USE_PHONE_JOYSTICK is 1 (boards.h).
// =====================================================================================
//
//  WiFi joins the network saved with the `wifi <name> <password>` command. With none
//  saved, or when it cannot join within WIFI_JOIN_TIMEOUT_MS of starting, the robot opens
//  its own hotspot instead. The page is then at http://192.168.4.1, or on a home network
//  at http://<WEB_NAME>.local (both names are in config.h).
//
//  Implementation: the web server is ESP-IDF's own (esp_http_server) and runs in its own
//  task. Its callbacks do not drive anything. They place each event in a queue, with its
//  arrival time, and updateWifi() reads the queue in loop(). The driving code in
//  phone_control.cpp therefore runs in only one place. The same applies in the other
//  direction: everything sent to a page, and every disconnect, goes through the server's
//  own work queue, so only the server task accesses its connections.
//
//  This file normally needs no changes. Names and timings are in config.h.
//
// =====================================================================================

#include "config.h"

#if USE_PHONE_JOYSTICK

#include <Arduino.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include <WiFi.h>
#include <esp_http_server.h>
#include <unistd.h>

#include "bootloader_random.h"
#include "web_assets.h"
#include "phone_control.h"
#include "serial_commands.h"
#include "wifi_server.h"

namespace {

const uint32_t TELEMETRY_MS = 100;
const uint32_t WIFI_POLL_MS = 500;
const uint32_t BUSY_CLOSE_DELAY_MS = 300;  // lets "busy" arrive before the close
const size_t MAX_FRAME = 4096;             // anything bigger is not from the page

struct NetEvent {
  enum Kind : uint8_t { CONNECT, DISCONNECT, MESSAGE } kind;
  int fd;
  uint32_t receivedMs;
  char text[128];
};

struct Outgoing {
  int fd;
  size_t len;
  char text[200];
};

httpd_handle_t server = nullptr;
QueueHandle_t events;
volatile bool eventsLost = false;
Preferences prefs;  // namespace "net": ssid, pass, ap_pass

String savedSsid, savedPass, apPass;
bool hotspot = false;
bool wasConnected = false;
bool joinedSinceStart = false;
uint32_t joinStartMs = 0;

struct PendingClose {
  int fd;
  uint32_t atMs;
} pendingClose[4] = { { -1, 0 }, { -1, 0 }, { -1, 0 }, { -1, 0 } };

// A client id for the drive logic: the socket number + 1, so 0 can mean none.
uint32_t clientOf(int fd) {
  return (uint32_t)fd + 1;
}

int fdOf(uint32_t client) {
  return (int)client - 1;
}

// ------------------------------------------------- server task (callbacks) --

void postEvent(const NetEvent &event) {
  // If the queue is full, drop the event and set a flag. handleEvents() then stops the
  // robot until the page's next full drive message arrives.
  if (xQueueSend(events, &event, 0) != pdTRUE) eventsLost = true;
}

esp_err_t onSocket(httpd_req_t *req) {
  NetEvent event;
  event.fd = httpd_req_to_sockfd(req);
  event.receivedMs = millis();
  if (req->method == HTTP_GET) {  // the handshake: a page connected
    event.kind = NetEvent::CONNECT;
    event.text[0] = 0;
    postEvent(event);
    return ESP_OK;
  }

  httpd_ws_frame_t frame = {};
  esp_err_t err = httpd_ws_recv_frame(req, &frame, 0);  // reads the header: type and length
  if (err != ESP_OK) return err;
  if (frame.len > MAX_FRAME) return ESP_FAIL;  // closes the connection
  if (frame.type != HTTPD_WS_TYPE_TEXT || frame.len >= sizeof event.text) {
    // Not one of the page's messages: read it to keep the stream in step, then drop it.
    uint8_t *junk = (uint8_t *)malloc(frame.len + 1);
    if (!junk) return ESP_ERR_NO_MEM;
    frame.payload = junk;
    err = httpd_ws_recv_frame(req, &frame, frame.len);
    free(junk);
    return err;
  }
  frame.payload = (uint8_t *)event.text;
  err = httpd_ws_recv_frame(req, &frame, sizeof event.text - 1);
  if (err != ESP_OK) return err;
  event.text[frame.len] = 0;
  event.kind = NetEvent::MESSAGE;
  postEvent(event);
  return ESP_OK;
}

// Called for every connection that ends, page or not.
void onClose(httpd_handle_t, int fd) {
  NetEvent event;
  event.kind = NetEvent::DISCONNECT;
  event.fd = fd;
  event.receivedMs = millis();
  event.text[0] = 0;
  postEvent(event);
  close(fd);  // a custom close callback has to close the socket itself
}

esp_err_t onAsset(httpd_req_t *req) {
  const WebAsset *asset = (const WebAsset *)req->user_ctx;
  httpd_resp_set_type(req, asset->type);
  if (asset->gzip) httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
  httpd_resp_set_hdr(req, "Cache-Control", asset->cache ? "max-age=86400" : "no-cache");
  return httpd_resp_send(req, (const char *)asset->data, asset->size);
}

void sendWork(void *arg) {
  Outgoing *out = (Outgoing *)arg;
  // The socket may have closed, or been reused for a plain page request.
  if (httpd_ws_get_fd_info(server, out->fd) == HTTPD_WS_CLIENT_WEBSOCKET) {
    httpd_ws_frame_t frame = {};
    frame.final = true;
    frame.type = HTTPD_WS_TYPE_TEXT;
    frame.payload = (uint8_t *)out->text;
    frame.len = out->len;
    httpd_ws_send_frame_async(server, out->fd, &frame);
  }
  free(out);
}

// --------------------------------------------------------------- loop task --

void sendText(uint32_t client, const char *text) {
  size_t len = strlen(text);
  Outgoing *out = (Outgoing *)malloc(sizeof(Outgoing));
  if (!out || len > sizeof out->text) {
    free(out);
    return;
  }
  out->fd = fdOf(client);
  out->len = len;
  memcpy(out->text, text, len);
  if (httpd_queue_work(server, sendWork, out) != ESP_OK) free(out);
}

void closeClient(uint32_t client) {
  httpd_sess_trigger_close(server, fdOf(client));
}

void closeLater(uint32_t client, uint32_t now) {
  for (auto &p : pendingClose)
    if (p.fd < 0) {
      p = { fdOf(client), now + BUSY_CLOSE_DELAY_MS };
      return;
    }
  closeClient(client);  // no room to wait: close right away
}

void startServer() {
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.max_uri_handlers = WEB_ASSET_COUNT + 1;
  config.max_open_sockets = 8;
  config.lru_purge_enable = true;  // when full, drop the oldest connection instead of refusing
  config.close_fn = onClose;
  config.stack_size = 6144;
  if (httpd_start(&server, &config) != ESP_OK) {
    server = nullptr;
    Serial.println("   web: the server did not start");
    return;
  }
  httpd_uri_t socket = {};
  socket.uri = "/ws";
  socket.method = HTTP_GET;
  socket.handler = onSocket;
  socket.is_websocket = true;
  httpd_register_uri_handler(server, &socket);
  for (size_t i = 0; i < WEB_ASSET_COUNT; i++) {
    httpd_uri_t page = {};
    page.uri = WEB_ASSETS[i].path;
    page.method = HTTP_GET;
    page.handler = onAsset;
    page.user_ctx = (void *)&WEB_ASSETS[i];
    httpd_register_uri_handler(server, &page);
  }
}

void handleEvents(uint32_t now) {
  if (eventsLost) {
    eventsLost = false;
    phoneMessagesLost();
  }
  NetEvent event;
  while (xQueueReceive(events, &event, 0) == pdTRUE) {
    uint32_t client = clientOf(event.fd);
    if (event.kind == NetEvent::CONNECT) {
      if (!phoneConnected(client, now)) {
        sendText(client, "{\"type\":\"busy\",\"message\":\"Another device is driving the robot\"}");
        closeLater(client, now);
      }
    } else if (event.kind == NetEvent::DISCONNECT) {
      phoneDisconnected(client);
    } else if (const char *reply = phoneMessage(client, event.text, event.receivedMs, now)) {
      sendText(client, reply);
    }
  }
  for (auto &p : pendingClose)
    if (p.fd >= 0 && (int32_t)(now - p.atMs) >= 0) {
      closeClient(clientOf(p.fd));
      p.fd = -1;
    }
  if (uint32_t gone = phoneGoneQuiet(now)) closeClient(gone);
}

void sendTelemetry(uint32_t now) {
  static uint32_t lastMs = 0;
  if (now - lastMs < TELEMETRY_MS) return;
  lastMs = now;
  uint32_t client = phoneInControl();
  if (!client) return;
  char buf[200];
  int rssi = !hotspot && WiFi.status() == WL_CONNECTED ? (int)WiFi.RSSI() : 0;
  phoneStatus(buf, sizeof buf, hotspot, rssi);
  sendText(client, buf);
}

// ------------------------------------------------------------------- WiFi --

// Made while the radio is still off, with the true random source switched on.
String makePassword() {
  const char *chars = "abcdefghjkmnpqrstuvwxyz23456789";
  bootloader_random_enable();
  String pass;
  for (int i = 0; i < 10; i++) pass += chars[esp_random() % 31];
  bootloader_random_disable();
  return pass;
}

void startHotspot() {
  WiFi.disconnect(true);
  WiFi.mode(WIFI_AP);
  WiFi.softAP(HOTSPOT_NAME, apPass.c_str());
  WiFi.setSleep(false);  // power saving adds 100+ ms of lag to every message
  hotspot = true;
  wasConnected = false;
}

void joinNetwork() {
  WiFi.softAPdisconnect(true);
  WiFi.disconnect(true);
  WiFi.setHostname(WEB_NAME);
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(savedSsid.c_str(), savedPass.c_str());
  WiFi.setSleep(false);
  hotspot = false;
  wasConnected = false;
  joinedSinceStart = false;
  joinStartMs = millis();
}

void printWifiStatus() {
  if (hotspot) {
    if (!savedSsid.isEmpty())
      Serial.printf("   wifi: could not join %s, so the hotspot is on"
                    " (reset the robot to try again)\r\n",
                    savedSsid.c_str());
    Serial.printf("   wifi: hotspot %s, password %s\r\n", HOTSPOT_NAME, apPass.c_str());
    Serial.printf("   web joystick: http://%s\r\n", WiFi.softAPIP().toString().c_str());
  } else if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("   wifi: connected to %s, signal %d dBm\r\n", savedSsid.c_str(),
                  (int)WiFi.RSSI());
    Serial.printf("   web joystick: http://%s  or  http://%s.local\r\n",
                  WiFi.localIP().toString().c_str(), WEB_NAME);
    Serial.printf("   wifi: if it cannot join at power-up, it opens hotspot %s, password %s\r\n",
                  HOTSPOT_NAME, apPass.c_str());
  } else {
    Serial.printf("   wifi: joining %s ... (not connected yet)\r\n", savedSsid.c_str());
  }
}

// Joins, drops and the fallback to the hotspot. They are reported only after something
// has been typed in the Serial Monitor: lines printed earlier would wait in the USB
// buffer until the next Serial Monitor opens.
void pollWifi(uint32_t now) {
  static uint32_t lastPollMs = 0;
  if (now - lastPollMs < WIFI_POLL_MS || hotspot) return;
  lastPollMs = now;
  bool connected = WiFi.status() == WL_CONNECTED;
  if (connected != wasConnected) {
    wasConnected = connected;
    if (connected) joinedSinceStart = true;
    if (someoneTyped()) {
      if (connected) printWifiStatus();
      else Serial.printf("   wifi: lost %s, trying again\r\n", savedSsid.c_str());
    }
  }
  if (!connected && !joinedSinceStart && now - joinStartMs > WIFI_JOIN_TIMEOUT_MS) {
    startHotspot();
    if (someoneTyped()) printWifiStatus();
  }
}

}  // namespace

// ----------------------------------------------------------------- public --

void setupWifi() {
  events = xQueueCreate(64, sizeof(NetEvent));
  prefs.begin("net", false);
  // isKey first: reading a missing key logs an error from the core
  if (prefs.isKey("ssid")) savedSsid = prefs.getString("ssid", "");
  if (prefs.isKey("pass")) savedPass = prefs.getString("pass", "");
  if (prefs.isKey("ap_pass")) apPass = prefs.getString("ap_pass", "");
  if (apPass.length() < 8) {
    apPass = makePassword();
    prefs.putString("ap_pass", apPass);
  }
  if (savedSsid.isEmpty()) startHotspot();
  else joinNetwork();
  if (MDNS.begin(WEB_NAME)) MDNS.addService("http", "tcp", 80);
  startServer();
}

void updateWifi() {
  if (!server) return;
  uint32_t now = millis();
  handleEvents(now);
  sendTelemetry(now);
  pollWifi(now);
}

// `wifi`                      status and the page's address
// `wifi <name> <password>`    join a network and save it (put a name with spaces in quotes)
// `wifi off`                  delete it and run the robot's own hotspot
void wifiCommand(const char *args) {
  char text[128];
  while (*args == ' ') args++;
  strlcpy(text, args, sizeof text);
  size_t end = strlen(text);
  while (end && text[end - 1] == ' ') text[--end] = 0;

  if (!*text || !strcasecmp(text, "status")) {
    printWifiStatus();
    return;
  }
  if (!strcasecmp(text, "off")) {
    if (savedSsid.isEmpty() && hotspot) {
      printWifiStatus();
      return;
    }
    serialStop();  // the page drives over this link
    prefs.remove("ssid");
    prefs.remove("pass");
    savedSsid = savedPass = "";
    startHotspot();
    Serial.println("   wifi: network forgotten");
    printWifiStatus();
    return;
  }

  char ssid[33];
  size_t n = 0;
  const char *p = text;
  bool quoted = *p == '"';
  if (quoted) p++;
  while (*p && (quoted ? *p != '"' : *p != ' ') && n < 32) ssid[n++] = *p++;
  ssid[n] = 0;
  if (quoted && *p++ != '"') n = 0;  // no closing quote, or a name over 32 characters
  if (*p && *p != ' ') n = 0;        // name over 32 characters
  while (*p == ' ') p++;
  size_t passLen = strlen(p);
  bool reserved = !strcasecmp(ssid, "on") || !strcasecmp(ssid, "help") || !strcasecmp(ssid, "join");
  if (!n || reserved || passLen < 8 || passLen > 63) {
    serialStop();  // as for every invalid line
    Serial.println("   usage: wifi <name> <password>  (password 8-63 characters;"
                   " quote a name with spaces: wifi \"My Net\" <password>)");
    return;
  }

  serialStop();  // the page drives over this link
  savedSsid = ssid;
  savedPass = p;
  prefs.putString("ssid", savedSsid);
  prefs.putString("pass", savedPass);
  joinNetwork();
  Serial.printf("   wifi: joining %s and remembering it; type wifi to check\r\n", ssid);
}

#else  // ---- phone joystick off: empty stand-ins, so the rest of the program is unchanged ----

#include <Arduino.h>

#include "serial_commands.h"
#include "wifi_server.h"

void setupWifi() {}
void updateWifi() {}

// `wifi` still answers: it prints why there is no phone joystick. As with the joystick on,
// a plain `wifi` does not affect the robot, and `wifi` with anything after it (wifi off,
// wifi <name> <password>) stops it.
void wifiCommand(const char *args) {
  while (*args == ' ') args++;
  if (*args) serialStop();
#if defined(ESP32)
  Serial.println("   phone joystick is off: set REMOTE_CONTROL to 1 in config.h");
#else
  Serial.println("   the phone joystick needs the program to run on an ESP32"
                 " (on a UNO R4 WiFi the ESP32-S3 is only the WiFi modem)");
#endif
}

#endif
