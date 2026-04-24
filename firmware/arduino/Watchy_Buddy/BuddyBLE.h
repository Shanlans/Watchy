#pragma once
#include <Arduino.h>
#include "BuddyState.h"

// Nordic UART Service UUIDs (per Claude Buddy REFERENCE.md)
#define NUS_SERVICE_UUID    "6e400001-b5a3-f393-e0a9-e50e24dcca9e"
#define NUS_RX_CHAR_UUID    "6e400002-b5a3-f393-e0a9-e50e24dcca9e"  // desktop -> device (write)
#define NUS_TX_CHAR_UUID    "6e400003-b5a3-f393-e0a9-e50e24dcca9e"  // device -> desktop (notify)

class BuddyBLE {
public:
  void begin(BuddyState* state);
  void tick();

  void sendLine(const String& line);

  // Pack commands received via BLE are queued here for main-loop dispatch
  bool hasPendingPackCmd() const;
  String popPackCmd();

  String deviceName;
};
