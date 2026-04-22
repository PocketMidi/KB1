---
name: kb1-conventions
description: KB1 design patterns, protocols, and conventions. Use when implementing new features, modifying BLE protocol, planning breaking changes, reviewing firmware contracts, or understanding system assumptions. Contains LED color conventions, MIDI implementation (Serial MIDI, clock generation), BLE protocol assumptions, breaking changes log, critical checklists, and validation rules. Essential for maintaining consistency and avoiding regressions.
---

# KB1 Design Conventions & Protocols

Design patterns, protocols, and breaking change management for KB1 firmware.

---

## LED Color Conventions

**Global LED Color Meaning:**

- **PINK LED** = Up / Forward / Increase / Positive direction
- **BLUE LED** = Down / Reverse / Decrease / Negative direction

### Default Behavior

Levers and press button functions follow this convention by default through their hardware configuration.

### Special Mode Overrides

Some modes require explicit LED control to maintain the convention:

#### Pattern Selector (CC 201)
- **Forward cycling** (P1→P2→P3...→P6→P1): Pink LED on, Blue LED off
- **Reverse cycling** (P6→P5→P4...→P1→P6): Blue LED on, Pink LED off
- LEDs override default button/switch assignments only during Pattern Selector operation

**Implementation locations:**
- `TouchControl.h`: Detects direction from `offsetTime` setting
- `LeverPushControls.h`: Tracks pattern changes to detect direction

#### Octave Control
- **Octave Up:** Uses OCTAVE_UP LED (separate from Pink/Blue)
- **Octave Down:** Uses OCTAVE_DOWN LED (separate from Pink/Blue)

### Implementation Rule

When adding new control modes that have directional behavior, ensure LED feedback follows the **Pink=Up / Blue=Down** convention by explicitly setting LED states in the control logic.

---

## MIDI Implementation

### Hardware Interface

**CRITICAL: KB1 uses SERIAL MIDI ONLY (UART)**

```cpp
MIDI_CREATE_INSTANCE(HardwareSerial, Serial0, MIDI)
```

- **Interface:** `Serial0` (Hardware UART on ESP32-S3)
- **Declaration:** `MIDI_NAMESPACE::MidiInterface<MIDI_NAMESPACE::SerialMIDI<HardwareSerial>>`
- **Baud Rate:** 31,250 (standard MIDI)
- **Physical:** 5-pin DIN or 3.5mm TRS MIDI connector
- **NOT USB MIDI:** KB1 does not support USB MIDI class device mode

### MIDI Clock Protocol

**CRITICAL: MIDI Clock requires continuous messages (cannot be "set and forget")**

**Messages:**
- **0xF8 (Clock)** — Must send **24 times per quarter note** at precise intervals
- **0xFA (Start)** — Sent once to begin playback
- **0xFC (Stop)** — Sent once to halt playback
- **0xFB (Continue)** — Resume from stopped position

**Timing Example (120 BPM):**
```
120 beats/min × 24 pulses/beat = 2,880 pulses/min
= 48 clock messages per second
= One 0xF8 message every ~20.8ms
```

**If clock messages stop, receiving device (e.g., Tracker Mini) halts immediately.**

### Tracker Mini MIDI Configuration

**Key Insight: Clock and Transport are SEPARATE settings**

#### Clock Settings
- **Clock In:** Internal / USB / MIDI In jack
- **Clock Out:** Off / USB / MIDI Out jack / USB+MIDI jack
- **Clock Sync Delay:** -50 to +50 (latency compensation, 1/250 pattern step units)

#### Transport Settings
- **Transport In:** Internal / USB / MIDI In jack
- **Transport Out:** Off / USB / MIDI Out jack / USB+MIDI jack

#### Other MIDI Settings
- **Notes In:** Off / USB / MIDI In jack / USB+MIDI jack
- **Notes Input Channel:** All channels or Ch 1-16
- **CC In:** Off / USB / MIDI In jack / USB+MIDI jack
- **CC In Channel:** All channels or Ch 1-16
- **MIDI Out:** Off / USB / MIDI Out jack / USB+MIDI jack
- **Middle C:** C-3, C-4, C-5, C-6 (calibrate note ranges)

### KB1 + Tracker Mini Workflow Options

#### Option 1: Clock Sync Only (Recommended)

**Tracker Settings:**
- Clock In = MIDI In jack (receives KB1's Serial MIDI timing)
- Transport In = Internal (manual play/pause on Tracker)

**KB1 Sends:**
- MIDI Clock (0xF8) continuously when enabled
- No Start/Stop/Continue

**Result:** KB1 provides timing reference, Tracker playback controlled manually. Arpeggiator and Tracker stay in perfect sync.

#### Option 2: Full Remote Control

**Tracker Settings:**
- Clock In = MIDI In jack
- Transport In = MIDI In jack

**KB1 Sends:**
- MIDI Clock (0xF8) continuously
- Start (0xFA) when arpeggiator engages
- Stop (0xFC) when arpeggiator disengages

**Result:** Arpeggiator controls Tracker playback automatically. More integrated but invasive.

### Clock Generation Implementation

```cpp
// At given BPM, calculate microseconds between clock pulses
unsigned long microsBetweenClocks = 2500000 / bpm; // 24 PPQN formula

// Send 0xF8 at precise intervals
if (micros() - lastClockMicros >= microsBetweenClocks) {
    MIDI.sendRealTime(midi::Clock);
    lastClockMicros = micros();
}
```

**Default Recommendation:**
- Clock: ON (always synced)
- Transport: OFF (user controls playback)
- BPM: 40-300 range (shared with arpeggiator)

---

## Critical Assumptions

### BLE Protocol Assumptions

#### Assumption: Single 256-Byte Buffer
**What:** All device settings are sent as a single 256-byte buffer over BLE  
**Why:** Firmware expects atomic updates to prevent partial/inconsistent state  
**Violation:** Sending settings in multiple chunks would desync firmware state  
**Validation:** Check buffer size in `encodeSettings()` — must be exactly 256 bytes

#### Assumption: Firmware Validates on Receive, NOT on Encode
**What:** Web app encodes settings, firmware validates and may reject  
**Why:** Firmware is source of truth for what's valid (hardware constraints)  
**Violation:** Web app must not assume encoded settings will be accepted  
**Validation:** Always check device response after BLE write, handle rejections

#### Assumption: Invalid Settings Reject ENTIRE Update
**What:** If ANY field is invalid, firmware ignores the whole settings buffer  
**Why:** Prevents partial updates that would leave device in inconsistent state  
**Violation:** Assuming individual fields can fail independently  
**Validation:** Use `validateSettingsDetailed()` before sending to get clear errors

### Strum Speed Assumptions

#### Assumption: Signed 16-bit Value (-360 to +360)
**What:** Strum speed is ALWAYS stored as signed int, negative = reverse  
**Why:** Firmware uses sign bit to determine strum direction  
**Violation:** Storing as unsigned or separate direction field breaks firmware contract  
**Validation:** Type is `number` in TS, but range check in validation

#### Assumption: Steps of 5ms Only
**What:** Strum speed must be multiple of 5 (e.g., 5, 10, 15, not 7 or 13)  
**Why:** Firmware timer resolution is 5ms, other values round unpredictably  
**Violation:** Sending non-multiples causes inconsistent timing  
**Validation:** `speed % 5 === 0` in `firmware-contract.ts`

#### Assumption: Forbidden Range -4 to +4
**What:** Values in range -4 to +4 are NOT allowed (firmware rejects them)  
**Why:** Too fast to be physically usable, firmware enforces minimum 5ms  
**Violation:** Sending values in this range causes BLE update to fail  
**Validation:** Check `abs(speed) >= 5` in validation

#### Assumption: CC 200 is BIPOLAR, All Others are UNIPOLAR
**What:** Only CC 200 uses bipolar mapping (0-63=negative, 65-127=positive)  
**Why:** Allows single lever to control forward AND reverse smoothly  
**Violation:** Treating CC 200 like other CCs breaks lever sync  
**Validation:** Use `isBipolarCC()` from `firmware-contract.ts`

### Root Note Assumptions

#### Assumption: Chromatic Mode Ignores Root Note
**What:** When `scaleType=0` (Chromatic), firmware ignores the `rootNote` value  
**Why:** Chromatic scale includes all 12 notes regardless of root  
**Violation:** Assuming rootNote matters in Chromatic causes confusion  
**Validation:** UI can allow `rootNote=0` for Chromatic, displays as C for clarity

#### Assumption: Non-Chromatic Requires 60-71 Range
**What:** For all scales except Chromatic, `rootNote` must be 60-71 (C to B)  
**Why:** Firmware uses rootNote as base for interval calculation  
**Violation:** Values outside this range cause firmware to reject settings  
**Validation:** Check `isValidRootNote(note, isChromatic)` in `firmware-contract.ts`

#### Assumption: UI Defaults to C (60) for Invalid Values
**What:** If loaded rootNote is outside 60-71, UI displays/uses 60 (C)  
**Why:** Prevents broken UI when legacy presets have invalid values  
**Violation:** Displaying invalid value confuses users, may break picker controls  
**Validation:** `rootNoteValue` computed in `KeyboardSettings.vue`

### Voicing Range Assumption

#### Assumption: 1 to 3 Only (Inclusive)
**What:** Voicing must be exactly 1, 2, or 3 (number of octaves to stack)  
**Why:** Firmware has fixed array size for chord note stacking  
**Violation:** 0 or 4+ causes buffer overflow or silent failure  
**Validation:** Use `isValidVoicing()` from `firmware-contract.ts`

### Performance Assumptions

#### Assumption: Drag Handlers Only Emit on Value Change
**What:** `updateValueFromPosition()` checks if value changed before emitting  
**Why:** Prevents 100+ redundant model updates during single drag gesture  
**Violation:** Assuming updates happen on every pixel breaks performance  
**Validation:** Always check `if (newValue !== currentValue)` before emit

#### Assumption: Validation Happens BEFORE Send, NOT During Drag
**What:** Expensive validation (`validateSettingsDetailed`) only runs on BLE send  
**Why:** Keeps drag interactions smooth (60fps), validation when it matters  
**Violation:** Calling validation in drag handlers kills performance  
**Validation:** NO validation calls in `updateValueFromPosition()`

---

## Breaking Changes Log

Track all breaking changes to prevent accidental regressions.

### v1.7 (April 2026)

#### Scale Validation - rootNote=0 Now Valid for Chromatic
**What Changed:**
- BLE protocol validation now allows `rootNote: 0` when `scaleType: 0` (Chromatic mode)
- Previously rejected rootNote < 60, now skips validation for Chromatic

**Why:**
- Firmware ignores rootNote in Chromatic mode (all 12 notes active regardless)
- Old validation was overly strict, causing false rejections

**Impact:**
- **Firmware:** Old firmware (v1.6 and earlier) may reject settings with rootNote=0
- **Web App:** Old web app (v1.6 and earlier) won't send rootNote=0
- **Presets:** Existing presets likely have rootNote=60, no migration needed

**Migration:**
- Update both firmware AND web app together for v1.7
- If rootNote=0 causes issues, set to 60 (C) — firmware ignores it anyway

**Code Locations:**
- `KB1-config/src/ble/kb1Protocol.ts` — `validateScale()`
- `KB1-config/src/components/KeyboardSettings.vue` — `rootNoteValue` computed

#### Strum Speed - Bipolar CC 200 Mapping
**What Changed:**
- CC 200 now uses BIPOLAR mapping: MIDI 0-63 = reverse, 65-127 = forward
- Previously used unipolar mapping (exact behavior varied by firmware version)
- Lever controls now sync correctly with negative strum speeds

**Why:**
- Allows single physical lever to control both forward AND reverse strum smoothly
- Eliminates need to toggle direction separately when using lever
- Matches user expectation: full left = full reverse, full right = full forward

**Impact:**
- **Firmware:** Lever behavior completely changed — MUST update firmware
- **Web App:** Lever assignments to CC 200 will behave differently
- **Presets:** Stored strum speeds are unchanged (still -360 to +360)

**Migration:**
- CRITICAL: Update firmware before using lever for strum speed
- Old firmware with new web app: Lever will send wrong values
- Re-test lever assignments after firmware update

**Code Locations:**
- `firmware/src/controls/LeverControls.h` — Constructor and `syncValue()`
- `firmware/src/controls/LeverPushControls.h` — Send conversion

#### UI Model - Strum Speed Now Unipolar Display
**What Changed:**
- UI now displays strum speed as positive value (5-360) with separate FWD/REV toggle
- Previously displayed bipolar value (-360 to +360)
- Model still stores signed value for firmware compatibility

**Why:**
- Cleaner UX: speed and direction are independent concepts
- Matches user mental model: "how fast" vs "which direction"
- Eliminates confusing negative numbers

**Impact:**
- **Breaking:** Custom code reading `chord.strumSpeed` directly may need updates
- **Non-Breaking:** BLE protocol unchanged, presets unchanged
- **UI Only:** Pure display change, no data model changes

**Code Locations:**
- `KB1-config/src/components/KeyboardSettings.vue` — `smartSliderValue` computed

#### Performance - Drag Update Optimization
**What Changed:**
- Drag handlers now only emit updates when value actually changes
- Previously emitted on every pixel movement (~100 updates per drag)
- Now emits only on value changes (~7 updates per drag for strum speed)

**Why:**
- Eliminates unnecessary re-renders and model cloning during drag
- Dramatically improves drag responsiveness
- Reduces CPU/battery usage on mobile devices

**Impact:**
- **Non-Breaking:** Pure performance optimization
- **Behavior:** Drag feels more responsive, no functional change
- **Warning:** Do NOT add code that expects updates on every pixel

**Code Locations:**
- `KB1-config/src/components/KeyboardSettings.vue` — `updateValueFromPosition()`

### v1.6 (Previous Release)

#### Battery Persistence
**What Changed:**
- Battery % now persists correctly after reboot
- Fixed: Manual battery set now calculates equivalent consumed time

**Impact:**
- Requires firmware update to take effect
- Old firmware: Battery resets to 100% after reboot

---

## Critical Checklist

**⚠️ READ BEFORE ANY ARCHITECTURE CHANGES ⚠️**

### Before Modifying Task Core Assignments, I2C, BLE, or Touch Sensor Code:

#### MANDATORY: Check These First
1. **`/memories/repo/esp32-architecture.md`** (now `.github/skills/kb1-hardware/`) — Core constraints, I2C limitations
2. **`/memories/repo/led-conventions.md`** (now here) — LED color meanings (Pink=Up, Blue=Down)
3. **`/memories/repo/serial-output-format.md`** (now `.github/skills/kb1-optimization/`) — Compact logging formats
4. **`/memories/repo/touch-sensor-optimizations.md`** (now `.github/skills/kb1-optimization/`) — Touch sensor behavior

### Hard Constraints (DO NOT VIOLATE)

**❌ NEVER Move These Off Core 1:**
- **BLE Stack** → ESP-IDF hardware limitation, causes disconnects
- **Touch Sensor** → Hardware peripheral affinity, returns garbage on Core 0
- **ALL I2C Tasks** → `readInputs` + `ledTask` MUST be same core
  - MCP23X17 GPIO expanders (buttons, levers, LEDs)
  - Cross-core I2C causes: `Unfinished Repeated Start transaction!` crashes

**❌ NEVER Split I2C Devices Across Cores:**
- `mcp_U1` and `mcp_U2` both require Core 1
- All keyboard, lever, octave button, and LED updates use I2C
- ESP32 I2C driver mutex cannot handle cross-core coordination

### Safe Optimizations (Always Allowed)

**✅ Serial Output Changes:**
- Compact format (already optimized ~80% reduction)
- Auto-detection (USB CDC connection detection)
- Throttling/rate limiting

**✅ I2C Speed:**
- Current: 400kHz (safe, tested)
- Maximum: 1.7MHz (MCP23X17 spec limit)
- Heat: Negligible at any supported speed

**✅ Task Priorities:**
- Adjust priority within same core (readInputs can be priority 2, ledTask priority 1)
- Does NOT move tasks between cores

**✅ Logic Changes:**
- LED patterns, control algorithms, MIDI mapping
- Anything that doesn't touch core assignment or hardware peripherals

### Battery/Charging Hardware Constraints

**⚡ CRITICAL: Charge Controller Boot Sequence Requirement**

**Hardware behavior:**
- **USB at boot** (boot with USB already connected) → **Bypass/power mode** → Device powered but **NOT charging battery**
- **Boot on battery → THEN plug USB** → **Charging mode** → Device powered AND **charging battery**

**Firmware tracking:**
- Detect boot sequence: `usbConnectedAtBoot = isUsbPowered()` captured EARLY in setup(), BEFORE `loadBatteryState()`
- Valid sequence (boot on battery → plug USB): `isChargingMode = true` → Run calibration timer, track charge
- Invalid sequence (USB at boot): `isChargingMode = false` → Stay uncalibrated (254), show ? in GUI
- Partial charges DO NOT accumulate (all-or-nothing: 5hrs continuous required)

**⚠️ BOOT SEQUENCE BUG PATTERN (Fixed April 2026 - DO NOT REINTRODUCE)**

**The trap:** `setup()` has a fresh-boot block that runs AFTER `loadBatteryState()` to handle sleep/wake:
```cpp
if (resetReason != ESP_RST_DEEPSLEEP) {
    usbConnectedAtBoot = false;       // ← WRONG: throws away early hardware detection
    batteryState.lastUsbState = false; // ← WRONG: causes fake "USB just plugged" event
}
```

**Why it causes charging LEDs on bypass boot:**
1. `usbConnectedAtBoot = isUsbPowered()` correctly detects USB at line ~956
2. `loadBatteryState()` overwrites it with stale NVS at line ~997
3. Fresh-boot block then sets it to `false` → `usbConnectedAtBoot` is now wrong
4. `lastUsbState = false` but USB IS connected → `usbJustPlugged = true`
5. `usbConnectedAtBoot == false` → takes charging path → LEDs pulse

**The fix (MUST keep):** Capture early detection BEFORE loadBatteryState, restore in fresh-boot block:
```cpp
bool usbAtBootEarlyDetection = isUsbPowered();  // Before loadBatteryState()
// ... after loadBatteryState() ...
if (resetReason != ESP_RST_DEEPSLEEP) {
    usbConnectedAtBoot = usbAtBootEarlyDetection;      // Restore hardware detection
    batteryState.lastUsbState = usbAtBootEarlyDetection; // Match actual state (no fake event)
}
```

### When Proposing Performance Improvements:

1. **Check memory files FIRST** before suggesting
2. **Verify against esp32-architecture.md** constraints
3. **State assumptions explicitly** (e.g., "Assuming I2C can be split..." → WRONG)
4. **If uncertain, ask user** rather than implementing

---

## Firmware Release Process (v1.6.2+)

### ⚠️ NEVER copy `.pio/build/firmware.bin` directly as a release binary

`.pio/build/seeed_xiao_esp32s3/firmware.bin` is the **app partition only** (must be flashed at offset `0x10000`).

Flashing it at offset `0x0` causes immediate boot loop: `Checksum failure. Calculated 0x57 stored 0xf3`

### ✅ Always use `build_complete.sh` to produce the release binary

```bash
cd firmware
bash build_complete.sh
```

This runs `esptool.py merge_bin` to combine:
- `bootloader.bin` @ `0x0`
- `partitions.bin` @ `0x8000`
- `firmware.bin`   @ `0x10000`

**Output:** `KB1-firmware-v1.6.2.bin` — merged, flashes at offset `0x0`

### Release Checklist (All Three Repos)

1. Bump `FIRMWARE_VERSION` in `src/objects/Constants.h`
2. Update `FIRMWARE_VERSION` in `build_complete.sh`
3. Run `bash build_complete.sh` → outputs `KB1-firmware-vX.Y.Z.bin` in firmware root
4. Copy bin → `kb1-flash/public/firmware/KB1-firmware-vX.Y.Z.bin`
5. Add entry to `kb1-flash/public/firmware/releases.json` (use `wc -c` for exact byte size)
6. Bump `APP_VERSION` in `KB1-config/src/constants.ts`
7. Commit + push all three repos; tag firmware repo with `vX.Y.Z`

---

## KB1-Config Web App Architecture Notes (post-v1.6.2)

### Connect Sequence — Order is Critical

All sequential in `useDeviceState.connect()`:
1. BLE connect + characteristic discovery (each char individually — DELETE failure doesn't block SAVE/LOAD/LIST)
2. `refreshDevicePresets()` — preset list read
3. `handleLoad()` — reads all 8 settings characteristics → triggers `lastDeviceLoadTime` → MobileScales resets `hasChanges = false`
4. `useBatteryStatus().initBatteryStatus()` — battery read LAST (BLE stack free)

### Battery Icon (`isAvailable` flag)
- Starts `false`, must be set `true` for icon to render
- Set by: `initBatteryStatus()` or `updateBatteryFromKeepAlive()` (whichever fires first)
- `MobileHeader` no longer owns battery init — `useDeviceState.connect()` owns it

### Upload Button Dirty State
- `lastDeviceLoadTime` ref in `useDeviceState` — updated after every successful `handleLoad()`
- `MobileScales` watches it → resets `localSettings` + `hasChanges = false`
- Prevents upload button activating on connect auto-load

### Keep-Alive State Sync (active — firmware v1.6.2+)
- `onKeepAliveState` callback in `bleClient` updates `deviceSettings.chord.strumPattern`, `scale.scaleType`, `scale.rootNote` every 60s
- Firmware v1.6.2 binary (rebuilt Apr 11) populates bytes 2-5 from live `_chordSettings` and `_scaleSettings` refs

---

## Guidelines for Future Changes

### When is Something "Breaking"?
1. **Breaking:** Changes BLE protocol format or validation
2. **Breaking:** Changes firmware behavior that affects saved presets
3. **Breaking:** Changes data model in ways that old code can't handle
4. **Non-Breaking:** Pure UI changes with no data model impact
5. **Non-Breaking:** Performance optimizations that don't change behavior

### How to Minimize Breaking Changes
1. Add new fields as optional, keep old fields working
2. Version the BLE protocol and check version on connect
3. Use feature flags to test new behavior alongside old
4. Document ANY change to validation rules
5. Test with old presets before releasing

### When Breaking Changes are Necessary
1. Fixing critical bugs (like scale validation being too strict)
2. Adding features that fundamentally change how hardware works
3. Performance improvements that require behavioral changes
4. Aligning with user expectations (like bipolar lever mapping)

**Always document WHY the break was necessary!**

---

## Revision History

- **April 22, 2026:** Migrated from `/memories/repo/` to `.github/skills/kb1-conventions/`
- **April 11, 2026:** v1.6.2 release — Battery set%, keep-alive notifications, 88% eval mode stuck bug fixes
- **March-April 2026:** v1.7 development — Scale validation, strum speed bipolar mapping, drag optimization
