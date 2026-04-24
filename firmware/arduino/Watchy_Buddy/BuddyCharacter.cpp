#include "BuddyCharacter.h"
#include "BuddyPack.h"
#include "BlobAscii.h"
#include <Fonts/FreeMonoBold9pt7b.h>
#include <Fonts/FreeMono9pt7b.h>

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
               BuddyMood mood, int cx, int cy,
               BuddyPack* pack, uint32_t nowMs) {

  // If a character pack is loaded, try to render the bitmap for this mood.
  if (pack && pack->isLoaded()) {
    uint8_t stateId = (uint8_t)mood;
    // Map blink to idle (pack has no blink-specific frame)
    if (mood == MOOD_IDLE_BLINK) stateId = (uint8_t)MOOD_IDLE;

    // For IDLE: rotate through variants based on time
    uint8_t variant = 0;
    int numVariants = pack->variantCount(stateId);
    if (numVariants > 1 && nowMs > 0) {
      variant = (nowMs / 800) % numVariants;   // rotate every 800 ms
    }

    int fw = 0, fh = 0;
    const uint8_t* bmp = pack->getFrame(stateId, variant, fw, fh);
    if (bmp) {
      d.drawBitmap(cx - fw / 2, cy - fh / 2, bmp, fw, fh, GxEPD_BLACK);
      return;
    }
    // Fall through to ASCII blob if bitmap frame not found
  }

  // --- ASCII blob rendering (default character) ---
  {
    const BlobPose* poses = nullptr;
    const uint8_t* seq = nullptr;
    int seqLen = 0;
    int numPoses = 0;

    BuddyMood lookupMood = mood;
    if (lookupMood == MOOD_IDLE_BLINK) lookupMood = MOOD_IDLE;  // blink uses idle seq

    switch (lookupMood) {
      case MOOD_SLEEP:     poses = BLOB_SLEEP;     seq = BLOB_SLEEP_SEQ;  seqLen = BLOB_SLEEP_SEQ_LEN;  numPoses = 6;  break;
      case MOOD_IDLE:      poses = BLOB_IDLE;       seq = BLOB_IDLE_SEQ;   seqLen = BLOB_IDLE_SEQ_LEN;   numPoses = 10; break;
      case MOOD_BUSY:      poses = BLOB_BUSY;       seq = BLOB_BUSY_SEQ;   seqLen = BLOB_BUSY_SEQ_LEN;   numPoses = 6;  break;
      case MOOD_ATTENTION: poses = BLOB_ATTENTION;  seq = BLOB_ATT_SEQ;    seqLen = BLOB_ATT_SEQ_LEN;    numPoses = 6;  break;
      case MOOD_CELEBRATE: poses = BLOB_CELEBRATE;  seq = BLOB_CEL_SEQ;    seqLen = BLOB_CEL_SEQ_LEN;    numPoses = 6;  break;
      case MOOD_HEART:     poses = BLOB_HEART;      seq = BLOB_HEART_SEQ;  seqLen = BLOB_HEART_SEQ_LEN;  numPoses = 5;  break;
      case MOOD_OFFLINE:   poses = BLOB_DIZZY;      seq = BLOB_DIZZY_SEQ;  seqLen = BLOB_DIZZY_SEQ_LEN;  numPoses = 5;  break;
      default:             poses = BLOB_IDLE;        seq = BLOB_IDLE_SEQ;   seqLen = BLOB_IDLE_SEQ_LEN;   numPoses = 10; break;
    }

    // Pick pose from sequence based on time (advance every ~10 sec / heartbeat)
    uint8_t beat = 0;
    if (nowMs > 0 && seqLen > 0) {
      beat = (nowMs / 10000) % seqLen;  // one pose per ~10 sec (each heartbeat)
    }
    uint8_t poseIdx = seq[beat];
    if (poseIdx >= numPoses) poseIdx = 0;

    // For blink: override to BLINK pose (index 5 in IDLE set = closed eyes)
    if (mood == MOOD_IDLE_BLINK && numPoses > 5) poseIdx = 5;

    const BlobPose& p = poses[poseIdx];

    // Render 5 lines of ASCII art centered around (cx, cy)
    d.setFont(&FreeMono9pt7b);
    d.setTextColor(GxEPD_BLACK);
    int lineH = 14;
    int startY = cy - (BLOB_LINES * lineH) / 2 + lineH;
    for (int i = 0; i < BLOB_LINES; ++i) {
      // Center each line: each char ~7px wide in FreeMono9pt
      int len = strlen(p.lines[i]);
      int textW = len * 7;
      d.setCursor(cx - textW / 2, startY + i * lineH);
      d.print(p.lines[i]);
    }
    return;  // done — skip GFX primitives below
  }

  // --- GFX primitive fallback (only reached if ASCII data missing) ---
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
    case MOOD_IDLE_BLINK:
      // Closed eyes: short horizontal lines
      d.drawLine(cx - EYE_DX - EYE_R, cy + EYE_DY,
                 cx - EYE_DX + EYE_R, cy + EYE_DY, GxEPD_BLACK);
      d.drawLine(cx + EYE_DX - EYE_R, cy + EYE_DY,
                 cx + EYE_DX + EYE_R, cy + EYE_DY, GxEPD_BLACK);
      break;

    case MOOD_HEART: {
      // Heart-shaped eyes: two circles on top, triangle pointing down
      auto heart = [&](int ex, int ey) {
        d.fillCircle(ex - 3, ey - 1, 3, GxEPD_BLACK);
        d.fillCircle(ex + 3, ey - 1, 3, GxEPD_BLACK);
        d.fillTriangle(ex - 5, ey + 1, ex + 5, ey + 1, ex, ey + 6, GxEPD_BLACK);
      };
      heart(cx - EYE_DX, cy + EYE_DY);
      heart(cx + EYE_DX, cy + EYE_DY);
      break;
    }

    case MOOD_CELEBRATE: {
      // Star-ish eyes: 4-point burst (two crossing lines + center dot)
      auto star = [&](int ex, int ey) {
        d.drawLine(ex - 6, ey, ex + 6, ey, GxEPD_BLACK);
        d.drawLine(ex, ey - 6, ex, ey + 6, GxEPD_BLACK);
        d.drawLine(ex - 4, ey - 4, ex + 4, ey + 4, GxEPD_BLACK);
        d.drawLine(ex - 4, ey + 4, ex + 4, ey - 4, GxEPD_BLACK);
        d.fillCircle(ex, ey, 2, GxEPD_BLACK);
      };
      star(cx - EYE_DX, cy + EYE_DY);
      star(cx + EYE_DX, cy + EYE_DY);
      // Spark lines radiating around the head
      for (int a = 0; a < 8; ++a) {
        float rad = a * (3.14159f / 4.0f);
        int x1 = cx + (int)((HEAD_R + 4)  * cos(rad));
        int y1 = cy + (int)((HEAD_R + 4)  * sin(rad));
        int x2 = cx + (int)((HEAD_R + 12) * cos(rad));
        int y2 = cy + (int)((HEAD_R + 12) * sin(rad));
        d.drawLine(x1, y1, x2, y2, GxEPD_BLACK);
      }
      break;
    }

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
    case MOOD_CELEBRATE:
      // Big open smile (arc with filled interior hint)
      drawArc(d, cx, cy + MOUTH_DY - 4, 14, true);
      d.drawLine(cx - 14, cy + MOUTH_DY - 4, cx + 14, cy + MOUTH_DY - 4, GxEPD_BLACK);
      break;
    case MOOD_HEART:
      // Soft smile
      drawArc(d, cx, cy + MOUTH_DY - 1, 10, true);
      break;
    case MOOD_IDLE_BLINK:
      // Same as IDLE — smile (blink keeps the smile stable so transition is minimal)
      drawArc(d, cx, cy + MOUTH_DY - 2, 12, true);
      break;
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
