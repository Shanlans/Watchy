#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>

struct PendingPrompt {
  String id;
  String tool;
  String hint;
  bool active = false;
  uint32_t startMs = 0;   // millis() when prompt first seen; for <5s fast-approve HEART detection
};

// Event flags the UI layer consumes to run one-shot animations. Cleared by UI
// after the animation starts.
struct BuddyEvents {
  bool celebrate = false;
  bool heart     = false;
};

class BuddyState {
public:
  bool connected = false;
  uint32_t lastHeartbeatMs = 0;
  String ownerName;
  String deviceName;

  // heartbeat fields
  int total = 0;
  int running = 0;
  int waiting = 0;
  String msg;
  String entries[3];
  int entryCount = 0;
  uint32_t tokens = 0;
  uint32_t tokensToday = 0;
  PendingPrompt prompt;

  // stats for status ack
  uint32_t approvedCount = 0;
  uint32_t deniedCount = 0;

  // Milestone tracking for celebrate events (only fires on upward crossing).
  // Each entry is the highest threshold already passed.
  uint32_t lastCelebrationMilestone = 0;

  // Events (one-shot flags consumed by UI)
  BuddyEvents events;

  // mark UI dirty
  bool dirty = true;

  // Parse one JSON line received from Claude Desktop.
  // Returns a response JSON line (including trailing \n) to send back, or empty string if no response needed.
  String handleLine(const String& line);

  // True if no heartbeat for >30 sec
  bool isStale(uint32_t nowMs) const {
    if (!connected) return true;
    if (lastHeartbeatMs == 0) return true;
    return (nowMs - lastHeartbeatMs) > 30000;
  }

  // Build permission response
  String buildPermissionReply(const String& id, const String& decision);

  // Called when user just pressed approve/deny. Checks if it was fast (<5s)
  // for HEART event trigger.
  void noteApproval(uint32_t nowMs);
};
