# KB1 Firmware v2.3.2 Release Notes

**Release Date:** August 15, 2026  
**Focus:** Hotfix for the octave button debounce introduced in v2.3.1.

> **Hotfix for v2.3.1.** If you installed v2.3.1, update to v2.3.2. All other v2.3.1 changes (battery charging LED stability) are unchanged and carried forward.

---

## Bug Fixes

### Octave Shift on Boot (Regression in v2.3.1)

Devices would frequently boot one octave too high.

- **Root cause:** The MCP23017 octave pins can read LOW before their internal pullups settle at startup. The v2.3.1 debouncer latched this as a real press and then fired an octave shift when the pin settled HIGH.
- **Fix:** Each octave button now starts *unarmed* and only begins accepting input after a stable released state has been observed. Any pre-settle glitch is discarded instead of becoming a press.
- **Impact:** Device reliably boots at octave 0.

### Octave Button Responsiveness

The v2.3.1 debounce required a firm, deliberate press and only acted once the button was released.

- **Trigger moved to the press edge** (was release edge) — the octave shift now happens on contact instead of after a full press-and-release cycle.
- **PRESS_DEBOUNCE_MS:** 15ms → **4ms** — light taps now register.
- **RELEASE_DEBOUNCE_MS:** 40ms (unchanged) — keeps a bouncing contact from re-opening into a new press.
- **RETRIGGER_LOCKOUT_MS:** 150ms → **180ms** — now the sole guard against double/triple taps, so it spans the full bounce tail. Effective max repeat rate ~5.5 shifts/sec.
- **Impact:** Octave buttons feel immediate again while still rejecting the multi-tap behavior that v2.3.1 was written to fix.

---

## Validation

- ✅ Repeated cold boots start at octave 0
- ✅ Light taps register on first press; no double or triple shifts
- ✅ Held buttons shift once, not repeatedly
- ✅ Backward compatible with all KB1-Config versions

---

## Upgrade Notes

Flash the complete image `KB1-firmware-v2.3.2.bin` at offset `0x0`. Settings and battery calibration are preserved when flashing via KB1 Studio.
