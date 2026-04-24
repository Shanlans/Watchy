// Watchy_Buddy — Claude Desktop Buddy on Watchy S3
// Phase A: BLE Nordic UART + text-only heartbeat display.
// Hardware: ESP32-S3 + 200x200 E-Ink (GDEH0154D67).
//
// This sketch does NOT sleep. It stays awake to keep BLE advertising/connected.
// Expect a few hours of battery life; Phase C will add an on/off mode.

#include <Arduino.h>
#include "BuddyState.h"
#include "BuddyBLE.h"
#include "BuddyUI.h"

// Button pins (ESP32-S3 Watchy V3)
static const int MENU_BTN = 7;
static const int BACK_BTN = 6;
static const int UP_BTN   = 0;
static const int DOWN_BTN = 45;

BuddyState state;
BuddyBLE   ble;
BuddyUI    ui;

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

  // Start BLE FIRST so advertising begins immediately on boot, before the
  // slower E-Ink refresh delays user-visible UI. BLE callbacks run on their
  // own task so they are not blocked by the display refresh below.
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

  // Periodic UI refresh (1 Hz)
  static uint32_t lastRender = 0;
  uint32_t now = millis();
  if (now - lastRender > 1000) {
    lastRender = now;
    // E-Ink redraw only if something actually changed or status dirty
    if (state.dirty || state.isStale(now)) {
      ui.render(state);
    }
  }

  delay(20);
}
