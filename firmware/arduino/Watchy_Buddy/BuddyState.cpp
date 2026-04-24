#include "BuddyState.h"

String BuddyState::handleLine(const String& line) {
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, line);
  if (err) return String();

  lastHeartbeatMs = millis();
  dirty = true;

  // Heartbeat snapshot: total/running/waiting
  if (doc["total"].is<int>()) {
    total = doc["total"] | 0;
    running = doc["running"] | 0;
    waiting = doc["waiting"] | 0;
    msg = doc["msg"] | "";
    tokens = doc["tokens"] | 0;
    tokensToday = doc["tokens_today"] | 0;

    // entries — newest first, take up to 3
    entryCount = 0;
    if (doc["entries"].is<JsonArray>()) {
      JsonArray arr = doc["entries"].as<JsonArray>();
      for (JsonVariant v : arr) {
        if (entryCount >= 3) break;
        entries[entryCount++] = v.as<String>();
      }
    }

    // prompt
    if (doc["prompt"].is<JsonObject>()) {
      String newId = doc["prompt"]["id"] | "";
      bool newActive = (newId.length() > 0);
      // Record startMs only on transition from inactive to active (fresh prompt).
      if (newActive && (!prompt.active || prompt.id != newId)) {
        prompt.startMs = millis();
      }
      prompt.id = newId;
      prompt.tool = doc["prompt"]["tool"] | "";
      prompt.hint = doc["prompt"]["hint"] | "";
      prompt.active = newActive;
    } else {
      prompt.active = false;
      prompt.id = "";
      prompt.tool = "";
      prompt.hint = "";
      prompt.startMs = 0;
    }

    // Milestone celebrate: fire once per upward crossing.
    // Thresholds: 1K, 5K, 10K, 50K, 100K tokens_today.
    static const uint32_t thresholds[] = {1000, 5000, 10000, 50000, 100000};
    for (uint32_t t : thresholds) {
      if (tokensToday >= t && lastCelebrationMilestone < t) {
        lastCelebrationMilestone = t;
        events.celebrate = true;
        break;   // fire at most one per heartbeat
      }
    }

    return String();
  }

  // Time sync
  if (doc["time"].is<JsonArray>()) {
    // We don't use time in Phase A; just ack silently (no response specified in spec).
    return String();
  }

  // Commands with ack
  const char* cmd = doc["cmd"];
  if (cmd) {
    JsonDocument reply;
    reply["ack"] = cmd;
    reply["ok"] = true;
    reply["n"] = 0;

    // Pack commands are dispatched by main loop (needs BuddyPack access)
    if (strncmp(cmd, "pack_", 5) == 0) {
      return String("PACK:") + line;
    }

    if (strcmp(cmd, "status") == 0) {
      JsonObject data = reply["data"].to<JsonObject>();
      data["name"] = deviceName.length() ? deviceName : "Claude-Watchy";
      data["sec"] = false; // Phase A: no LE Secure Connections bonding
      JsonObject sys = data["sys"].to<JsonObject>();
      sys["up"] = (uint32_t)(millis() / 1000);
      sys["heap"] = (uint32_t)ESP.getFreeHeap();
      JsonObject stats = data["stats"].to<JsonObject>();
      stats["appr"] = approvedCount;
      stats["deny"] = deniedCount;
    } else if (strcmp(cmd, "name") == 0) {
      deviceName = doc["name"] | "";
    } else if (strcmp(cmd, "owner") == 0) {
      ownerName = doc["name"] | "";
    } else if (strcmp(cmd, "unpair") == 0) {
      // Phase A: no stored bonds yet; always ok
    } else if (strcmp(cmd, "permission") == 0) {
      // This shouldn't come from desktop — but be safe
      return String();
    }

    String out;
    serializeJson(reply, out);
    out += '\n';
    return out;
  }

  return String();
}

void BuddyState::noteApproval(uint32_t nowMs) {
  // Fire HEART event if user approved within 5 sec of prompt appearing.
  if (prompt.active && prompt.startMs > 0) {
    uint32_t dt = nowMs - prompt.startMs;
    if (dt < 5000) events.heart = true;
  }
}

String BuddyState::buildPermissionReply(const String& id, const String& decision) {
  JsonDocument doc;
  doc["cmd"] = "permission";
  doc["id"] = id;
  doc["decision"] = decision;
  String out;
  serializeJson(doc, out);
  out += '\n';
  return out;
}
