# KB1 Firmware v2.3.0 Release Notes

**Release Date:** August 7, 2026  
**Focus:** Smart battery charging — calibration progress visibility, charge-aware LED suppression, and manual battery level reliability fixes.

---

## What's New

### Calibration Progress Bar (Web App)

New users who charge before ever connecting the config app now see meaningful progress instead of a generic "5 hour charge required" message.

- When an uncalibrated battery has accumulated partial charge time, the Battery Status modal shows:
  - **"Calibration in progress — ~Xh Ym remaining"** instead of static instructions
  - A bronze progress bar showing how far along the initial 5-hour calibration is
- Progress is live — if actively charging, the bar advances in real time
- Backward compatible: older firmware (pre-v2.3.0) still shows the static text gracefully

### Charge LED Auto-Suppress During Play

Charging LEDs (pink/blue pulse pattern) no longer interfere with playing.

- **Any keypress, lever move, or touch** immediately suppresses the charging LED pulse
- LEDs resume pulsing automatically after **30 seconds of silence**
- Zero user configuration required — the device detects play activity and gets out of the way
- LEDs always show correctly on fresh plug-in (not suppressed at boot before first keypress)

---

## Bug Fixes

### Manual Battery % No Longer Reverts to Uncalibrated

Setting the battery level manually via the Advanced panel now persists correctly through charging.

- **Root cause:** `isFullyCharged` was set to `false` for any manually-set % below 100%. The charging code then treated the battery as uncalibrated and overwrote the value with 254 on the next update cycle.
- **Fix:** A manual battery set always implies the battery is calibrated — `isFullyCharged` is now set to `true` regardless of the percentage value.

### Manual Battery % No Longer Overflows to 100% During Charging

- **Root cause:** Prior accumulated charge time (from the initial calibration session) was not cleared on manual set. During the next charging session, the firmware added all prior charge time to the new session total, which exceeded the deficit and calculated 100%.
- **Fix:** `accumulatedChargeMs` is now reset to 0 on manual battery set. Future charging is tracked fresh relative to the new baseline.

### Charge LED Suppress Timeout No Longer Triggers at Boot

- **Root cause:** `lastActivityMillis` is initialized to `millis()` at boot for sleep tracking. The LED suppression check used this variable, causing a 30-second blackout of charge LEDs immediately after every boot even on fresh plug-in.
- **Fix:** A dedicated `lastPlayActivityMs` variable (initialized to 0) is used for LED suppression. Suppression only activates after the first real keypress.

---

## BLE Protocol Change

Battery status characteristic extended from **10 → 14 bytes**.

| Offset | Field | Type |
|--------|-------|------|
| 0 | percentage | uint8 |
| 1–4 | remainingSeconds | uint32 LE |
| 5 | usbConnected | uint8 |
| 6–9 | calibrationTimestamp | uint32 LE |
| **10–13** | **accumulatedChargeMs** | **uint32 LE** |

`accumulatedChargeMs` includes any in-progress session time when the device is actively charging, enabling the web app to display live calibration progress.

Web app decodes all three packet sizes gracefully (6, 10, and 14 bytes).

---

## Technical Changes

### Firmware (`src/`)

- `BluetoothController.cpp`: Battery characteristic extended to 14 bytes; `accumulatedChargeMs` (NVS + live session) appended at bytes 10–13
- `CharacteristicCallbacks.cpp` (command 0x02 — manual battery set):
  - `isFullyCharged = true` (was `percentage == 100`)
  - `accumulatedChargeMs = 0` + `chargeSessionStartMs = 0` reset
  - `batAccChgMs` NVS key written as 0
- `main.cpp`: `lastPlayActivityMs` global added (initialized to 0); set on key/lever/touch input in `readInputs()` loop
- `main.cpp`: Charge LED block uses `lastPlayActivityMs > 0 && (millis() - lastPlayActivityMs) < 30000` for suppression (was `lastActivityMillis`)

### Web App (`KB1-config/src/`)

- `kb1Protocol.ts`: `BatteryStatus` interface gains `accumulatedChargeMs?: number`; `decodeBatteryStatus()` reads bytes 10–13 when packet ≥ 14 bytes
- `BatteryModal.vue`: Calibration progress bar + "~Xh Ym remaining" text in step 3 of charging instructions (shown when `percentage === 254 && accumulatedChargeMs > 0`)
- `useBatteryStatus.ts`: Eval mode mock updated — simulates uncalibrated device with 3hr partial charge for UI testing

---

## Upgrade Notes

- NVS settings are fully preserved — no calibration data is lost on upgrade
- If currently at a manually-set battery % before upgrading, re-set the level once after flashing to clear the stale `accumulatedChargeMs` from NVS
- BLE protocol change is backward compatible — web app v2.3.0 works with older firmware (no `accumulatedChargeMs` field)
