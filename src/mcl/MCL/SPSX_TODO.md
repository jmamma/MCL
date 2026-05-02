# SPSXSeqTrack Integration TODO

## What's Done
- `SPSXSeqDefines.h` — constants (INTERP=16, 96 ticks/step, 34 locks, 52 conditionals)
- `SPSXSeqTrackData.h` — track data struct (64-bit lock masks, signed microtiming)
- `SPSXSeqTrack.h` — class hierarchy (Base → Cond → SlideTrack → SPSXSeqTrack)
- `SPSXSeqTrack.cpp` — full sequencer (~850 lines): seq(), triggers, plocks, MID notes, slides, recording, editing, merge_from_md()
- `MDPattern.h/.cpp` — SPSX v0x40 sysex fromSysex/toSysex with RLE
- `ElektronDataEncoder.h/.cpp` — RLE encode/decode for sysex

All files are `#if !defined(__AVR__)` guarded. Headers syntax-check clean.

---

## DONE 1: MCLSeq Hot Swap ✓

- `MCLSeq.h`: Added `spsx_tracks[16]`, `using_spsx_tracks`, `neighbor_trig_mask`, `fill_mask`, `switch_to_spsx()`, `switch_to_legacy()`
- `MCLSeq.cpp`: seq() dispatches to SPSX or legacy tracks. Start/stop/continue callbacks reset the correct track set.
- SPSX tracks share `MDSeqTrack::md_trig_mask` and `MDSeqTrack::gui_update` statics (removed file-local duplicates)
- CC/note callbacks route to spsx_tracks when `using_spsx_tracks` is true

### Where to trigger the switch
- In `MDSysex` callback when MD.is_spsx is detected (or on firmware connect)
- Or in GridTask when loading a pattern set

---

## DONE 2: Timer/ISR — 384 PPQN for SPSX ✓

- `MidiClock.h`: Added `clock_interpolation` field (volatile uint8_t, default=2)
- `incrementCounters()`: `div192_time = diff_clock8 / (8 * clock_interpolation)` on rp2040
- `switch_to_spsx()` sets clock_interpolation=16, `switch_to_legacy()` sets it back to 2
- Must switch only when transport is stopped (enforced by check in switch functions)

---

## DONE 3: Stubs Completed ✓

### neighbor_fired() — implemented
- `record_trig_result()` sets/clears bits in `seq_class->neighbor_trig_mask`
- `neighbor_fired()` checks bit for `track_number - 1`

### FILL/NOT_FILL — implemented
- Checks `seq_class->fill_mask` bit for current track
- UI code needs to set `mcl_seq.fill_mask` bits when fill button is held

### send_trig_inline() — implemented
- Sets bit in `MDSeqTrack::md_trig_mask` (shared with legacy)
- Calls `mixer_page.trig(track_number)` for LED feedback

### preview_step() — implemented
- Sends all lock values for the step via `MD.setTrackParam()`
- Handles MID machines (note off, init notes, note on with length)
- Triggers track via `MD.triggerTrack()` + mixer_page LED

---

## DONE 4: Build Integration ✓

PlatformIO `build_src_filter = +<mcl/*>` already picks up SPSXSeqTrack.cpp.
The `#if !defined(__AVR__)` guard handles platform filtering. No build changes needed.

---

## DONE 5: merge_from_md Refinement ✓

- Added pattern length as default before SPSX overrides
- V3 swing amount (Q14 format) → SPSX microtiming conversion for swung steps
- SPSX v0x40 extension data overrides V3 defaults (microtiming, step flags, lock params, per-track length/speed)
- Lock row mapping preserved from original (iterates paramLocks correctly)
