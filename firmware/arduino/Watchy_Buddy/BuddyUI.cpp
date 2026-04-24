#include "BuddyUI.h"
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

// Use GxEPD2's built-in driver for the GDEH/GDEY 0154 D67 panel (200x200 mono B/W).
// WatchyDisplay in the Watchy library is a Watchy-specific fork of this; we go
// direct to GxEPD2 to avoid pulling in the Watchy library (whose BLE.cpp would
// link the old Bluedroid BLE stack alongside our NimBLE, causing GATT failures).
static GxEPD2_BW<GxEPD2_154_D67, GxEPD2_154_D67::HEIGHT> eink(
  GxEPD2_154_D67(PIN_DISPLAY_CS, PIN_DISPLAY_DC, PIN_DISPLAY_RES, PIN_DISPLAY_BUSY)
);

static const uint32_t FULL_REFRESH_INTERVAL_MS = 5UL * 60UL * 1000UL;

static String fit(const String& s, int maxChars) {
  if ((int)s.length() <= maxChars) return s;
  if (maxChars <= 3) return s.substring(0, maxChars);
  return s.substring(0, maxChars - 3) + "...";
}

void BuddyUI::begin() {
  SPI.begin(PIN_SPI_SCK, PIN_SPI_MISO, PIN_SPI_MOSI, PIN_DISPLAY_CS);
  eink.init(0, true, 2, false);
  eink.setRotation(0);
  eink.setTextWrap(false);
  forcedFull = true;
}

void BuddyUI::forceFullRefresh() { forcedFull = true; }

void BuddyUI::render(BuddyState& st) {
  const uint32_t now = millis();
  bool wantFull = forcedFull;
  if (now - lastFullRefreshMs > FULL_REFRESH_INTERVAL_MS) wantFull = true;
  if (st.prompt.active != lastPromptActive) wantFull = true;

  eink.setFullWindow();
  eink.setFont(&FreeMonoBold9pt7b);

  eink.firstPage();
  do {
    eink.fillScreen(GxEPD_WHITE);
    eink.setTextColor(GxEPD_BLACK);

    // Top bar
    eink.setCursor(2, 13);
    if (st.connected && !st.isStale(now)) {
      eink.print("* LIVE");
    } else {
      eink.print("o OFFLINE");
    }

    if (st.tokensToday > 0) {
      char tokBuf[16];
      if (st.tokensToday >= 1000) {
        snprintf(tokBuf, sizeof(tokBuf), "T:%ldK", (long)(st.tokensToday / 1000));
      } else {
        snprintf(tokBuf, sizeof(tokBuf), "T:%ld", (long)st.tokensToday);
      }
      int16_t x1, y1; uint16_t w, h;
      eink.getTextBounds(tokBuf, 0, 0, &x1, &y1, &w, &h);
      eink.setCursor(198 - (int)w, 13);
      eink.print(tokBuf);
    }

    eink.drawLine(0, 17, 200, 17, GxEPD_BLACK);

    eink.setCursor(2, 34);
    char line2[48];
    snprintf(line2, sizeof(line2), "sess:%d run:%d wait:%d",
             st.total, st.running, st.waiting);
    eink.print(line2);

    if (st.msg.length() > 0) {
      eink.setCursor(2, 54);
      eink.print(fit(st.msg, 22));
    }

    int y = 78;
    for (int i = 0; i < st.entryCount && i < 3; ++i) {
      eink.setCursor(2, y);
      eink.print(fit(st.entries[i], 22));
      y += 17;
    }

    if (st.prompt.active) {
      eink.drawLine(0, 140, 200, 140, GxEPD_BLACK);
      eink.setCursor(2, 155);
      eink.print(fit(String("? ") + st.prompt.tool, 22));
      if (st.prompt.hint.length() > 0) {
        eink.setCursor(2, 172);
        eink.print(fit(st.prompt.hint, 22));
      }
      eink.setCursor(2, 194);
      eink.print("[M] yes   [B] no");
    } else if (!st.connected) {
      eink.setCursor(2, 185);
      eink.print("Open Hardware Buddy");
      eink.setCursor(2, 199);
      eink.print("in Claude Desktop.");
    }
  } while (eink.nextPage());

  if (wantFull) {
    lastFullRefreshMs = now;
    forcedFull = false;
  }
  lastPromptActive = st.prompt.active;
  st.dirty = false;
}
