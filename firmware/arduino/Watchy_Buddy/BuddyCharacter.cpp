#include "BuddyCharacter.h"
#include <Fonts/FreeMonoBold9pt7b.h>

// How long of "total == 0" before we decide Claude is asleep
static const uint32_t SLEEP_THRESHOLD_MS = 5UL * 60UL * 1000UL;
static uint32_t s_idleSince = 0;

BuddyMood deriveMood(BuddyState& st, uint32_t nowMs) {
  if (!st.connected || st.isStale(nowMs)) {
    s_idleSince = 0;
    return MOOD_OFFLINE;
  }
  if (st.prompt.active)  return MOOD_ATTENTION;
  if (st.running > 0)    { s_idleSince = 0; return MOOD_BUSY; }

  if (st.total == 0) {
    if (s_idleSince == 0) s_idleSince = nowMs;
    if (nowMs - s_idleSince > SLEEP_THRESHOLD_MS) return MOOD_SLEEP;
  } else {
    s_idleSince = 0;
  }
  return MOOD_IDLE;
}

// Helper: fill a filled arc (approximation) for mouth shapes
static void drawArc(GxEPD2_BW<GxEPD2_154_D67, GxEPD2_154_D67::HEIGHT>& d,
                    int cx, int cy, int r, bool smile) {
  // Draw 3 segments approximating an arc (smile curves down, frown curves up)
  if (smile) {
    d.drawLine(cx - r,     cy,         cx - r / 2, cy + r / 2, GxEPD_BLACK);
    d.drawLine(cx - r / 2, cy + r / 2, cx + r / 2, cy + r / 2, GxEPD_BLACK);
    d.drawLine(cx + r / 2, cy + r / 2, cx + r,     cy,         GxEPD_BLACK);
  } else {
    d.drawLine(cx - r,     cy,         cx - r / 2, cy - r / 2, GxEPD_BLACK);
    d.drawLine(cx - r / 2, cy - r / 2, cx + r / 2, cy - r / 2, GxEPD_BLACK);
    d.drawLine(cx + r / 2, cy - r / 2, cx + r,     cy,         GxEPD_BLACK);
  }
}

void drawBuddy(GxEPD2_BW<GxEPD2_154_D67, GxEPD2_154_D67::HEIGHT>& d,
               BuddyMood mood, int cx, int cy) {
  const int HEAD_R = 46;          // head radius
  const int EYE_DX = 18;           // eye horizontal offset from center
  const int EYE_DY = -8;           // eye vertical offset from center (negative = above)
  const int EYE_R  = 5;            // eye radius
  const int MOUTH_DY = 18;         // mouth below center

  // Outer head: circle, filled black
  d.fillCircle(cx, cy, HEAD_R, GxEPD_BLACK);
  // Inner face: white
  d.fillCircle(cx, cy, HEAD_R - 4, GxEPD_WHITE);

  // Eyes per mood
  switch (mood) {
    case MOOD_SLEEP:
      // Horizontal lines (closed eyes)
      d.drawLine(cx - EYE_DX - EYE_R, cy + EYE_DY,
                 cx - EYE_DX + EYE_R, cy + EYE_DY, GxEPD_BLACK);
      d.drawLine(cx + EYE_DX - EYE_R, cy + EYE_DY,
                 cx + EYE_DX + EYE_R, cy + EYE_DY, GxEPD_BLACK);
      // "Z" floating top-right of head
      d.setFont(&FreeMonoBold9pt7b);
      d.setTextColor(GxEPD_BLACK);
      d.setCursor(cx + HEAD_R - 12, cy - HEAD_R + 6);
      d.print("z");
      d.setCursor(cx + HEAD_R - 4, cy - HEAD_R + 14);
      d.print("Z");
      break;

    case MOOD_ATTENTION:
      // Wide eyes (bigger filled circles) — alert look
      d.fillCircle(cx - EYE_DX, cy + EYE_DY, EYE_R + 2, GxEPD_BLACK);
      d.fillCircle(cx + EYE_DX, cy + EYE_DY, EYE_R + 2, GxEPD_BLACK);
      // Exclamation mark to the right
      d.setFont(&FreeMonoBold9pt7b);
      d.setTextColor(GxEPD_BLACK);
      d.setCursor(cx + HEAD_R - 2, cy + 4);
      d.print("!");
      break;

    case MOOD_BUSY:
      // Normal eyes
      d.fillCircle(cx - EYE_DX, cy + EYE_DY, EYE_R, GxEPD_BLACK);
      d.fillCircle(cx + EYE_DX, cy + EYE_DY, EYE_R, GxEPD_BLACK);
      // Sweat drop on upper-right of head (water droplet approximation)
      {
        int dx = cx + EYE_DX + 10;
        int dy = cy - EYE_DY - 8;
        d.fillTriangle(dx, dy - 6, dx - 3, dy - 2, dx + 3, dy - 2, GxEPD_BLACK);
        d.fillCircle(dx, dy + 1, 3, GxEPD_BLACK);
      }
      break;

    case MOOD_OFFLINE:
      // Eyes as small x's (disconnected)
      d.drawLine(cx - EYE_DX - EYE_R, cy + EYE_DY - EYE_R,
                 cx - EYE_DX + EYE_R, cy + EYE_DY + EYE_R, GxEPD_BLACK);
      d.drawLine(cx - EYE_DX - EYE_R, cy + EYE_DY + EYE_R,
                 cx - EYE_DX + EYE_R, cy + EYE_DY - EYE_R, GxEPD_BLACK);
      d.drawLine(cx + EYE_DX - EYE_R, cy + EYE_DY - EYE_R,
                 cx + EYE_DX + EYE_R, cy + EYE_DY + EYE_R, GxEPD_BLACK);
      d.drawLine(cx + EYE_DX - EYE_R, cy + EYE_DY + EYE_R,
                 cx + EYE_DX + EYE_R, cy + EYE_DY - EYE_R, GxEPD_BLACK);
      break;

    case MOOD_IDLE:
    default:
      // Normal open eyes (dots)
      d.fillCircle(cx - EYE_DX, cy + EYE_DY, EYE_R, GxEPD_BLACK);
      d.fillCircle(cx + EYE_DX, cy + EYE_DY, EYE_R, GxEPD_BLACK);
      break;
  }

  // Mouth per mood
  switch (mood) {
    case MOOD_BUSY:
      // Zigzag worry mouth
      d.drawLine(cx - 12, cy + MOUTH_DY,     cx - 6,  cy + MOUTH_DY - 3, GxEPD_BLACK);
      d.drawLine(cx - 6,  cy + MOUTH_DY - 3, cx,      cy + MOUTH_DY,     GxEPD_BLACK);
      d.drawLine(cx,      cy + MOUTH_DY,     cx + 6,  cy + MOUTH_DY - 3, GxEPD_BLACK);
      d.drawLine(cx + 6,  cy + MOUTH_DY - 3, cx + 12, cy + MOUTH_DY,     GxEPD_BLACK);
      break;
    case MOOD_ATTENTION:
      // Small o mouth (surprise)
      d.drawCircle(cx, cy + MOUTH_DY, 4, GxEPD_BLACK);
      break;
    case MOOD_SLEEP:
      // Tiny line
      d.drawLine(cx - 4, cy + MOUTH_DY, cx + 4, cy + MOUTH_DY, GxEPD_BLACK);
      break;
    case MOOD_OFFLINE:
      // Frown
      drawArc(d, cx, cy + MOUTH_DY + 2, 10, false);
      break;
    case MOOD_IDLE:
    default:
      // Smile
      drawArc(d, cx, cy + MOUTH_DY - 2, 12, true);
      break;
  }
}
