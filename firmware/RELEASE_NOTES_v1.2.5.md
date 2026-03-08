# KB1 Firmware v1.2.5 Release Notes

**Release Date:** March 7, 2026

Version 1.2.5 is a major musical capability expansion that transforms the KB1 into a comprehensive polyphonic chord controller with programmable strum patterns. This release adds Chord Mode, 15 chord types, 8 strum patterns with advanced musical expression controls and pattern looping capabilities, while preserving all existing Scale Mode functionality.

**Version Identification:**
- Firmware version string: `1.2.5`
- Version characteristic reports correctly via BLE

---

## What's New

### Dual Keyboard Modes

The KB1 now supports two distinct operational modes, selectable via BLE:

#### **Scale Mode (Original)**
- Quantized note output with multiple scale types
- Chromatic, Major, Minor, Pentatonic, Blues, and more
- Natural/Compact key mapping options
- Perfect for melodic performance

#### **Chord Mode (NEW)**
- Chromatic chord/strum playback
- 15 professional chord types
- Polyphonic or cascaded note output
- Ideal for harmonic accompaniment and looping pattern sequences

### Chord Mode: Two Sub-Modes

Toggle between two distinct playback behaviors:

| Mode | Behavior | Best For |
|------|----------|----------|
| **Chord** | All chord notes play simultaneously | Rhythm guitar, pads, comping |
| **Strum** | Notes cascade with configurable timing | Guitar-style strumming, harp glissando, arpeggios |

### 15 Chord Types

Professional chord voicings spanning common to extended harmonies:

| Category | Chord Types |
|----------|-------------|
| **Triads** | Major, Minor, Diminished, Augmented, Sus2, Sus4, Power |
| **7th Chords** | Major7, Minor7, Dominant7 |
| **Extended** | Major Add9, Minor Add9, Major6, Minor6, Major9 |

All chords are voiced for musical balance and playability across the keyboard range.

### 8 Strum Patterns

Pattern 0 plays the selected chord type, while Patterns 1-7 enable **pattern looping mode** with smooth root note transitions:

| Pattern | Name | Description |
|---------|------|-------------|
| **0** | Chord | Uses selected chord type (simultaneous or cascaded) |
| **1** | Triad Up | Ascending 1-3-5 pattern |
| **2** | Triad Down | Descending 5-3-1 pattern |
| **3** | Pentatonic Run | 5-note scale-based pattern |
| **4** | Octave Jump | Root-octave alternation |
| **5** | Major 7th Run | 1-3-5-7 jazz voicing |
| **6** | Blues Run | Blues scale phrase |
| **7** | Custom | User-definable via BLE (modify note sequence) |

**Pattern Looping Mode (Patterns 1-7):**
- Monophonic looping playback that cycles through the pattern
- Smooth transitions when changing root notes mid-pattern
- Pattern continues from its current position when switching keys
- Enables melodic sequencing and automatic phrase generation

### Advanced Musical Expression Controls

#### Strum Speed
- **Range:** 4-120ms delay between cascaded notes
- **Effect:** Controls the "roll" speed from slow harp glissando to rapid guitar strum
- **Pattern Mode:** Sets the tempo of looping patterns (Patterns 1-7)

#### Velocity Spread
- **Range:** 0-100% velocity reduction for upper chord notes
- **Effect:** Creates natural dynamic contour in chords/strums
- **Musical Use:** 
  - 0% = all notes at equal velocity (mechanical)
  - 50% = gentle taper (natural guitar strum)
  - 100% = pronounced dynamic arc (expressive harp)

#### Strum Swing
- **Range:** 0-100% swing/groove amount
- **Effect:** Applies swing timing to odd-numbered notes in the pattern
- **Musical Use:**
  - 0% = straight timing (8th notes)
  - 50% = moderate shuffle (triplet feel)
  - 100% = heavy swing (dotted rhythms)
- **Note:** Only affects patterns with 3+ notes

### Preset System Integration

All Chord Mode settings are now part of the preset system:
- Mode selection (Scale/Chord)
- Chord/Strum toggle
- Selected chord type
- Active pattern (0-7)
- Strum speed, velocity spread, swing amount
- Custom pattern data (Pattern 7)

**Benefit:** Save complete performance configurations including keyboard mode, chord voicings, and pattern settings for instant recall.

---

## Web App Enhancements (KB1-config v1.2.5)

### Keyboard Settings Panel
- **Mode Selector:** Toggle between Scale Mode and Chord Mode
- **Chord/Strum Toggle:** Switch playback behavior in Chord Mode
- **Pattern Carousel:** Visual selector for 8 strum patterns with icons
- **Pattern Name Display:** Shows active pattern name (e.g., "Triad Up", "Blues Run")
- **Chord Type Dropdown:** Select from 15 chord types with visual representation
- **Expression Controls:** Real-time sliders for strum speed, velocity spread, and swing

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
- 8 pre-defined patterns with musical phrasing
- Custom pattern support (Pattern 7) with BLE configuration
- Velocity spread calculation for dynamic expression
- Swing timing interpolation for groove control

**Pattern Looping Logic** (`src/controls/KeyboardControl.h`)
- Monophonic looping with smooth root transitions
- Pattern position tracking across note changes
- Automatic note-off management for clean voice leading
- Tempo-locked timing based on strum speed

### Modified Files
- `src/controls/KeyboardControl.h` — Integrated chord logic, pattern looping engine, pattern playback
- `src/objects/Settings.h` — Added ChordSettings structure to PresetData
- `src/bt/CharacteristicCallbacks.cpp` — BLE handlers for chord/strum parameters
- `src/objects/Constants.h` — Version bump to `1.2.5`
- `src/main.cpp` — Chord mode initialization and state management

### BLE Characteristics (New)
- Keyboard Mode (Scale/Chord)
- Chord/Strum toggle
- Chord type selection
- Strum pattern selection (0-7)
- Strum speed (4-120ms)
- Velocity spread (0-100%)
- Strum swing (0-100%)
- Custom pattern data (Pattern 7 note array)

---

## Musical Use Cases

### Rhythm Guitar Accompaniment
- Chord Mode → Chord
- Select chord progressions (e.g., Major, Minor, Dominant7)
- Use velocity spread for natural strumming dynamics

### Harp-Style Glissando
- Chord Mode → Strum
- Strum speed: 20-40ms
- Velocity spread: 60-80%
- Pattern 0 with extended chord (Major9, Major7)

### Jazz Comping
- Chord Mode → Chord
- Extended voicings (Major7, Dominant7, Minor7)
- Quick strum speed (4-8ms) for tight attacks

### Melodic Sequencing (Pattern Looping)
- Chord Mode → Strum
- Pattern 1-7 (e.g., Triad Up, Pentatonic Run)
- Strum speed sets tempo (40-80ms for 8th note feel)
- Add swing for groove (50-70%)
- Play different root notes — pattern continues smoothly

### Blues Lead Lines
- Pattern 6: Blues Run
- Strum speed: 60-80ms
- Swing: 60-70%
- Change root notes for automatic blues phrasing

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
   - Adjust strum speed, velocity spread, and swing to taste

4. **Explore Presets:**
   - Save your favorite chord configurations as presets
   - Use community presets showcasing arpeggiator patterns

---

## Known Limitations

- **Arpeggiator Polyphony:** Patterns 1-7 are monophonic (one note at a time)
- **Pattern Length:** Fixed per pattern (cannot customize except Pattern 7)
- **Swing Resolution:** Applied to odd-numbered notes only (standard swing behavior)
- **Note-Off Timing:** Arpeggiator notes have fixed 80% note duration for clean articulation

---

## Credits

This release represents a significant evolution in the KB1's musical capabilities, transforming it from a scale-based controller into a versatile chord and arpeggiator powerhouse. Special thanks to the community for feedback on chord voicings and pattern design.

**Next Steps:** Explore the [Chord/Arp Planning Document](CHORD_ARP_PLANNING_v2.0.md) for detailed implementation notes and future enhancement ideas.
