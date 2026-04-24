# Task: Buddy Phase D.1 — Character Display (hardcoded)

## Metadata
- Task name: Implement_Buddy_Phase_D1
- Owner: Shanlan
- Date: 2026-04-24
- Depends on: task/Implement_Buddy_Phase_A.md (merged)
- Acceptance profile: A2_Algorithm_Library_Change
- Current Phase: Archived
- Blocking Condition: none
- Last Build Exit Code: 0
- Acceptance State: passed
- Archive Ready: yes

## Post-Execution Review
- Changes: added `BuddyCharacter.{h,cpp}` with mood state machine + GFX-primitive face, rewrote `BuddyUI::render()` with top bar + 96px center character + info/prompt strips
- Build: 610 KB (+4 KB vs Phase A text-only)
- Device test: user confirmed buddy visible, moods transition on Claude activity

### Task Conclusion
- Outcome: accepted
- Decision: continue (D.2 or D.3)
- Key Evidence: user observation "可以。没问题。" 2026-04-24
- Next Action: D.2 (event-triggered micro-animations) or D.3 (GIF pack upload) per user preference

## Goal
Replace the text-only UI with a center-screen buddy character whose face changes based on Claude Desktop state. Use GxEPD2 drawing primitives (no bitmaps yet) so this is fast, small, and lets us validate the state machine before investing in a full GIF pack upload pipeline (Phase D.3).

## Scope
- New `BuddyCharacter.{h,cpp}` with:
  - `BuddyMood` enum: `OFFLINE`, `IDLE`, `BUSY`, `ATTENTION`, `SLEEP`
  - `deriveMood(BuddyState&)` → picks current mood from heartbeat data
  - `drawBuddy(display, mood, cx, cy)` → draws centered character face using GFX primitives
- Rewrite `BuddyUI::render()` layout:
  - Top bar (0-17 px): connection + tokens_today
  - Character center (20-140 px): 96×96 buddy face
  - Info strip (145-175 px): msg + most recent entry
  - Prompt strip (180-199 px): permission hints when active

## Out of Scope (Phase D.2 / D.3)
- Animation (e.g., blink, celebrate bounce) — handled in Phase D.2
- Custom character packs / GIF upload — Phase D.3
- IMU shake detection (dizzy mood) — Phase D.2
- Edge-triggered moods (celebrate, heart) — Phase D.2

## Mood derivation rules

| Condition | Mood |
|---|---|
| `!connected` OR stale | OFFLINE (sad face) |
| `prompt.active` | ATTENTION (wide eyes, "!") |
| `running > 0` | BUSY (sweat drop) |
| `total == 0` AND idle > 5 min | SLEEP (closed eyes, "Z") |
| otherwise | IDLE (smile) |

## Pass/Fail
| # | Criterion | Threshold |
|---|---|---|
| 1 | Build exit code | 0 |
| 2 | Device boots, draws centered face | visual |
| 3 | Face changes when Claude activity happens (BUSY↔IDLE) | observable within 10s |
| 4 | Permission prompt → face shows ATTENTION | visible on next heartbeat |

## Plan
1. Create `BuddyCharacter.h` / `.cpp`
2. Refactor `BuddyUI::render()` for new layout
3. Build + flash + observe

## Risks
- Partial refresh + GFX drawing primitives should be efficient; not a refresh-rate concern.
- Face might look bad on e-ink — iterate visually.
