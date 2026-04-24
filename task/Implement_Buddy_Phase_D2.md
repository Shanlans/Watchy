# Task: Buddy Phase D.2 — Flicker Fix + Event Animations

## Metadata
- Task name: Implement_Buddy_Phase_D2
- Owner: Shanlan
- Date: 2026-04-24
- Depends on: task/Achieve/Implement_Buddy_Phase_D1.md (archived, buddy character working)
- Acceptance profile: A2_Algorithm_Library_Change
- Acceptance reference: master_spec/acceptance_spec/A2_Algorithm_Library_Change.acceptance.md
- Task review approval: approved by user 2026-04-24
- Current Phase: Archived
- Blocking Condition: none
- Acceptance State: passed
- Last Build Exit Code: 0
- Archive Ready: yes

### Runtime State
- Acceptance State: not_run
- Last Build Exit Code: n/a
- Last Verified At: 2026-04-24
- Owning Chat ID: grandfathered

## Background And Goal
Phase D.1 put a static buddy face on screen but it redraws on every 10-sec heartbeat using `setFullWindow()`, causing a visible black/white flash each cycle. User feedback: "手表一直刷屏闪".

Two goals for D.2:
1. **Fix the flicker** — switch default render path to partial refresh, skip render if nothing visible changed, schedule periodic full refresh only to clean ghosting.
2. **Add event-triggered animations** — blink (idle), celebrate (token milestone), heart (fast permission approval). Event-triggered to stay battery-reasonable; no continuous animation.

## Scope (In)
- Refactor `BuddyUI::render()` to use `setPartialWindow(0, 0, 200, 200)` by default; `setFullWindow()` only when:
  - First render after boot
  - Every 5 min (ghosting cleanup)
  - Mood transition to/from ATTENTION (permission prompt — needs clean contrast)
- Skip render when rendered content would be identical to last frame (compute a cheap content hash: mood + connected + running + waiting + total + tokensToday + msg + prompt.id + entry 0).
- Add IDLE blink: when mood is IDLE, render eyes-closed variant briefly (~250 ms) every 4-5 seconds. New mood `MOOD_IDLE_BLINK` in `BuddyCharacter`.
- Add CELEBRATE event: edge-triggered when `tokensToday` crosses 1K / 5K / 10K / 50K / 100K boundaries since boot. Show `MOOD_CELEBRATE` (starry eyes + open smile + spark lines around head) for 3 sec.
- Add HEART event: edge-triggered when user taps Menu to approve a prompt within 5 sec of `prompt.active` becoming true. Show `MOOD_HEART` (heart-shaped eyes + smile) for 2 sec.
- Short vibrate pulse on celebrate/heart events (visible status if watch has motor) — skipped if no vibration motor API readily available in this sketch context.

## Scope (Out — deferred)
- DIZZY mood via BMA423 shake detection (needs separate accelerometer driver integration; will treat as its own task if wanted).
- GIF-pack bitmap character upload (Phase D.3).
- Sleep / battery optimization (Phase C).

## Data flow
```
heartbeat arrives
  -> BuddyState.handleLine() updates fields, sets dirty=true
     -> also: check tokensToday crossed milestone -> notifyCelebrate()
     -> also: if prompt.active became true, record prompt.startMs
button Menu pressed during active prompt
  -> if millis() - prompt.startMs < 5000 -> notifyHeart()
  -> send permission:once, clear prompt.active, sets dirty=true

loop() at 20 Hz
  -> tick(): advance blink state, expire celebrate/heart overrides
  -> if dirty OR blink-frame-changed OR override-expired:
     -> compute effective mood (override > derived)
     -> hash(content) if hash == last_hash: skip render
     -> else render with partial window (unless full-refresh-due)
```

## UI changes
No layout change from D.1 — same top bar, center character, info strip, bottom strip. Only mood rendering and refresh strategy change.

## Pass/Fail Criteria
| # | Category | Criterion | Pass Threshold |
|---|---|---|---|
| 1 | Build | arduino-cli compile exit code | = 0 |
| 2 | Flicker | 10-sec heartbeat cycle does NOT produce visible full-screen flash | user confirms no flashing |
| 3 | Full refresh cadence | Full refresh happens at startup + ≤ once per 5 min + on prompt edge | observe timing |
| 4 | Blink | In IDLE mood, eyes blink once every 4-5 sec | user observes blinking |
| 5 | Celebrate | When Claude tokens cross 1K boundary, buddy shows celebrate for ~3 sec | observable |
| 6 | Heart | When user presses Menu within 5 sec of permission prompt, buddy shows heart for 2 sec | observable |
| 7 | No regression | OFFLINE / BUSY / ATTENTION / SLEEP still behave as D.1 | visual |

## Plan
1. Add `MOOD_CELEBRATE`, `MOOD_HEART`, `MOOD_IDLE_BLINK` to `BuddyCharacter` enum + draw paths.
2. Add milestone detection in `BuddyState::handleLine` (tokens crossing list: 1000/5000/10000/50000/100000).
3. Add `prompt.startMs` timestamp; `BuddyState::noteApproval(now)` checks delta for HEART trigger.
4. In `BuddyUI`, split `render()` into state computation (mood override selection, content hash) + draw (partial vs full).
5. Add blink scheduler: idle-mode blink every 4 s for 250 ms.
6. Build, flash, observe flicker and event triggers.

## Risks
| Risk | Mitigation |
|---|---|
| Partial refresh accumulates ghosting over time | Full refresh every 5 min already mitigates; ATTENTION edge also full-refreshes |
| Content hash misses a change and we skip needed render | Include all visible fields in hash; start conservative — when in doubt, render |
| Celebrate milestone fires repeatedly on same bucket | Track last-milestone-reached; only fire on upward crossing |
| Heart event fires on slow approvals | Strict 5 sec window; only triggered when `prompt.active` → `!prompt.active` via button |

## Estimated effort
2-3 hours.

## Execution Update (2026-04-24)

### Iteration 1 — implemented
- Added `MOOD_CELEBRATE`, `MOOD_HEART`, `MOOD_IDLE_BLINK` draw paths in `BuddyCharacter.cpp`.
- Added `BuddyEvents` struct + milestone crossing detection + `noteApproval()` fast-approve detection in `BuddyState`.
- Rewrote `BuddyUI` with:
  - `setPartialWindow(0,0,200,200)` by default
  - `setFullWindow()` only when: first render, 5-min ghosting cleanup, `prompt.active` toggled (either direction)
  - 32-bit content hash — skip render if nothing visible changed
  - `tick()` consumes events → sets `overrideMood` with expiry (3s celebrate / 2s heart)
  - Blink scheduler — 250 ms closed eyes every 4.5 sec when in IDLE
- Build: 616 KB (+6 KB vs D.1). User flashed and observed behavior.

### User feedback post-flash
- Heartbeat cycle no longer flickers — FIX VERIFIED.
- Permission flow (prompt appear → user approve) still produces TWO visible flashes per interaction.
- User comment: "accept 后还是会闪，这个正常吗？"

### Pending proposal (awaiting approval)
**Tweak**: narrow the "prompt transition" full-refresh rule to only fire when `prompt.active` transitions FALSE→TRUE (appearance), not TRUE→FALSE (clearing).
- **Why it's safe**: when prompt clears, removed content was text at bottom of screen; replacing with the single-line entries row is a minor change, partial refresh leaves negligible ghosting.
- **Code change**: 1 line in `BuddyUI::render()` — replace
  ```
  if (st.prompt.active != lastPromptActive) wantFull = true;
  ```
  with
  ```
  if (st.prompt.active && !lastPromptActive) wantFull = true;
  ```
- **Effect**: permission interaction goes from 2 flashes to 1.
- **Risk**: faint ghosting of permission hint line may remain for up to 5 min until the scheduled full refresh. Mitigation: existing 5-min full refresh cadence still cleans it.

Status: **APPROVED_AND_APPLIED** (2026-04-24)

### Iteration 2 — implemented
- 1-line change in `BuddyUI::render()`: full refresh fires only on prompt appearance (`st.prompt.active && !lastPromptActive`), not on clear.
- Build: 616 KB (unchanged).
- User verified: post-accept no longer flashes. 5-min periodic full refresh retained as ghosting mitigation.

## Final Conclusion
- Outcome: accepted
- Decision: continue (D.3 optional — GIF pack upload)
- Key Evidence: heartbeat cycle no longer flashes (iter 1); permission accept no longer flashes (iter 2); blink/celebrate/heart moods implemented.
- Next Action: user-initiated Phase D.3 or extended usage.
