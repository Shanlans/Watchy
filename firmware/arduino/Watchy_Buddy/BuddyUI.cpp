#include "BuddyUI.h"
#include "BuddyCharacter.h"
#include <GxEPD2_BW.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include <SPI.h>

// Watchy S3 V3 display pins (from Watchy/src/config.h)
static const int PIN_DISPLAY_CS   = 33;
static const int PIN_DISPLAY_DC   = 34;
static const int PIN_DISPLAY_RES  = 35;
static const int PIN_DISPLAY_BUSY = 36;
static const int PIN_SPI_MOSI     = 48;
static const int PIN_SPI_MISO     = 46;
static const int PIN_SPI_SCK      = 47;

static GxEPD2_BW<GxEPD2_154_D67, GxEPD2_154_D67::HEIGHT> eink(
  GxEPD2_154_D67(PIN_DISPLAY_CS, PIN_DISPLAY_DC, PIN_DISPLAY_RES, PIN_DISPLAY_BUSY)
);

// Refresh timing
static const uint32_t FULL_REFRESH_INTERVAL_MS = 5UL * 60UL * 1000UL;  // 5 min

// Blink: when idle, eyes closed for 250 ms every ~4.5 sec
static const uint32_t BLINK_PERIOD_MS = 4500;
static const uint32_t BLINK_DURATION_MS = 250;

// Event mood display durations
static const uint32_t CELEBRATE_DURATION_MS = 3000;
static const uint32_t HEART_DURATION_MS     = 2000;

static String fit(const String& s, int maxChars) {
  if ((int)s.length() <= maxChars) return s;
  if (maxChars <= 3) return s.substring(0, maxChars);
  return s.substring(0, maxChars - 3) + "...";
}

static uint32_t hash32(uint32_t h, uint32_t v) {
  h ^= v + 0x9e3779b9UL + (h << 6) + (h >> 2);
  return h;
}
static uint32_t hashStr(uint32_t h, const String& s) {
  for (size_t i = 0; i < s.length(); ++i) h = hash32(h, (uint8_t)s[i]);
  return h;
}

void BuddyUI::begin() {
  SPI.begin(PIN_SPI_SCK, PIN_SPI_MISO, PIN_SPI_MOSI, PIN_DISPLAY_CS);
  eink.init(0, true, 2, false);
  eink.setRotation(0);
  eink.setTextWrap(false);
  forcedFull = true;
  nextBlinkAt = millis() + 2000;
}

void BuddyUI::forceFullRefresh() { forcedFull = true; }

void BuddyUI::tick(BuddyState& st) {
  const uint32_t now = millis();

  // Consume one-shot events → turn into timed overrides
  if (st.events.celebrate) {
    st.events.celebrate = false;
    overrideMood  = MOOD_CELEBRATE;
    overrideEndAt = now + CELEBRATE_DURATION_MS;
    st.dirty = true;
  }
  if (st.events.heart) {
    st.events.heart = false;
    overrideMood  = MOOD_HEART;
    overrideEndAt = now + HEART_DURATION_MS;
    st.dirty = true;
  }

  // Expire override
  if (overrideMood >= 0 && (int32_t)(now - overrideEndAt) >= 0) {
    overrideMood = -1;
    st.dirty = true;
  }

  // Blink scheduler (only schedules dirty flips; mood resolution in render())
  if (blinkEndAt > 0 && (int32_t)(now - blinkEndAt) >= 0) {
    blinkEndAt = 0;
    st.dirty = true;
  } else if (blinkEndAt == 0 && (int32_t)(now - nextBlinkAt) >= 0) {
    // Only blink when we'd otherwise be IDLE (not busy/attention/etc).
    BuddyMood m = deriveMood(st, now);
    if (m == MOOD_IDLE && overrideMood < 0) {
      blinkEndAt = now + BLINK_DURATION_MS;
      st.dirty = true;
    }
    nextBlinkAt = now + BLINK_PERIOD_MS;
  }
}

uint32_t BuddyUI::computeHash(BuddyState& st, int effMood) const {
  uint32_t h = 0xDEADBEEF;
  h = hash32(h, (uint32_t)effMood);
  h = hash32(h, (uint32_t)(st.connected ? 1 : 0));
  h = hash32(h, (uint32_t)st.running);
  h = hash32(h, (uint32_t)st.waiting);
  h = hash32(h, (uint32_t)st.total);
  h = hash32(h, (uint32_t)st.tokensToday);
  h = hash32(h, (uint32_t)(st.prompt.active ? 1 : 0));
  h = hashStr(h, st.msg);
  h = hashStr(h, st.prompt.id);
  h = hashStr(h, st.prompt.hint);
  if (st.entryCount > 0) h = hashStr(h, st.entries[0]);
  return h;
}

void BuddyUI::render(BuddyState& st) {
  const uint32_t now = millis();

  // Effective mood = override (celebrate/heart) > blink > derived
  BuddyMood derived = deriveMood(st, now);
  int effMood;
  if (overrideMood >= 0)          effMood = overrideMood;
  else if (blinkEndAt > 0 && derived == MOOD_IDLE) effMood = MOOD_IDLE_BLINK;
  else                            effMood = derived;

  // Decide full vs partial refresh
  bool wantFull = forcedFull;
  if (now - lastFullRefreshMs > FULL_REFRESH_INTERVAL_MS) wantFull = true;
  // Full refresh only when the prompt APPEARS (not when it clears) — prevents
  // the second "accept flash". The 5-min periodic full refresh cleans any
  // residual ghosting left by the partial refresh on prompt clear.
  if (st.prompt.active && !lastPromptActive) wantFull = true;
  if (effMood == MOOD_CELEBRATE || effMood == MOOD_HEART) {
    // Event moods look better with a clean refresh at start, handled by hash-change below
  }

  // Content hash — skip render if nothing visible changed
  uint32_t h = computeHash(st, effMood);
  bool contentChanged = (h != lastContentHash);
  if (!wantFull && !contentChanged) {
    // nothing to do; keep dirty cleared
    st.dirty = false;
    return;
  }

  // Set refresh window
  if (wantFull) {
    eink.setFullWindow();
  } else {
    eink.setPartialWindow(0, 0, 200, 200);
  }

  eink.firstPage();
  do {
    eink.fillScreen(GxEPD_WHITE);
    eink.setTextColor(GxEPD_BLACK);
    eink.setFont(&FreeMonoBold9pt7b);

    // Top bar
    eink.setCursor(2, 13);
    if (st.connected && !st.isStale(now)) {
      eink.print("* LIVE");
    } else {
      eink.print("o OFFLINE");
    }
    if (st.tokensToday > 0) {
      char tokBuf[16];
      if (st.tokensToday >= 1000)
        snprintf(tokBuf, sizeof(tokBuf), "T:%ldK", (long)(st.tokensToday / 1000));
      else
        snprintf(tokBuf, sizeof(tokBuf), "T:%ld", (long)st.tokensToday);
      int16_t x1, y1; uint16_t w, h2;
      eink.getTextBounds(tokBuf, 0, 0, &x1, &y1, &w, &h2);
      eink.setCursor(198 - (int)w, 13);
      eink.print(tokBuf);
    }
    eink.drawLine(0, 17, 200, 17, GxEPD_BLACK);

    // Buddy character center
    drawBuddy(eink, (BuddyMood)effMood, 100, 78);

    // Info strip
    if (st.running > 0 || st.waiting > 0 || st.total > 0) {
      char counts[40];
      snprintf(counts, sizeof(counts), "%d run %d wait %d total",
               st.running, st.waiting, st.total);
      eink.setCursor(2, 148);
      eink.print(counts);
    }
    if (st.msg.length() > 0) {
      eink.setCursor(2, 164);
      eink.print(fit(st.msg, 22));
    }

    // Bottom region
    if (st.prompt.active) {
      eink.drawLine(0, 177, 200, 177, GxEPD_BLACK);
      if (st.prompt.hint.length() > 0) {
        eink.setCursor(2, 189);
        eink.print(fit(st.prompt.hint, 22));
      }
      eink.setCursor(2, 199);
      eink.print("[M] yes  [B] no");
    } else if (!st.connected || st.isStale(now)) {
      eink.setCursor(2, 199);
      eink.print("Open Hardware Buddy");
    } else if (st.entryCount > 0) {
      eink.setCursor(2, 199);
      eink.print(fit(st.entries[0], 22));
    }
  } while (eink.nextPage());

  if (wantFull) {
    lastFullRefreshMs = now;
    forcedFull = false;
  }
  lastPromptActive = st.prompt.active;
  lastContentHash = h;
  st.dirty = false;
}
