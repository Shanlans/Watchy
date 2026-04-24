#pragma once
#include <Arduino.h>
#include "BuddyState.h"

class BuddyUI {
public:
  void begin();
  void render(BuddyState& st);
  void forceFullRefresh();
  void tick(BuddyState& st);          // call every loop — advances animations, returns via dirty

private:
  uint32_t lastFullRefreshMs = 0;
  bool forcedFull = true;
  bool lastPromptActive = false;

  // Content hash to skip redundant partial refreshes
  uint32_t lastContentHash = 0;

  // Blink scheduler — when idle, eyes close briefly every few seconds
  uint32_t nextBlinkAt = 0;
  uint32_t blinkEndAt  = 0;          // 0 when not blinking

  // Override mood (celebrate, heart) with expiry
  int      overrideMood   = -1;      // -1 = none (type-compatible with BuddyMood enum)
  uint32_t overrideEndAt  = 0;

  // Compute a cheap 32-bit hash of what will be drawn this frame
  uint32_t computeHash(BuddyState& st, int effectiveMood) const;
};
