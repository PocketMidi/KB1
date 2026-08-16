# KB1 Firmware v2.3.3 Release Notes

**Release Date:** August 16, 2026  
**Focus:** Root-cause fix for the boot-into-wrong-octave issue introduced in v2.3.1/v2.3.2.

> **Hotfix for v2.3.1/v2.3.2.** If you installed either, update to v2.3.3.

---

## Bug Fixes

### Octave Shift on Boot (Root Cause Found)

Devices would intermittently boot one octave too high, despite the v2.3.2 fix.

- **Root cause:** `startupPulseSequence()` (the boot LED wave-bounce animation) writes to `OCTAVE_UP`/`OCTAVE_DOWN` on the **same MCP23017 chip** as the S3/S4 octave button inputs, and blocks for ~1.3 seconds. v2.3.2's boot-suppression window (400ms) expired long before that sequence finished, so the debounce logic could still interpret interference from the still-running LED writes as a real button press/release.
- **Fix:** Boot suppression is no longer a fixed time guess. `OctaveControl` now stays suppressed until `endBootSuppression()` is explicitly called right after `startupPulseSequence()` completes, tying the gate to the actual event that causes the interference instead of an arbitrary delay.
- **Impact:** Confirmed via boot diagnostics across repeated cold boots — device now reliably boots at octave 0.

---

## Validation

- ✅ Repeated cold boots (5+) all start at octave 0, confirmed via serial diagnostics
- ✅ No spurious octave shifts logged during the boot LED sequence
- ✅ Octave button responsiveness (press-edge trigger, 4ms debounce) unchanged from v2.3.2
- ✅ Backward compatible with all KB1-Config versions

---

## Upgrade Notes

Flash the complete image `KB1-firmware-v2.3.3.bin` at offset `0x0`. Settings and battery calibration are preserved when flashing via KB1 Studio.
