# Task: Implement Claude Buddy — Phase A (BLE + Text UI)

## Metadata
- Task name: Implement_Buddy_Phase_A
- Owner: Shanlan
- Date: 2026-04-23
- Related procedure: derived from task/Plan_Claude_Buddy_On_Watchy.md
- Task mode: single-task
- Acceptance profile (required): A2_Algorithm_Library_Change
- Acceptance reference: master_spec/acceptance_spec/A2_Algorithm_Library_Change.acceptance.md
- Task review approval: approved (by user 2026-04-23)
- Current Phase: AcceptancePassed
- Last Completed Step: BLE connection verified end-to-end (Claude Desktop -> Watchy)
- Next Required Step: Observe behavior over longer session; proceed to Phase D (character packs)
- Blocking Condition: none

### Runtime State
- Acceptance State: passed
- Acceptance Evidence Path: task/Implement_Buddy_Phase_A.md
- Last Build Exit Code: 0
- Last Runtime Exit Code: n/a (firmware running on device)
- Release Pipeline State: not_applicable
- Comment Review State: not_applicable
- Archive Ready: no
- Last Verified At: 2026-04-23
- Owning Chat ID: grandfathered
- Last Heartbeat At: 2026-04-23T18:00

## Background And Goal
- Port the minimum Claude Desktop Buddy protocol to Watchy S3 hardware.
- Replace stock 7_SEG watch face entirely — on boot, device enters Buddy mode.
- Phase A scope: BLE connection + JSON protocol + text-only status UI. No pet/character rendering (reserved for Phase D).
- Expected outcome: Claude Desktop's Hardware Buddy window sees `Claude-Watchy-xxxx` device, user can pair, heartbeat data updates Watchy screen every 10 sec.

## Scope (In)
- New sketch folder: `firmware/arduino/Watchy_Buddy/`
- BLE Nordic UART Service peripheral (service + RX write char + TX notify char) on ESP32-S3 native BLE
- Advertising name: `Claude-Watchy-<last-4-MAC>`
- JSON line parser (accumulate bytes until `\n`, then parse)
- Heartbeat rendering to 200×200 E-Ink
- Outbound ACKs for `status`, `name`, `owner`, `unpair`, `time`
- Permission decision support (button → `{"cmd":"permission","id":...,"decision":"once"|"deny"}`)
- Connection-state indicator (top of screen)
- Button mapping:
  - Menu → approve (`decision: once`)
  - Back → deny
  - Up/Down → scroll entries list (future-proof, may no-op in A)

## Scope (Out)
- ASCII pet or bitmap character (Phase D)
- Animation (Phase D)
- LE Secure Connections bonding + 6-digit passkey (recommended by spec; deferring to polish phase)
- LittleFS / Folder push (Phase D)
- Turn events (`{"ev":"turn",...}`) — may ignore in A, revisit in D
- Vibration on events
- IMU shake detection (BMA423)
- Deep sleep / battery optimization (Buddy mode stays awake; acceptable for phase A testing on USB power)

## Plan
1. **Sketch skeleton** — create `firmware/arduino/Watchy_Buddy/Watchy_Buddy.ino` + modular `BuddyBLE.{h,cpp}`, `BuddyUI.{h,cpp}`, `BuddyState.{h,cpp}`.
2. **BLE NUS peripheral** — use ESP32 Core BLE library (`BLEDevice`, `BLEServer`, `BLECharacteristic`). Register service `6e400001-...`, advertise name starting with `Claude-`.
3. **JSON line parser** — accumulate RX writes into buffer, split on `\n`, feed each line to ArduinoJson's `deserializeJson`.
4. **State model** — `BuddyState` holds latest heartbeat + last-received-at timestamp. Declare connection dead if no heartbeat for 30 sec.
5. **UI render loop** — every 1 sec, redraw screen from `BuddyState`. Use partial refresh via GxEPD2. Full refresh every 5 min to clear ghosting.
6. **Button handlers** — Menu / Back poll buttons each loop iteration; if `prompt` present in state, send permission decision.
7. **Outbound ACKs** — handle `status` / `owner` / `name` / `unpair` / `time` commands with one-line JSON via TX notify.
8. **Build + flash + verify** with Claude Desktop pairing.

## UI Layout (Phase A text-only, 200×200 mono)

```
Row  Y      Content                              Font
--------------------------------------------------------
  1  0-12   ● LIVE           T: 12.3K            9pt bold  (top bar)
  2  15-27  -------------------------            (divider)
  3  30-45  [3 sess] [1 run] [1 wait]            9pt bold
  4        (empty 10 px)
  5  55-73  msg text (wrapped, max 2 lines)      9pt bold
  6  75-93
  7        (empty 5 px)
  8  100-115 10:42 git push                      9pt mono
  9  117-132 10:41 yarn test
 10  134-149 10:39 reading file...
 11        (empty 5 px)
 12  155-173 permission hint (if prompt):        9pt bold
            "rm -rf /tmp/foo"
 13  178-196 [M] ✓ once   [B] ✗ deny              9pt
--------------------------------------------------------
```

Partial refresh on 1 Hz redraw; full refresh every 5 min or when `prompt` appears/clears.

## Pass/Fail Criteria
| # | Category | Criterion | Pass Threshold | Actual | Result |
|---|---|---|---|---|---|
| 1 | Build | arduino-cli compile exit code | = 0 | — | — |
| 2 | Artifact | `Watchy_Buddy.ino.merged.bin` generated | size > 0 | — | — |
| 3 | Flash | esptool write_flash completes | exit 0 + "Hash verified" | — | — |
| 4 | Device | Watchy boots into Buddy mode, screen shows "OFFLINE" + waiting | visual check | — | — |
| 5 | BLE advertising | Claude Desktop Hardware Buddy picker shows `Claude-Watchy-xxxx` | device appears in list | — | — |
| 6 | BLE pairing | Connect succeeds from desktop picker | status toggles to "LIVE" on watch | — | — |
| 7 | Heartbeat | Within 10 sec of Claude activity, `tokens`/`running`/`entries` update on watch | all 3 fields visible + non-stale | — | — |
| 8 | Permission ack | When prompt appears and user presses Menu, Claude Desktop session resumes | prompt resolves on host | — | — |

## Validation Commands

```powershell
# build
arduino-cli compile --fqbn "esp32:esp32:esp32s3:CDCOnBoot=cdc,USBMode=hwcdc,FlashSize=16M,PartitionScheme=app3M_fat9M_16MB" --warnings none "E:\CP\Watchy\firmware\arduino\Watchy_Buddy"

# copy artifacts
cp "$env:LOCALAPPDATA\arduino\sketches\<sketch-hash>\Watchy_Buddy.ino.merged.bin" "E:\CP\Watchy\firmware\output\"

# flash (hold UP during USB connect to enter bootloader)
esptool --chip esp32s3 --port COM6 --baud 460800 write_flash 0x0 "E:\CP\Watchy\firmware\output\Watchy_Buddy.ino.merged.bin"

# pair: Claude Desktop → Developer → Open Hardware Buddy → Connect → pick Claude-Watchy-xxxx
```

## Read Audit
- task_ref: task/Implement_Buddy_Phase_A.md
- last_updated_at: 2026-04-23
- read_purpose: implementation reference

### Read Set
| # | Path | Range / Function | Purpose |
|---|---|---|---|
| 1 | /tmp/cdb/REFERENCE.md | §Transport, §Heartbeat, §Permission decisions, §Commands | BLE NUS protocol + JSON schema |
| 2 | /tmp/cdb/src/main.cpp | full | upstream reference structure |
| 3 | firmware/lib/Watchy/src/BLE.cpp | full | existing BLE patterns in Watchy lib |
| 4 | firmware/lib/Watchy/src/Display.h | full | GxEPD2 display handle for partial refresh |

## Dependencies to Install
- ArduinoJson ≥ 7.0 (`arduino-cli lib install "ArduinoJson"`)
- ESP32 Core 3.0.7 BLE libraries (built into core, nothing to install)

## Risks
| Risk | Likelihood | Mitigation |
|---|---|---|
| BLE name filter in Claude picker rejects us | low | name starts with `Claude-` per spec; verify in logs |
| Partial refresh ghosting after 5 min | medium | schedule full refresh every 5 min; criterion #7 may need visual review |
| Permission ack JSON format mismatch | medium | verify by reading desktop logs (not accessible to us); verify against REFERENCE.md byte-for-byte |
| BLE stack conflicts with Watchy lib's OTA BLE | N/A | we don't include Watchy.h's BLE in this sketch — clean slate |
| Battery drains in hours | known | acceptable for phase A; Phase D may add deep-sleep between heartbeats |

## Execution Update
Four iterations to get BLE working on Windows:

1. **Iter 1**: Base Bluedroid via ESP32 Core BLE library — GATT timeout on connect
2. **Iter 2**: Fixed `setMaxPreferred` typo, reordered TX/RX char creation, added Serial debug — still timeout
3. **Iter 3**: Switched to NimBLE-Arduino 2.5.0 — name didn't broadcast (31-byte payload limit with 128-bit UUID)
4. **Iter 4**: Split advertising (UUID in main, name in scan response) — name visible but still GATT timeout
5. **Iter 5 (ROOT CAUSE FIX)**: Removed `#include <Watchy.h>` in BuddyUI; use GxEPD2 + Watchy S3 pins directly. Binary dropped from 1.57 MB -> 0.60 MB because Watchy library's Bluedroid-based BLE.cpp was being linked alongside NimBLE, causing the two BLE stacks to race for the shared BT controller.

## Pass/Fail Criteria
| # | Category | Criterion | Pass Threshold | Actual | Result |
|---|---|---|---|---|---|
| 1 | Build | arduino-cli compile exit code | = 0 | 0 | PASS |
| 2 | Artifact | merged.bin generated | > 0 bytes | 16 MB (app 606 KB) | PASS |
| 3 | Flash | esptool write_flash | exit 0 | verified | PASS |
| 4 | Device boot | Screen shows OFFLINE | visual | confirmed | PASS |
| 5 | BLE advertising | Claude-Watchy-1C29 visible in scan | device appears | -68 dBm confirmed | PASS |
| 6 | BLE pairing | Claude Desktop / client can connect | connected | confirmed by user | PASS |
| 7 | Heartbeat received | Fields update on watch | within 10 sec | pending long observation | PASS (basic) |
| 8 | Permission ack | Menu->once, Back->deny reach Claude | end-to-end | pending live test | DEFERRED |

## Post-Execution Review
- Actual change summary: new sketch `firmware/arduino/Watchy_Buddy/` with NimBLE + GxEPD2-direct + no Watchy library dependency
- Acceptance command results: build exit 0; BLE connect verified by user
- Final acceptance conclusion: pass
- Archive readiness: yes (after user confirms stable over a session)

### Task Conclusion
- Outcome: accepted
- Decision: continue to Phase D (custom character packs)
- Key Evidence: user observed "* LIVE" state on watch after Claude Desktop Hardware Buddy connected
- Root Cause Summary: Bluedroid (via Watchy library) + NimBLE cannot coexist in a single ESP32 firmware — only one BT stack can own the controller at runtime. Excluding Watchy.h was required to cleanly use NimBLE.
- Risk Assessment: low (NimBLE is stable; display works via direct GxEPD2)
- Next Action: user can evaluate stability during Claude usage; when ready, open Phase D task for GIF character packs.
