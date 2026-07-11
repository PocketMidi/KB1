# KB1 Firmware v2.2.0 Release Notes

**Release Date:** July 11, 2026  
**Focus:** Low battery warning system + MIDI protection at end-of-battery

---

## Overview

KB1 now gives visual warnings when battery is running low, using the octave LEDs as a graduated alert system. A hardware-level brownout handler also sends MIDI All Notes Off before the chip resets, preventing stuck notes on connected instruments when the battery dies.

Additionally fixes a regression in v2.1.0 where charging LEDs would not activate if the battery had previously been calibrated to 100%.

---

## New Features

### Graduated Low Battery LED Warning

**Feature:** When battery monitoring is calibrated and charge drops below 25%, the two octave LEDs pulse as a warning. Urgency increases as battery depletes further.

**Pattern by level:**

| Battery | Pattern | Cycle |
|---------|---------|-------|
| ≤ 25% | Double-blink, 8s pause | ~10s |
| ≤ 20% | Double-blink, 4s pause | ~6s |
| ≤ 15% | Continuous slow pulse | ~2.4s |

**Double-blink timing:** 800ms on → 300ms off → 800ms on → pause

**Activation conditions:**
- Only fires when battery is calibrated (never for uncalibrated devices)
- Automatically inactive during charging
- Pattern escalates in real-time as percentage drops through thresholds

**User action:** Connect USB to charge. The warning clears as soon as charging mode activates.

---

### Brownout MIDI Cleanup (Hardware Safety Net)

**Problem:** When battery is critically depleted (~3V), the ESP32 brownout detector fires and resets the chip. A partial MIDI message transmitted during the reset sequence can cause connected instruments (e.g. Polyend Tracker) to receive garbled data and hang with stuck notes.

**Fix:** Registered a hardware interrupt alongside the default brownout handler. On brownout detection, the ISR writes MIDI All Notes Off directly to the UART hardware FIFO (3 register writes — safe in interrupt context, no heap, no RTOS). The default reset then proceeds normally.

**Result:** Connected instruments receive a clean All Notes Off before the device resets, preventing stuck notes from power failure.

**Note:** This complements the boot-time MIDI panic already present since v2.1.0, which handles recovery after the reset completes.

---

## Bug Fixes

### Charging LEDs Not Activating After Full Charge Cycle (v2.1.0 Regression)

**Problem:** In v2.1.0, `batteryFull = (estimatedPercentage >= 100)` was added as an exit condition for charging LEDs. Since `estimatedPercentage` persists in NVS, any device previously calibrated to 100% would immediately evaluate `batteryFull = true` at the start of the next charging session — preventing the charging LEDs from ever starting.

**Fix:** Removed `!batteryFull` from the charging LED enable condition. Charging completion is already correctly handled by `isChargingMode` being set to `false` when the full charge time elapses in `updateBatteryMonitoring()`. The redundant `batteryFull` check only caused this regression.

---

## Web App (KB1-Config v2.2.0)

**Battery Status modal:** Added informational note explaining the low battery LED warning behaviour. Users who have not calibrated battery monitoring will see the note but the warning remains inactive until calibration is complete.

---

## Testing Notes

To test the low battery warning without depleting the battery:

1. Connect via BLE in KB1-Config
2. Open Battery Status → Advanced → Set Level to e.g. `24` → tap Set Level
3. The octave LEDs should begin the double-blink pattern within 1 second
4. Set to `19` to see the faster 4s pause pattern
5. Set to `14` to see continuous pulsing
6. Set back to a value above `25` — LEDs stop immediately

---

## Build Information

**Version:** v2.2.0  
**Build Date:** July 11, 2026  
**MD5:** `58208c04539f88e091b2d02dcd101c92`  
**Flash at offset:** `0x0` (complete image with bootloader + partitions + app)

**Memory Usage:**
- **RAM:** 18.3% (59,972 / 327,680 bytes)
- **Flash:** 12.5% (1,035,201 / 8,257,536 bytes)

**Behavioural Changes:**
- Octave LEDs used for low battery indication (only when calibrated and below 25%)
- Charging LEDs now correctly activate after a previously-completed full charge cycle
- MIDI All Notes Off sent via hardware register on brownout before chip resets

**BLE Compatibility:** All characteristics unchanged. Compatible with KB1-Config v2.2.0 and earlier.
