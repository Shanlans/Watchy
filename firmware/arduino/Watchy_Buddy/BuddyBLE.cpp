#include "BuddyBLE.h"
#include <NimBLEDevice.h>
#include "esp_mac.h"

static BuddyState* g_state = nullptr;
static NimBLECharacteristic* g_txChar = nullptr;
static NimBLECharacteristic* g_rxChar = nullptr;
static NimBLEServer* g_server = nullptr;
static String g_rxBuffer;

class ServerCallbacks : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer* s, NimBLEConnInfo& info) override {
    Serial.println(F("[BLE] onConnect"));
    if (g_state) {
      g_state->connected = true;
      g_state->dirty = true;
    }
  }
  void onDisconnect(NimBLEServer* s, NimBLEConnInfo& info, int reason) override {
    Serial.printf("[BLE] onDisconnect (reason=%d) - restarting advertising\n", reason);
    if (g_state) {
      g_state->connected = false;
      g_state->prompt.active = false;
      g_state->dirty = true;
      g_state->lastHeartbeatMs = 0;
    }
    // NimBLE auto-restarts advertising by default, but be explicit.
    NimBLEDevice::startAdvertising();
  }
  void onMTUChange(uint16_t MTU, NimBLEConnInfo& info) override {
    Serial.printf("[BLE] MTU=%u\n", MTU);
  }
};

class RxCallbacks : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic* c, NimBLEConnInfo& info) override {
    if (!g_state) return;
    NimBLEAttValue v = c->getValue();
    g_rxBuffer += String((const char*)v.data(), v.length());

    int nl;
    while ((nl = g_rxBuffer.indexOf('\n')) >= 0) {
      String line = g_rxBuffer.substring(0, nl);
      g_rxBuffer.remove(0, nl + 1);
      if (line.length() == 0) continue;
      String reply = g_state->handleLine(line);
      if (reply.length() > 0 && g_txChar) {
        g_txChar->setValue((uint8_t*)reply.c_str(), reply.length());
        g_txChar->notify();
      }
    }
    if (g_rxBuffer.length() > 4096) g_rxBuffer = "";
  }
};

void BuddyBLE::begin(BuddyState* state) {
  g_state = state;

  // Build display name "Claude-Watchy-XXXX" using last 2 bytes of BT MAC
  uint8_t mac[6] = {0};
  esp_read_mac(mac, ESP_MAC_BT);
  char buf[24];
  snprintf(buf, sizeof(buf), "Claude-Watchy-%02X%02X", mac[4], mac[5]);
  deviceName = String(buf);
  state->deviceName = deviceName;

  NimBLEDevice::init(deviceName.c_str());
  NimBLEDevice::setPower(ESP_PWR_LVL_P9);       // max TX power
  NimBLEDevice::setMTU(517);

  g_server = NimBLEDevice::createServer();
  g_server->setCallbacks(new ServerCallbacks());

  NimBLEService* svc = g_server->createService(NUS_SERVICE_UUID);

  // TX char: server notifies client (desktop -> device direction isn't this one)
  g_txChar = svc->createCharacteristic(
      NUS_TX_CHAR_UUID,
      NIMBLE_PROPERTY::NOTIFY
  );

  // RX char: client writes to server
  g_rxChar = svc->createCharacteristic(
      NUS_RX_CHAR_UUID,
      NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR
  );
  g_rxChar->setCallbacks(new RxCallbacks());

  svc->start();

  // Split advertising payload: 128-bit UUID fills most of the 31-byte main
  // advertisement, so put the name in the scan response. Both are visible to
  // scanners that request scan response (bleak on Windows does by default).
  NimBLEAdvertisementData advData;
  advData.setFlags(BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP);
  advData.addServiceUUID(NimBLEUUID(NUS_SERVICE_UUID));

  NimBLEAdvertisementData scanResp;
  scanResp.setName(deviceName.c_str());

  NimBLEAdvertising* adv = NimBLEDevice::getAdvertising();
  adv->setAdvertisementData(advData);
  adv->setScanResponseData(scanResp);
  NimBLEDevice::startAdvertising();

  Serial.printf("[BLE] advertising as '%s' (NimBLE)\n", deviceName.c_str());
}

void BuddyBLE::tick() {}

void BuddyBLE::sendLine(const String& line) {
  if (!g_txChar || !g_state || !g_state->connected) return;
  g_txChar->setValue((uint8_t*)line.c_str(), line.length());
  g_txChar->notify();
}
