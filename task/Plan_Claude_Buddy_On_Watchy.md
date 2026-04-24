# Plan: Port claude-desktop-buddy to Watchy S3

## Metadata
- Task name: Plan_Claude_Buddy_On_Watchy
- Owner: Shanlan
- Date: 2026-04-23
- Task mode: single-task (planning only; implementation phases broken into future tasks)
- Current Phase: PlanCreated
- Status: awaiting user scope decision

## Source
- Upstream repo: https://github.com/anthropics/claude-desktop-buddy (official Anthropic reference implementation)
- Target hardware: M5StickC Plus (ESP32 + 135×240 color LCD)
- Our hardware: Watchy S3 (ESP32-S3 + 200×200 mono E-Ink)

## What the app does
A Bluetooth companion device for Claude Desktop. Claude Desktop connects via BLE (Nordic UART Service) and pushes:
- Heartbeat every 10 sec (running sessions, waiting permissions, token counts, recent transcript)
- Permission prompts (approve/deny via button)
- Turn events (summaries, completion milestones)
- Character asset uploads (bitmap/ASCII pet packs)

Device shows an animated desk pet whose state reflects Claude's activity: `sleep`/`idle`/`busy`/`attention`/`celebrate`/`dizzy`/`heart`.

## BLE Protocol (unchanged, directly portable)
- **Service UUID**: `6e400001-b5a3-f393-e0a9-e50e24dcca9e` (Nordic UART)
- Device advertises name starting with `Claude`
- Line-delimited UTF-8 JSON, one object per line
- Desktop reassembles fragmented packets; device must do same

Watchy already has `BLE.cpp/.h` (used for OTA), so BLE stack is proven working.

## Hardware porting matrix

| Aspect | Upstream (M5StickC) | Watchy S3 | Port effort |
|---|---|---|---|
| MCU | ESP32 | ESP32-S3 | compatible |
| Display API | `M5.Lcd` / `TFT_eSprite` (color) | `GxEPD2_BW` (mono) | rewrite draw layer |
| Display size | 135×240 landscape | 200×200 square | relayout UI |
| Refresh rate | 60+ fps | partial 2-3 fps, full ~0.7 fps | drop real-time anim |
| GIF | `AnimatedGIF` lib, full-speed | preprocess to 1-bit bitmap frames | offline tooling |
| Buttons | A/B/C | Menu/Back/Up/Down | remap |
| Power mgmt | `M5.Axp` (AXP192) | Watchy own circuit | swap API |
| Sound | `M5.Beep` | **none** | remove |
| IMU | MPU6886 | BMA423 | swap driver (Watchy lib has it) |
| Sleep | M5 LightSleep | Watchy deep sleep | reconcile with BLE |

## Core design challenge: sleep vs. BLE

| Mode | Battery life | Claude connectivity |
|---|---|---|
| Always-on (BLE live) | ~4–6 hours | real-time push works |
| Watchy default deep-sleep every minute | days–week | BLE drops, misses heartbeats |

**→ Must pick one explicitly; they can't coexist.** Resolved via user-toggled Buddy Mode (see Phase C).

## Animation feasibility on E-Ink

| Animation | Feasible? | Notes |
|---|---|---|
| Blink (2 frames, every 3 sec) | ✅ yes | partial refresh, looks natural |
| Breathing sleep (2-3 frames, 1 fps) | ✅ yes | slow but charming |
| Celebration burst (5-10 frames, 2-3 fps, ~3 sec) | ✅ yes | event-triggered, one-shot |
| Smooth walking / 30 fps | ❌ no | E-Ink can't do it |
| GIF at original speed | ❌ no | preprocess to key frames only |

**Principle**: static-by-default, animate only on events.

---

## Phased plan

### Phase A — Minimum Viable Buddy (est. 4–8 hrs)

**Goal**: Watchy acts as a Claude device, receives heartbeats, shows text status.

Deliverables:
- New sketch `firmware/arduino/Watchy_Buddy/` (parallel to `Watchy_7SEG`)
- BLE Nordic UART service advertising as `Claude-Watchy-xxxx`
- Parse heartbeat JSON, display on E-Ink:
  - Line 1: token count
  - Line 2: running/waiting session counts
  - Line 3-5: last 3 entries
  - Line 6 (bottom): current `msg`
- Button mapping:
  - Menu: approve pending permission
  - Back: deny pending permission
  - Up/Down: scroll entries
- **No pet, no animation, no character pack** — just text UI.

Validation: pair Watchy with Claude Desktop, see `entries` update within 10 sec after Claude activity.

### Phase B — ASCII Desk Pet (est. 4–6 hrs)

**Goal**: Add the 18 built-in ASCII pets as an optional display mode.

Deliverables:
- Port `character.cpp` ASCII rendering from M5 `Lcd.drawString` to Watchy `display.print` with monospace font
- Port the 7 state transitions (`sleep`/`idle`/`busy`/`attention`/`celebrate`/`dizzy`/`heart`)
- Menu button cycles: text UI ↔ ASCII pet
- Event-triggered micro-animations only (no continuous loops):
  - `attention`: 3 blinks at 1 fps, then static
  - `celebrate`: 5 frames at 2 fps for 2.5 sec, then static
  - All other states: single static frame

Validation: trigger permission prompt in Claude, watch pet flip to `attention`; approve, watch it flip to `celebrate`.

### Phase C — Dual Mode (est. 3–4 hrs)

**Goal**: Solve the BLE-vs-battery conflict by making Buddy opt-in.

Deliverables:
- Long-press Menu from watch face: enter Buddy Mode (BLE on, no sleep)
- Long-press Back from Buddy Mode: return to watch face (BLE off, resume deep sleep)
- Top status bar in Buddy Mode shows `● LIVE` or `○ IDLE`
- When returning to watch face, show "Buddy connected X minutes" as footer

Validation: day-long battery test — 90% of day in watch mode, 10% in Buddy mode, see battery loss match expectations (~30%).

### Phase D — Custom Character Packs (est. 6–10 hrs, optional)

**Goal**: Support the GIF-based `bufo` style character packs from upstream.

Deliverables:
- Tooling script (Python) to convert GIF character packs to 1-bit `.bmp` arrays suitable for `drawBitmap`
- LittleFS filesystem on Watchy to hold character packs
- "Folder push" protocol support (from REFERENCE.md §Folder push): receive pack upload over BLE
- Pick 1 key frame per state (no GIF playback, just static posters)

Validation: Claude Desktop uploads `bufo` pack via BLE, Watchy renders each state with the bufo toad instead of ASCII.

### Phase E — Polish (est. 2–4 hrs)

- Persist settings to NVS (brightness, last mode, character pack)
- Factory-reset via long-hold Menu+Back
- Stats view (token counts, cumulative uptime)

---

## Known caveats

1. **No sound** — Watchy has no speaker. Permission-approve beeps become vibrate pulses via `vibMotor()`.
2. **No GIF animation** — character packs converted to key frames only.
3. **BLE OTA conflict** — Watchy's existing BLE OTA uses a different service UUID; both can coexist (BLE supports multiple services).
4. **Battery impact** — expect Buddy Mode to drain ~15%/hour vs <1%/hour in normal watch mode.

## Decisions you need to make (pick before we start)

| # | Decision | Options |
|---|---|---|
| 1 | Start from which phase? | **A only (text)**, A+B (text+ASCII pet), or go full A+B+C? |
| 2 | Keep 7_SEG watch face accessible? | Yes (dual mode, phase C required), or No (full-time Buddy) |
| 3 | Character pack priority? | Skip D (ASCII only), or make D required (budget +10 hrs) |
| 4 | Animation aggressiveness? | Event-only (recommended), or 1 fps always-on (battery hit) |
| 5 | Want to share source upstream? | Contribute Watchy port back to anthropics/claude-desktop-buddy as `env:watchy-s3` in their platformio.ini |

## Estimated total effort
- Minimal (A only): **4–8 hours**
- Typical (A+B+C): **11–18 hours**
- Complete (A+B+C+D+E): **20–32 hours**

## Risks
| Risk | Likelihood | Mitigation |
|---|---|---|
| BLE stack conflicts with Watchy OTA | medium | Test coexistence in phase A; fall back to disabling OTA in Buddy mode |
| E-Ink ghosting from frequent partial refresh | medium | Schedule full refresh every 5 min |
| Claude Desktop doesn't see Watchy in device picker | low | Advertising name starts with "Claude" per spec |
| Battery dies faster than "few hours" estimate | medium | Phase C is gating; without it, Buddy is only useful on charger |

## Next action
User picks decisions 1–5, then we open a concrete implementation task (e.g., `task/Implement_Buddy_Phase_A.md`) following the same A2_Algorithm_Library_Change acceptance profile as Build_7SEG_WatchFace_Firmware.
