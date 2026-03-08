# KB1 Firmware v1.2.5 Release Notes

**Release Date:** March 7, 2026

Version 1.2.5 adds Chord Mode with 15 chord types and 6 strum patterns with looping capabilities. Scale Mode functionality is unchanged.

**Version Identification:**
- Firmware version string: `1.2.5`
- Version characteristic reports correctly via BLE

---

## What's New

### Dual Keyboard Modes

Two keyboard modes, selectable via BLE:

#### **Scale Mode**
- Quantized note output with multiple scale types
- Chromatic, Major, Minor, Pentatonic, Blues, and more
- Natural/Compact key mapping options

#### **Chord Mode**
- Chromatic chord/strum playback
- 15 chord types
- Polyphonic or cascaded note output

### Chord Mode: Two Sub-Modes

Toggle between simultaneous or cascaded playback:

| Mode | Behavior |
|------|----------|
| **Chord** | All chord notes play simultaneously |
| **Strum** | Notes cascade with configurable timing |

### 15 Chord Types

Available chord voicings:

| Category | Chord Types |
|----------|-------------|
| **Triads** | Major, Minor, Diminished, Augmented, Sus2, Sus4, Power |
| **7th Chords** | Major7, Minor7, Dominant7 |
| **Extended** | Major Add9, Minor Add9, Major6, Minor6, Major9 |

### 6 Strum Patterns

Pattern 0 plays the selected chord type. Patterns 1-6 enable pattern looping mode with root note transitions:

| Pattern | Name | Description |
|---------|------|-------------|
| **0** | Chord | Uses selected chord type (simultaneous or cascaded) |
| **1** | Up | Ascending pattern |
| **2** | Down | Descending pattern |
| **3** | Bounce | Back and forth pattern |
| **4** | Inclusive | Center outward pattern |
| **5** | Exclusive | Outward to center pattern |
| **6** | Random | Random note order |

**Pattern Looping Mode (Patterns 1-6):**
- Monophonic looping playback
- Root note transitions while maintaining pattern
- Pattern position tracking across note changes

### Advanced Musical Expression Controls

#### Strum Speed
- Range: 4-120ms delay between cascaded notes
- Controls roll speed in Strum mode
- Sets tempo for pattern looping (Patterns 1-6)

#### Velocity Spread
- Range: 0-100% velocity reduction for upper chord notes
- Creates dynamic contour in chords/strums
- 0% = equal velocity, 100% = maximum reduction

#### Strum Swing
- Range: 0-100% swing/groove amount
- Applies swing timing to odd-numbered notes in the pattern
- 0% = straight timing, 100% = heavy swing
- Requires patterns with 3+ notes

### Preset System Integration

Chord Mode settings stored in presets:
- Mode selection (Scale/Chord)
- Chord/Strum toggle
- Selected chord type
- Active pattern (0-6)
- Strum speed, velocity spread, swing amount

---

## Web App Enhancements (KB1-config v1.2.5)

### Keyboard Settings Panel
- **Mode Selector:** Toggle between Scale Mode and Chord Mode
- **Chord/Strum Toggle:** Switch playback behavior in Chord Mode
- **Pattern Carousel:** Visual selector for strum patterns with icons
- **Pattern Name Display:** Shows active pattern name
- **Chord Type Dropdown:** Select from 15 chord types
- **Expression Controls:** Sliders for strum speed, velocity spread, and swing

### Visual Feedback
- Pattern icons provide instant recognition of strum pattern type
- Active pattern highlighted in carousel
- Disabled controls when not applicable (e.g., swing disabled for 2-note patterns)
- Tooltips explain each parameter's musical effect

---

## Technical Details

### New Code Modules

**ChordSettings Structure** (`src/objects/Settings.h`)
- Encapsulates all chord mode configuration
- Integrated into PresetData for persistence
- Default values ensure musical playback out-of-the-box

**Strum Pattern Engine** (`src/music/StrumPatterns.h`)
- 6 pre-defined patterns: Up, Down, Bounce, Inclusive, Exclusive, Random
- Pattern 0 uses chord type intervals
- Velocity spread and swing timing calculations

**Pattern Looping Logic** (`src/controls/KeyboardControl.h`)
- Monophonic looping with root note transitions
- Pattern position tracking
- Automatic note-off management
- Tempo-locked timing

### Modified Files
- `src/controls/KeyboardControl.h` — Chord logic, pattern looping, pattern playback
- `src/objects/Settings.h` — Added ChordSettings structure to PresetData
- `src/bt/CharacteristicCallbacks.cpp` — BLE handlers for chord/strum parameters
- `src/objects/Constants.h` — Version bump to `1.2.5`
- `src/main.cpp` — Chord mode initialization and state management

### BLE Characteristics (New)
- Keyboard Mode (Scale/Chord)
- Chord/Strum toggle
- Chord type selection
- Strum pattern selection (0-6)
- Strum speed (4-120ms)
- Velocity spread (0-100%)
- Strum swing (0-100%)

---

## Compatibility

- **Web App:** Requires KB1-config v1.2.5 for full chord mode UI
- **Presets:** Fully backward compatible with v1.1.x and v1.2.0 presets
  - v1.1.x/v1.2.0 presets load with default chord settings (Major chord, Pattern 0)
  - v1.2.5 presets include complete chord/pattern configuration
- **Hardware:** All KB1 hardware revisions supported

---

## Breaking Changes

**None.** All previous functionality preserved:
- Scale Mode works identically to v1.2.0
- Cross-lever BLE gesture unchanged
- Lever controls, touch, octave buttons function as before
- Existing presets load and save correctly

---

## Upgrade Instructions

1. **Flash Firmware:**
   - Use PlatformIO: `pio run --target upload --environment seeed_xiao_esp32s3`
   - Or use complete image: Flash `KB1-firmware-v1.2.5.bin` at offset 0x0

2. **Update Web App:**
   - Navigate to KB1-config v1.2.5
   - Chord mode controls appear in Keyboard Settings panel

3. **Try Chord Mode:**
   - Enable BLE (cross-lever gesture: push both levers toward center, hold 3s)
   - Connect to web app
   - In Keyboard Settings, select "Chord Mode"
   - Toggle "Chord/Strum" and experiment with patterns
   - Adjust strum speed, velocity spread, and swing

4. **Explore Presets:**
   - Save chord configurations as presets
   - Browse community presets

---

## Known Limitations

- **Pattern Polyphony:** Patterns 1-6 are monophonic (one note at a time)
- **Swing Resolution:** Applied to odd-numbered notes only
- **Note Duration:** Pattern notes have fixed 80% note duration

---

## Credits

Chord Mode adds versatile polyphonic playback and strum pattern capabilities to the KB1.

**Next Steps:** See [Chord/Arp Planning Document](CHORD_ARP_PLANNING_v2.0.md) for detailed implementation notes and future enhancements.
