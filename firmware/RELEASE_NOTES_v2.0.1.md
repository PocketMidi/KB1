# KB1 Firmware v2.0.1 Release Notes

**Release Date:** May 20, 2026  
**Focus:** Hotfix — octave buttons broken in v2.0.0

---

## Bug Fixes

### Octave Buttons Non-Functional (Critical)

`octaveControl.begin()` was accidentally omitted when the MIDI panic block was added to `setup()` in v2.0.0. This call configures the MCP23017 octave button pins (S3/S4) as `INPUT_PULLUP` — without it the pins float and octave button presses are never detected.

**Affected:** All v2.0.0 devices — octave up/down buttons completely non-functional.  
**Fix:** Restored `octaveControl.begin()` after `keyboardControl.begin()` in `setup()`.

---

## Upgrade Notes

Direct upgrade from v2.0.0. No settings migration required.
