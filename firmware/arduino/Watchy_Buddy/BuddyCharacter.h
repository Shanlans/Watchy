#pragma once
#include <Arduino.h>
#include <GxEPD2_BW.h>
#include <epd/GxEPD2_154_D67.h>
#include "BuddyState.h"

enum BuddyMood {
  MOOD_OFFLINE = 0,
  MOOD_IDLE,
  MOOD_BUSY,
  MOOD_ATTENTION,
  MOOD_SLEEP,
};

BuddyMood deriveMood(BuddyState& st, uint32_t nowMs);

// Draw the buddy face centered at (cx, cy). Face occupies approximately
// a 96x96 region around the center.
void drawBuddy(GxEPD2_BW<GxEPD2_154_D67, GxEPD2_154_D67::HEIGHT>& d,
               BuddyMood mood, int cx, int cy);
