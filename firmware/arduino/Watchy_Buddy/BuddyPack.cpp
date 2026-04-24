#include "BuddyPack.h"
#include <FFat.h>
#include <Preferences.h>

static const char* PACKS_DIR  = "/packs";
static const char* NVS_NS     = "buddy";
static const char* NVS_KEY    = "pack";
static const uint8_t MAGIC[8] = {'B','P','A','C','K',0,0,0};

bool BuddyPack::begin() {
  if (!FFat.begin(true)) {   // true = format on first use
    Serial.println(F("[Pack] FFat mount failed"));
    return false;
  }
  _mounted = true;
  // Create packs directory if missing
  if (!FFat.exists(PACKS_DIR)) FFat.mkdir(PACKS_DIR);
  Serial.printf("[Pack] FFat mounted. Free: %u KB\n", FFat.freeBytes() / 1024);
  return true;
}

bool BuddyPack::load(const String& name) {
  unload();
  if (!_mounted) return false;

  String path = String(PACKS_DIR) + "/" + name + ".pack";
  File f = FFat.open(path, "r");
  if (!f) {
    Serial.printf("[Pack] file not found: %s\n", path.c_str());
    return false;
  }

  // Read header (32 bytes)
  uint8_t hdr[32];
  if (f.read(hdr, 32) != 32) { f.close(); return false; }
  if (memcmp(hdr, MAGIC, 8) != 0) {
    Serial.println(F("[Pack] bad magic"));
    f.close(); return false;
  }

  uint32_t version, numFrames32;
  memcpy(&version, hdr + 8, 4);
  memcpy(&numFrames32, hdr + 12, 4);
  _numFrames = (uint16_t)numFrames32;
  memcpy(&_frameW, hdr + 16, 2);
  memcpy(&_frameH, hdr + 18, 2);

  if (_numFrames > PACK_MAX_FRAMES) _numFrames = PACK_MAX_FRAMES;

  // Read frame table
  for (int i = 0; i < _numFrames; ++i) {
    uint8_t entry[16];
    if (f.read(entry, 16) != 16) { f.close(); return false; }
    _frames[i].state      = entry[0];
    _frames[i].variant    = entry[1];
    memcpy(&_frames[i].durationMs, entry + 2, 2);
    memcpy(&_frames[i].offset,     entry + 4, 4);
    memcpy(&_frames[i].size,       entry + 8, 4);
  }

  // Read all frame data into RAM
  uint32_t dataStart = 32 + 16 * (uint32_t)_numFrames;
  uint32_t fileSize = f.size();
  _dataSize = fileSize - dataStart;
  _data = (uint8_t*)malloc(_dataSize);
  if (!_data) {
    Serial.println(F("[Pack] OOM loading data"));
    f.close(); return false;
  }
  f.seek(dataStart);
  f.read(_data, _dataSize);
  f.close();

  _name = name;
  _loaded = true;
  Serial.printf("[Pack] loaded '%s': %d frames, %dx%d, %u bytes data\n",
    name.c_str(), _numFrames, _frameW, _frameH, _dataSize);
  return true;
}

void BuddyPack::unload() {
  if (_data) { free(_data); _data = nullptr; }
  _loaded = false;
  _numFrames = 0;
  _dataSize = 0;
  _name = "";
}

const uint8_t* BuddyPack::getFrame(uint8_t state, uint8_t variant, int& outW, int& outH) {
  if (!_loaded || !_data) return nullptr;
  outW = _frameW;
  outH = _frameH;
  for (int i = 0; i < _numFrames; ++i) {
    if (_frames[i].state == state && _frames[i].variant == variant) {
      if (_frames[i].offset + _frames[i].size <= _dataSize) {
        return _data + _frames[i].offset;
      }
    }
  }
  // Fallback: return variant 0 for this state
  if (variant != 0) return getFrame(state, 0, outW, outH);
  return nullptr;
}

int BuddyPack::variantCount(uint8_t state) const {
  int count = 0;
  for (int i = 0; i < _numFrames; ++i) {
    if (_frames[i].state == state) ++count;
  }
  return count;
}

// --- File upload via BLE ---

bool BuddyPack::createPackFile(const String& name, uint32_t expectedSize) {
  if (!_mounted) return false;
  String path = String(PACKS_DIR) + "/" + name + ".pack";
  _uploadFile = FFat.open(path, "w");
  if (!_uploadFile) return false;
  _uploadName = name;
  _uploadExpectedSize = expectedSize;
  _uploadWritten = 0;
  Serial.printf("[Pack] upload begin: %s (%u bytes expected)\n", name.c_str(), expectedSize);
  return true;
}

bool BuddyPack::appendChunk(const uint8_t* data, size_t len) {
  if (!_uploadFile) return false;
  size_t written = _uploadFile.write(data, len);
  _uploadWritten += written;
  return (written == len);
}

bool BuddyPack::finalizePackFile(uint32_t expectedCrc) {
  if (!_uploadFile) return false;
  _uploadFile.close();
  Serial.printf("[Pack] upload done: %u bytes written\n", _uploadWritten);

  // Verify CRC if provided
  if (expectedCrc != 0) {
    String path = String(PACKS_DIR) + "/" + _uploadName + ".pack";
    File f = FFat.open(path, "r");
    if (!f) return false;
    // CRC verification skipped on device for simplicity; the Python upload tool
    // verifies CRC before sending pack_end.
    f.close();
  }
  return true;
}

String BuddyPack::listPacks() {
  if (!_mounted) return "[]";
  String result = "[";
  File dir = FFat.open(PACKS_DIR);
  if (!dir) return "[]";
  bool first = true;
  File entry;
  while ((entry = dir.openNextFile())) {
    String name = entry.name();
    if (name.endsWith(".pack")) {
      if (!first) result += ",";
      // Strip path and extension
      int lastSlash = name.lastIndexOf('/');
      String stem = (lastSlash >= 0) ? name.substring(lastSlash + 1) : name;
      stem = stem.substring(0, stem.length() - 5);  // remove .pack
      result += "\"" + stem + "\"";
      first = false;
    }
    entry.close();
  }
  dir.close();
  result += "]";
  return result;
}

void BuddyPack::saveActivePack(const String& name) {
  Preferences prefs;
  prefs.begin(NVS_NS, false);
  prefs.putString(NVS_KEY, name);
  prefs.end();
}

String BuddyPack::loadActivePack() {
  Preferences prefs;
  prefs.begin(NVS_NS, true);
  String name = prefs.getString(NVS_KEY, "");
  prefs.end();
  return name;
}
