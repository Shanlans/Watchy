#pragma once
#include <Arduino.h>
#include "BuddyState.h"

class BuddyUI {
public:
  void begin();                  // initialize display
  void render(BuddyState& st);   // partial redraw
  void forceFullRefresh();       // request a full (non-partial) refresh next render

private:
  uint32_t lastFullRefreshMs = 0;
  bool forcedFull = true;        // first render is always full
  bool lastPromptActive = false;
};
