# KB1 Firmware v1.2.0 Release Notes

**Release Date:** March 2, 2026

Version 1.2.0 introduces a new cross-lever gesture for BLE activation, providing a more intuitive and ergonomic way to toggle Bluetooth connectivity.

**Version Identification:**
- Firmware version string: `1.2.0`
- Version characteristic reports correctly via BLE

---

## What's New

### Cross-Lever BLE Activation Gesture

**The old method (holding both octave buttons for 3 seconds) has been replaced with a new cross-lever gesture:**

1. **Push both levers toward each other** (left lever → right, right lever → left)
2. **Hold for 3 seconds** while watching progressive LED feedback
3. **Release when LEDs turn off** - BLE is now toggled

### Progressive LED Feedback During Gesture

Visual feedback builds as you hold the gesture:

| Time | Octave Arrows | Pink + Blue LEDs |
|------|---------------|------------------|
| 0s   | ON (solid)    | Pulse @ 800ms    |
| 1s   | ON (solid)    | Pulse @ 500ms    |
| 2s   | ON (solid)    | Pulse @ 250ms    |
| 3s   | **All OFF**   | **All OFF**      |

- **Octave arrows ON immediately** = gesture detected
- **Pulse speed increases** = approaching activation
- **All LEDs OFF** = activation complete, release levers

### Accidental Trigger Prevention

The gesture automatically aborts if any keyboard key is pressed during the hold. This prevents accidental BLE toggles during performance or if the device is jostled in a bag.

### BLE Connection LED Feedback

Visual confirmation when the app connects or disconnects:

| Event | LED Feedback |
|-------|--------------|
| **App connects** | Pink double-blink (200ms) |
| **App disconnects** | Blue single blink |

This provides clear feedback that the app has successfully paired or unpaired.

### Why This Change?

- **Impossible to trigger accidentally** - cross-lever gesture doesn't happen during normal play
- **Keyboard safeguard** - pressing any key cancels the gesture
- **Lever cooldown** - 1.5 second MIDI silence after toggle so lever release doesn't send spurious CC
- **Muscle memory friendly** - "push both levers together" is easy to remember
- **Clear feedback** - you know exactly when the gesture is recognized and when it completes
- **Doesn't interrupt workflow** - levers return to center naturally

---

## Breaking Changes

- **Octave buttons no longer toggle BLE** - use the cross-lever gesture instead
- **No post-toggle LED pulse** - the gesture feedback itself serves as confirmation

---

## Code Changes

### New Files
- `src/controls/BLEGestureControl.h` - Cross-lever gesture detection with progressive LED feedback

### Modified Files
- `src/controls/OctaveControl.h` - Removed BLE toggle logic, simplified to octave-only control
- `src/controls/LeverControls.h` - Added cooldown check to prevent MIDI output after BLE toggle
- `src/objects/Globals.h` - Added `leverCooldownUntil` global for cooldown tracking
- `src/bt/BluetoothController.cpp` - Removed post-toggle LED pulse (superseded by gesture feedback)
- `src/bt/ServerCallbacks.cpp` - Added LED feedback on BLE connect/disconnect
- `src/main.cpp` - Integrated BLEGestureControl

---

## Compatibility

- **Web App:** Compatible with KB1-config v1.x
- **Presets:** Compatible with all v1.1.x presets
- **Hardware:** Requires KB1 hardware with dual levers

---

## Upgrade Instructions

1. Flash firmware via PlatformIO
2. Use the new cross-lever gesture to enable BLE:
   - Push both levers toward center
   - Hold for 3 seconds (watch LED feedback)
   - Release when all LEDs turn off

---

## Documentation Updates

- README.md updated with new BLE activation instructions
- KB1-config README.md updated accordingly
