# KB1 Firmware v2.0.0 Release Notes

**Release Date:** May 17, 2026  
**Focus:** Keyboard section redesign, all new ARP section with chord section overhaul, and functional default controls that record real time on the track.

---

## What's New

The keyboard settings sections in the web app have been reorganized and refreshed. Controls are grouped more logically around how they are actually used.

---

### Functional Default Controls (Press 1 & Lever 1)

Press 1 and Lever 1 now default to controls that are **recorded directly to a track** — not just performance shaping.

**Lever 1 → CC 208 Pitch Bend (factory default):**
- Dedicated pitch bend mode using KB1-reserved CC 208.
- Integrates with keyboard pitch bend system; configurable note-overlap (0–100ms).

**Press 1 → CC 209 Sustain (factory default):**
- Virtual sustain pedal. Sends standard sustain behavior via KB1-reserved CC 209.
- Delayed NoteOff scheduled on key release (500ms default); voice freed immediately so keyboard stays playable.
- MOM: sustain active while held. LAT: each press toggles.
- Sustain suppresses pitch bend retrigger to avoid tone artifacts.
- Duration configurable 100–2000ms in 25ms steps.


---

### Touch pad: FWD/REV Direction Toggle

Continuous mode now supports direction inversion — previously only available in Cycling Parameter mode.

- FWD (`offsetTime = 0`): finger bottom = minCC, top = maxCC.
- REV (`offsetTime > 0`): finger bottom = maxCC, top = minCC.
- Default: CC 1 (Cutoff), Continuous, MIN 38 / MAX 114, REV.

---

### Preset Baseline Reset

Local built-in starter presets cleared for a v2 starter library rebuild. Slot/save/load behavior unchanged.

---

## Technical Changes

### Firmware
- `KeyboardControl`: sustain redesigned from hold-until-off queue to per-release timed NoteOff scheduling using the existing PB pending-offs queue (`MAX_PB_PENDING_OFFS = 24`).
- `KeyboardControl`: pitch bend retrigger suppressed when `_sustainActive` is true.
- `KeyboardControl`: `_baseNote[]` array added to fix Compact/Scale stuck notes during pitch bend.
- `LeverPushControls`: SUSTAIN case passes `onsetTime` as release duration at activation; MOM/LAT behavior driven by `offsetTime`.
- `TouchControl`: CONTINUOUS mode inverts CC mapping when `offsetTime > 0` (REV).
- `Globals.h`: `SUSTAIN = 4` added to `LeverPushFunctionMode` enum.
- `main.cpp`: Press 1 factory default updated to CC 209 / SUSTAIN / 500ms / MOM.
- `main.cpp`: Touch factory default updated to CC 1 / Continuous / MIN 38 / MAX 114 / REV (offsetTime=100).
- `KeyboardControl` strum timing now applies CHORD swing split logic from `gateValue`.
- Removed orphaned legacy gate-chop state (`_strumNoteOffVelocities`) and related dead paths.
- Updated comments and semantics in settings defaults to reflect CHORD swing behavior.

### Web App
- `ccMap.ts`: CC 209 "Sustain" added to KB1 Expression group (press1/2 only).
- `LeverPushSettings.vue`: Sustain mode UI — CC 209 auto-selects SUSTAIN function mode; duration slider (100–2000ms, 25ms steps); Log profile locked; MIN/MAX and level meter hidden.
- `LeverPushSettings.vue`: CC 208 (Pitch Bend) filtered out of press control CC options.
- `LeverSettings.vue`: Level meter hidden when in pitch bend mode (CC 208).
- `TouchSettings.vue`: FWD/REV toggle button now visible for Continuous mode in addition to Cycling Parameter mode.
- `kb1Protocol.ts`: BLE validator updated — CC 209 added to valid range; `functionMode 0–4` bounds check added.
- `kb1Protocol.ts`: leverPush1 default updated to CC 209 / SUSTAIN / 500ms / MOM.
- `kb1Protocol.ts`: touch default updated to MIN 38 / MAX 114 / REV (offsetTime=100).
- `MobileScales.vue` / `MobileControls.vue`: fallback default for Press 1 updated from CC 24 to CC 209.
- Added display formatter/parser mapping for chord swing (internal 10-100 → displayed 50-100).
- Updated chord swing defaults/fallbacks/resets to raw 35 (~64% displayed).
- Added inline comments clarifying touch Gate mode vs chord swing repurpose.
- Updated touch-mode labeling back to Gate in touch-specific contexts.
- Cleared built-in local starter preset definitions for v2 refresh.

---

## Bug Fixes

- **Compact mode stuck notes:** Fixed notes getting stuck when using pitch bend lever in Compact/Scale keyboard mode. Root cause: retrigger logic was not tracking the original (pre-offset) note per key. Added `_baseNote[]` array to store the note played at key-press time, ensuring the correct NoteOff is sent on key release regardless of bend state.
- Fixed render errors in control components caused by incorrect template access (`model.value.ccNumber` vs `model.ccNumber`).
- Corrected note-range label display behavior and meter wiring in related controls.
- Removed stale naming/comment drift after Gate->Swing transition.

---

## Installation

**Firmware Update:**
1. Use KB1 Flash Tool to flash `KB1-firmware-v2.0.0.bin`
2. NVS partition is preserved (battery calibration and settings retained)

**Web App Update:**
- Updated automatically at https://kb1.pocketmidi.com/

---

## Developer Notes

- Keep touch control mode naming as Gate in touch-specific code paths.
- Keep chord timing naming as Swing in chord-specific UI and docs.
- Do not rename `gateValue` in BLE protocol without coordinated firmware+app migration.

---

## Compatibility

- **Firmware**: v2.0.0
- **Web App**: v2.0.0+
- **Hardware**: Seeed XIAO ESP32-S3
- **Bluetooth**: BLE 4.2+

---

**Previous Release:** v1.7.2 (May 1, 2026) - Battery system corrections and power planning  
**Next Planned:** TBD
