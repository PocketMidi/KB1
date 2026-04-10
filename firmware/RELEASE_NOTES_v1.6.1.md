# KB1 Firmware v1.6.1 Release Notes
**Release Date:** April 9, 2026

## 🐛 Bug Fix

### USB Bypass Detection (Silent Fix)
- **Fixed charging LEDs pulsing incorrectly** when device is booted with USB already connected
- **Root cause:** USB state detected correctly at early boot, but then discarded by the fresh-boot/sleep-wake block that runs after NVS load — causing the main loop to see a fake "USB just plugged" event and enter charging mode incorrectly
- **Effect of bug:** LEDs would pulse pink/blue on every USB-at-boot, misleading the user into thinking the battery was charging when it was not (bypass/power mode only)
- **Fix:** Early hardware detection is now preserved through the NVS load and restored correctly for fresh boots

**Who is affected:**
- All users who boot KB1 with USB already connected (wall charger or computer)
- Bug was present since v1.5.0 hotfix (shipped unchanged in v1.6.0)

## ⬆️ Upgrade Notes
No settings or calibration data will be lost — this is a firmware-only fix with full NVS preservation.
