// Watchy_Buddy — Claude Desktop Buddy on Watchy S3
// Phase A: BLE Nordic UART + text-only heartbeat display.
// Hardware: ESP32-S3 + 200x200 E-Ink (GDEH0154D67).
//
// This sketch does NOT sleep. It stays awake to keep BLE advertising/connected.
// Expect a few hours of battery life; Phase C will add an on/off mode.

#include <Arduino.h>
#include <ArduinoJson.h>
#include "BuddyState.h"
#include "BuddyBLE.h"
#include "BuddyUI.h"
#include "BuddyPack.h"

// Button pins (ESP32-S3 Watchy V3)
static const int MENU_BTN = 7;
static const int BACK_BTN = 6;
static const int UP_BTN   = 0;
static const int DOWN_BTN = 45;

BuddyState state;
BuddyBLE   ble;
BuddyUI    ui;
BuddyPack  pack;

// Debounced button edge detection
struct Btn {
  int pin;
  bool lastStable = false;  // active-high on S3: HIGH = pressed
  bool stable    = false;
  uint32_t lastChangeMs = 0;
};

Btn btnMenu{MENU_BTN};
Btn btnBack{BACK_BTN};
Btn btnUp  {UP_BTN};
Btn btnDown{DOWN_BTN};

// Returns true on rising edge (press) after 30ms debounce
static bool pollEdge(Btn& b) {
  bool now = (digitalRead(b.pin) == HIGH);
  uint32_t t = millis();
  if (now != b.stable) {
    if (t - b.lastChangeMs > 30) {
      b.stable = now;
      b.lastChangeMs = t;
      if (now && !b.lastStable) {
        b.lastStable = true;
        return true;
      }
    }
  } else {
    b.lastChangeMs = t;
    if (!now) b.lastStable = false;
  }
  return false;
}

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println(F("[Watchy_Buddy] boot"));

  pinMode(MENU_BTN, INPUT);
  pinMode(BACK_BTN, INPUT);
  pinMode(UP_BTN,   INPUT);
  pinMode(DOWN_BTN, INPUT);

  // Mount FFat + load previously active pack before BLE/UI
  pack.begin();
  String savedPack = pack.loadActivePack();
  if (savedPack.length() > 0) {
    pack.load(savedPack);
  }

  ble.begin(&state);
  ui.begin();

  Serial.print(F("[Watchy_Buddy] advertising as: "));
  Serial.println(ble.deviceName);
  state.deviceName = ble.deviceName;
  state.dirty = true;
}

void loop() {
  ble.tick();

  // Button handling — permission accept/deny via Menu/Back
  if (pollEdge(btnMenu)) {
    if (state.prompt.active) {
      uint32_t now = millis();
      state.noteApproval(now);               // may set events.heart if <5 sec
      String reply = state.buildPermissionReply(state.prompt.id, "once");
      ble.sendLine(reply);
      state.approvedCount++;
      state.prompt.active = false;
      state.dirty = true;
    }
  }
  if (pollEdge(btnBack)) {
    if (state.prompt.active) {
      String reply = state.buildPermissionReply(state.prompt.id, "deny");
      ble.sendLine(reply);
      state.deniedCount++;
      state.prompt.active = false;
      state.dirty = true;
    }
  }
  // Up/Down reserved for Phase B+
  pollEdge(btnUp);
  pollEdge(btnDown);

  // Advance animation state (blink, override expiry)
  ui.tick(state);

  // Handle pack commands from BLE
  if (ble.hasPendingPackCmd()) {
    String raw = ble.popPackCmd();
    JsonDocument doc;
    if (!deserializeJson(doc, raw)) {
      const char* cmd = doc["cmd"];
      String ack;
      JsonDocument reply;

      if (strcmp(cmd, "pack_begin") == 0) {
        String name = doc["name"] | "unknown";
        uint32_t size = doc["size"] | 0;
        bool ok = pack.createPackFile(name, size);
        reply["ack"] = "pack_begin"; reply["ok"] = ok;
      }
      else if (strcmp(cmd, "pack_chunk") == 0) {
        String hexData = doc["data"] | "";
        size_t len = hexData.length() / 2;
        uint8_t* buf = (uint8_t*)malloc(len);
        if (buf) {
          for (size_t i = 0; i < len; ++i) {
            char hi = hexData[i*2], lo = hexData[i*2+1];
            auto h = [](char c) -> uint8_t { return (c >= 'a') ? c-'a'+10 : (c >= 'A') ? c-'A'+10 : c-'0'; };
            buf[i] = (h(hi) << 4) | h(lo);
          }
          pack.appendChunk(buf, len);
          free(buf);
        }
        reply["ack"] = "pack_chunk"; reply["ok"] = true; reply["n"] = (int)len;
      }
      else if (strcmp(cmd, "pack_end") == 0) {
        uint32_t crc = doc["crc"] | 0;
        bool ok = pack.finalizePackFile(crc);
        reply["ack"] = "pack_end"; reply["ok"] = ok;
      }
      else if (strcmp(cmd, "pack_select") == 0) {
        String name = doc["name"] | "";
        bool ok = pack.load(name);
        if (ok) pack.saveActivePack(name);
        state.dirty = true;
        ui.forceFullRefresh();  // bypass content hash — pack changed
        reply["ack"] = "pack_select"; reply["ok"] = ok;
      }
      else if (strcmp(cmd, "pack_clear") == 0) {
        pack.unload();
        pack.saveActivePack("");
        state.dirty = true;
        ui.forceFullRefresh();
        reply["ack"] = "pack_clear"; reply["ok"] = true;
      }
      else if (strcmp(cmd, "pack_list") == 0) {
        reply["ack"] = "pack_list"; reply["ok"] = true;
        reply["packs"] = serialized(pack.listPacks());
      }

      String out;
      serializeJson(reply, out);
      out += '\n';
      ble.sendLine(out);
    }
  }

  // Periodic UI refresh (1 Hz)
  static uint32_t lastRender = 0;
  uint32_t now = millis();
  if (now - lastRender > 1000) {
    lastRender = now;
    // E-Ink redraw only if something actually changed or status dirty
    if (state.dirty || state.isStale(now)) {
      ui.render(state, &pack);
    }
  }

  delay(20);
}
