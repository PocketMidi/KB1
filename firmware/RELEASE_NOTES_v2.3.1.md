# KB1 Firmware v2.3.1 Release Notes

**Release Date:** August 15, 2026  
**Focus:** Critical bugfixes for octave button responsiveness and battery charging LED stability.

---

## Bug Fixes

### Octave Button Debounce Stabilization

Octave buttons (±octave) now have proper debounce filtering to prevent accidental octave shifts.

- **PRESS_DEBOUNCE_MS:** 15ms — filters electrical noise on button down
- **RELEASE_DEBOUNCE_MS:** 40ms — filters electrical noise on button up (longer to catch cleaner release edge)
- **RETRIGGER_LOCKOUT_MS:** 150ms — prevents double-triggers if user holds button too long or presses repeatedly within 150ms
- **Impact:** Octave shifts are now reliable and predictable; eliminates accidental multi-octave jumps from noisy button contacts

### Battery Charging LED Pulse Stability

Fixed critical issue where charging LEDs would drop out or behave erratically during extended charging sessions.

- **Root cause:** USB disconnect detection used simple frame counter check without grace period. When computer sleeps mid-charge, USB frame counter stops incrementing, falsely triggering "USB disconnected" logic that suppressed charging LEDs.
- **Fix:** Added 180-second (3-minute) grace period. USB frame counter is only checked during the first 3 minutes after plug-in (catches real unplugs quickly). After 3 minutes, frame counter is ignored — charging mode continues until battery is full, immune to computer sleep events.
- **LED lockout minimum:** 30-minute minimum LED pulse duration to ensure visible feedback even on low battery charges
- **Impact:** Charging LEDs pulse reliably throughout entire charge cycle, unaffected by computer sleep or USB bus hiccups

---

## Validation

- ✅ Octave buttons tested on both S3/S4 pins with clean press/release sequences
- ✅ Charging LEDs tested through extended charge cycles with computer sleep/wake
- ✅ USB disconnect detection still responsive within first 3 minutes
- ✅ Backward compatible with all KB1-Config versions

---
