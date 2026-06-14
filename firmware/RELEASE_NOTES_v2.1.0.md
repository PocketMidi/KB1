# KB1 Firmware v2.1.0 Release Notes

**Release Date:** June 13, 2026  
**Focus:** MIDI reliability improvements + Smart battery charging

---

## Overview

MIDI transmission now has explicit value validation, buffer management, and state tracking synchronization. Fixes root cause of stuck notes during mode changes and state transitions.

Battery charging LEDs now use intelligent duration calculation based on state of charge, and handle computer sleep gracefully during long charges.

---

## New Features

### Smart Charging LED Duration

**Feature:** Charging LEDs now pulse for estimated time to reach 100% based on current battery %.

**How it works:**
- Calculates remaining capacity: `(100 - current%) × 420mAh`
- Estimates charge time: `remaining_capacity ÷ 100mA charge_rate`
- Rounds up to nearest 10 minutes with **30-minute minimum**
- LEDs stop when: estimated time elapsed OR battery reaches 100%

**Examples:**
- Fresh battery (0%) → ~5 hours LED pulse
- Battery at 80% → ~1 hour LED pulse  
- Battery at 95% → 30 minutes LED pulse (enforced minimum)
- Uncalibrated → 5 hours LED pulse (default full charge)

**Manual battery adjustment:** If user manually sets battery % via web app, LED timer resets and recalculates based on new %.

### Computer Sleep Grace Period

**Problem:** When computer goes to sleep during charge, USB data frames stop but 5V power remains. Firmware detected "USB disconnect" and stopped charging LEDs prematurely.

**Fix:** 3-minute grace period at charge start
- First 3 minutes: Actively check USB frame counter for true disconnect
- After 3 minutes: Ignore frame counter, assume computer may sleep
- LEDs pulse until estimated charge time completes

**Edge case:** If computer sleeps then user unplugs USB after 3 minutes, LEDs may pulse briefly until timer expires. Device loses power naturally when actually unplugged.

**Impact:** Users can start long charge (overnight) and close laptop without stopping calibration.

---

## Critical Bug Fixes

### Serial TX Buffer Overflow

**Problem:** ESP32-S3 default 256-byte TX buffer insufficient for MIDI burst transmission. Caused message loss during chords, arpeggios, and rapid playing.

**Fix:** Increased Serial0 TX buffer to 512 bytes before `begin()` call.

**Impact:** Prevents MIDI message loss during burst transmission.

---

### Note Tracking Desynchronization (Critical)

**Problem:** Internal `_isNoteOn[]` tracking array could desynchronize with actual MIDI state on receiving devices. The duplicate note prevention guard in `stopMidiNote()` would then block valid note-off messages, causing stuck notes:

```cpp

**Problem:** Internal `_isNoteOn[]` tracking array could desync with actual MIDI state. Duplicate note prevention guard in `stopMidiNote()` would then block valid note-offs, causing stuck notes.

**Trigger scenarios:**
- Mode changes while keys pressed
- BLE disconnect during performance  
- Sleep transitions mid-note
- Panic sequences that cleared MIDI state but not internal tracking

**Fix:** MIDI panic sequences now call `clearKeyboardNoteTracking()` to sync internal state with MIDI reality.

**Impact:** Eliminates stuck notes from state desync

### MIDI Panic Sequences with TX Drain

Added comprehensive MIDI cleanup at critical state transition points:

**`sendMidiPanic()` helper:**
- Sends CC 123 (All Notes Off) + CC 121 (Reset All Controllers) to all 16 channels
- Waits for TX buffer drain (`waitForMidiTxDrain()`)
- Clears internal note tracking to prevent state desync
- Ensures complete transmission before continuing

**Panic triggers:**
- **Boot sequence** - Cl

MIDI cleanup at state transition points:

**`sendMidiPanic()`:**
- Sends CC 123 (All Notes Off) + CC 121 (Reset All Controllers) to all 16 channels
- Waits for TX buffer drain
- Clears internal note tracking

**Triggers:**
- Boot sequence
- Light/deep sleep entry
- BLE disconnect
- Mode changes (Scale/Chord/Arp)
- Settings changes (chord type, arp settings)

**`waitForMidiTxDrain()`:**
- Waits for Serial0 TX buffer to drain (480/512 bytes available)
- 2-second timeout
- Ensures complete transmission before continuing
```

**Note numbers:**
```cpp
// Chord mode, strum mode, arpeggiator
int chordNote = constrain(rootNote + interval, 0, 127);
_arpCurrentNote = constrain(_arpRootNote + interval, 0, 127);
```

**CC valutransmission points now clamp values to MIDI spec (0-127):
- Velocity in `setVelocity()`
- Note numbers in chord/strum/arp modes
- CC values in lever controls

Prevents invalid MIDI messages from BLE out-of-range values or arithmetic overflow
- Buffer overflow prevention via increased capacity
- State tracking synchronization after state-clearing operations
- Defensive programming against edge cases

### Code Organization

**New global MIDI helpers** (main.cpp):
- `waitForMidiTxDrain()` - Explicit TX buffer drain with timeout
- `sendMidiPanic()` - Comprehensive MIDI cleanup sequence
- `clearKeyboardNoteTracking()` - Tracking synchronization wrapper

**New KeyboardControl method:**
- `clearNoteTracking()` - Clears `_isNoteOn[]` array without sending MIDI

---

## Memory Usage

No significant memory impact:
- **RAM:** 18.3% (59,956 / 327,680 bytes) - unchanged
- **Flash:** 12.5% (1,033,901 / 8,257,536 bytes) - +~200 bytes for new helpers
**MIDI architecture:**
- Direct Serial0 buffer management via `availableForWrite()`
- Explicit TX drain before critical operations
- State synchronization at transition points

**New functions (main.cpp):**
- `waitForMidiTxDrain()` - TX buffer drain with timeout
- `sendMidiPanic()` - MIDI cleanup sequence
- `clearKeyboardNoteTracking()` - Tracking synchitecture review.

---

## Testing Recommendations

After upgrade, test these scenarios to validate MIDI reliability:
1. Rapid chord bursts (5+ notes simultaneously)
2. Mode changes while keys are pressed (Scale → Chord → Arp)
3. Enter light sleep mid-note (auto-sleep timeout)
4. BLE disconnect during performance
5. High-speed arpeggios (verify no stuck notes)

Expected result: Zero stuck notes, clean MIDI state at all transitions.

---

## Credits

MIDI reliability improvements developed in response to field testing with Polyend Tracker Mini. Root cause analysis identified state tracking desynchronization as primary issue.

Battery charging improvements based on real-world usage patterns where computers sleep during overnight charges.

---

## Build Information

**Version:** v2.1.0  
**Build Date:** June 13, 2026  
**MD5:** `83851f6356f6a2d5f63c9000751cd5d2`  
**Flash at offset:** `0x0` (complete image with bootloader + partitions + app)

**Memory Usage:**
- **RAM:** 18.3% (59,964 / 327,680 bytes)
- **Flash:** 12.5% (1,034,549 / 8,257,536 bytes)
- **TX Buffer:** 512 bytes (increased from 256)

**Behavioral Changes:**
- Charging LEDs now pulse for estimated time based on battery %
- Computer sleep no longer stops charging LED indication after 3 minutes
- Manual battery % adjustment resets charging timer

**BLE Compatibility:** All characteristics unchanged. Compatible with KB1-Config v2.1.0 and earlier.
