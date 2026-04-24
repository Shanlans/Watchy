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
  MOOD_CELEBRATE,
  MOOD_HEART,
  MOOD_IDLE_BLINK,
};

BuddyMood deriveMood(BuddyState& st, uint32_t nowMs);

class BuddyPack;  // forward decl

// Draw the buddy face centered at (cx, cy). If pack is loaded, uses bitmap;
// otherwise falls back to GFX primitives.
void drawBuddy(GxEPD2_BW<GxEPD2_154_D67, GxEPD2_154_D67::HEIGHT>& d,
               BuddyMood mood, int cx, int cy,
               BuddyPack* pack = nullptr, uint32_t nowMs = 0);
