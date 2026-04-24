# Task: Buddy Phase D.3 — Bitmap Character Pack (bufo)

## Metadata
- Task name: Implement_Buddy_Phase_D3
- Owner: Shanlan
- Date: 2026-04-24
- Depends on: task/Achieve/Implement_Buddy_Phase_D2.md (merged, flicker fix + animations verified)
- Acceptance profile: A2_Algorithm_Library_Change
- Task review approval: pending
- Current Phase: PlanCreated
- Blocking Condition: awaiting user approval

## Background And Goal
Upstream ships one official character pack — **bufo** (a frog, 15 GIFs, 96×100 dims, ~570 KB total). User wants to load bufo onto Watchy and have the buddy render as bufo instead of the current GFX-primitive face.

Storage decision (already approved): **flash via FFat** (`app3M_fat9M_16MB` partition, 9.9 MB available). No SD card in Phase D.3.

## Design choice: bypass Claude Desktop folder-push protocol

The REFERENCE.md §Folder push protocol sends raw files as-dropped. If we accept `.gif` files directly, Watchy would need to decode GIFs at runtime (heavy on ESP32-S3 + mono e-ink requires extra dithering stage per frame).

Instead, **preprocess on PC, upload pre-rendered bitmaps via our own BLE command set**. This is:
- Much simpler to implement on-device (no GIF decoder, no runtime dithering)
- Much smaller per-pack (1-bit bitmaps are far smaller than GIFs)
- Compatible with arbitrary sources (any GIF/PNG folder → pack file)

Claude Desktop's folder-push support can be added later (Phase D.4+) if wanted.

## Scope (In)

### Python tool `tools/buddy_pack.py`
- `pack` subcommand
  - Input: directory with `manifest.json` (upstream format) + state GIFs
  - For each GIF, extract key frame(s), dither to 1-bit via Floyd-Steinberg
  - Resize to 96×96 (center-fit, preserve aspect)
  - Write compact `.pack` binary (see format below)
- `upload` subcommand
  - Connect to `Claude-Watchy-*` via bleak
  - Send pack in chunks via NUS RX char using new JSON protocol (below)
  - Verify with CRC32
- `list` subcommand
  - Query installed packs on device

### On-device additions
- Mount FFat at boot
- New `BuddyPack.{h,cpp}`
  - `bool load(const String& name)` — open `/packs/<name>.pack`, parse header + frame table into RAM
  - `const uint8_t* frameBitmap(BuddyMood mood, uint32_t nowMs, int& outW, int& outH)` — return active frame bitmap for the mood (handles idle variant rotation)
- Extend `BuddyBLE` JSON command handling:
  - `{"cmd":"pack_begin","name":"bufo","size":N,"crc":X}` → create `/packs/bufo.pack`, truncate, ack with chunk size
  - `{"cmd":"pack_chunk","seq":i,"data":"<hex>"}` → append bytes (hex-encoded to avoid JSON escaping issues)
  - `{"cmd":"pack_end"}` → close file, verify CRC, activate pack
  - `{"cmd":"pack_list"}` → ack with array of installed pack names
  - `{"cmd":"pack_select","name":"bufo"}` → call `BuddyPack::load`, persist to NVS so it survives reboot
  - `{"cmd":"pack_clear"}` → deactivate, revert to built-in GFX face
- `BuddyCharacter::drawBuddy()` checks loaded pack:
  - Pack loaded → `display.drawBitmap(cx-48, cy-48, bitmap, 96, 96, BLACK)`
  - No pack → current GFX primitives (fallback, zero-pack mode still works)
- NVS key `active_pack` persisted across reboot

### Pack file format (.pack)
```
Header (32 bytes)
  magic       : "BPACK\0\0\0" (8 bytes)
  version     : uint32 LE = 1
  num_frames  : uint32 LE
  frame_w     : uint16 LE = 96
  frame_h     : uint16 LE = 96
  reserved    : 12 bytes (zero)

Frame table (16 bytes × num_frames)
  state       : uint8 (enum matching BuddyMood: 0=OFFLINE 1=IDLE 2=BUSY 3=ATTENTION 4=SLEEP 5=CELEBRATE 6=HEART)
  variant     : uint8 (for idle 0..N-1, others 0)
  duration_ms : uint16 LE (rotation period; 0 = static)
  offset      : uint32 LE (offset into data section, 0-based)
  size        : uint32 LE (bytes in data)
  reserved    : uint16 LE

Data section
  Raw 1-bit bitmap bytes (MSB-first per byte, left-to-right, top-to-bottom)
  Each 96×96 frame = 1152 bytes
```

Pack size estimate: 15 frames × 1152 bytes = 17 KB + 272 bytes header/table. Negligible on 9.9 MB partition.

### Animation playback
- Pack's idle has 9 variants → rotate through them every ~800 ms while staying in IDLE mood. Preempt on mood change.
- Non-idle states: use variant 0 if multiple exist (Phase D.3 keeps celebrate/attention/etc. as static single frame).

## Scope (Out — Phase D.4+)
- Multi-frame animation playback for non-idle states (only idle rotates; celebrate stays single frame).
- Claude Desktop folder-push protocol compatibility (user still drags bufo folder in desktop → desktop sends GIFs). For D.3 user runs our Python tool manually.
- Runtime GIF decoding on device.
- Pack browser UI on watch (Up/Down to switch active pack).

## Pass/Fail Criteria
| # | Criterion | Threshold |
|---|---|---|
| 1 | Python tool packs bufo directory successfully | `.pack` file generated, CRC reproducible |
| 2 | Preview: each state frame renders on PC (sanity check) | visual in `buddy_pack.py preview` (optional flag) |
| 3 | Watchy compile | exit 0 |
| 4 | `upload` completes end-to-end over BLE | reports OK, watch ACKs all chunks |
| 5 | Pack activation | Buddy on watch changes from GFX face to bufo frog |
| 6 | IDLE state rotates through ≥2 variants | visible change over 2-3 seconds |
| 7 | Mood transitions still trigger correct state (BUSY frog, ATTENTION frog, etc.) | observable with Claude activity |
| 8 | Pack persists across reboot | bufo still active after power cycle |

## Plan
1. Write `tools/buddy_pack.py pack` — convert bufo → bufo.pack (2 hrs)
2. Add FFat mount + BLE pack_begin/chunk/end handlers in sketch (2 hrs)
3. Add `tools/buddy_pack.py upload` — BLE chunked upload (1 hr)
4. Add `BuddyPack.{h,cpp}` + integrate with `BuddyCharacter::drawBuddy` (1-2 hrs)
5. Test upload + render + persistence (1 hr)

## Risks
| Risk | Mitigation |
|---|---|
| 1-bit dithering of bufo GIFs looks bad on mono e-ink | Tune threshold / try multiple dithering algs in Python tool; can ship the user best-effort result |
| BLE upload too slow (~2-5 KB/sec typical, ~17 KB pack = 4-9 sec) | acceptable, document timing |
| FFat file writes exhaust 4KB wear-level sectors | infrequent writes (user-initiated pack upload); well within flash lifetime |
| CRC mismatch on upload | retry loop in tool; report and bail |
| Frame rotation flicker | use partial refresh; already handled by Phase D.2 infrastructure |

## Dependencies
- Python: `pillow` (image processing), `bleak` (already have)

## Estimated effort
6-8 hours.
