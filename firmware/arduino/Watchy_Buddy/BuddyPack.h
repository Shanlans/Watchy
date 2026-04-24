#pragma once
#include <Arduino.h>
#include <FFat.h>

// Max frames a single pack can hold
#define PACK_MAX_FRAMES 32

struct PackFrameEntry {
  uint8_t  state;
  uint8_t  variant;
  uint16_t durationMs;
  uint32_t offset;      // byte offset into data section
  uint32_t size;
};

class BuddyPack {
public:
  bool begin();   // mount FFat; call once in setup()

  // Load a pack from /packs/<name>.pack. Returns true if valid.
  bool load(const String& name);
  void unload();
  bool isLoaded() const { return _loaded; }
  String activeName() const { return _name; }

  // Get bitmap for a given state + variant. Returns nullptr if not found.
  // outW/outH set to frame dimensions.
  const uint8_t* getFrame(uint8_t state, uint8_t variant, int& outW, int& outH);

  // Count how many variants exist for a state (e.g. idle has 9)
  int variantCount(uint8_t state) const;

  // File I/O for BLE upload
  bool createPackFile(const String& name, uint32_t expectedSize);
  bool appendChunk(const uint8_t* data, size_t len);
  bool finalizePackFile(uint32_t expectedCrc);

  // List installed packs
  String listPacks();

  // Persist / restore active pack name via NVS Preferences
  void saveActivePack(const String& name);
  String loadActivePack();

private:
  bool     _mounted = false;
  bool     _loaded = false;
  String   _name;
  uint16_t _frameW = 96;
  uint16_t _frameH = 96;
  uint16_t _numFrames = 0;
  PackFrameEntry _frames[PACK_MAX_FRAMES];

  // Loaded frame data in RAM (small enough: 15 frames × 1152 bytes = ~17 KB)
  uint8_t* _data = nullptr;
  uint32_t _dataSize = 0;

  // Upload state
  File     _uploadFile;
  uint32_t _uploadExpectedSize = 0;
  uint32_t _uploadWritten = 0;
  String   _uploadName;
};
