# KB1 Firmware v1.7.1 Release Notes

**Release Date:** April 20, 2026  
**Focus:** Hardware-based musical expression with new MIDI CCs

---

## 🎼 New MIDI Expression Controls

This release adds three new **KB1 Expression** MIDI control change numbers that enable direct hardware control over musical parameters—allowing you to cycle through scales, chords, and root notes using levers, lever push buttons, or the touch sensor.

### New MIDI CCs

**CC 204: Scale Type (0-20)**
- Cycle through all 21 available scale types directly from hardware
- Supports Incremental (lever), Toggle (lever push/touch), and Continuous (touch) modes
- Discrete stepping with intelligent range mapping
- Range: 0-20 (Chromatic, Major, Minor, Natural Minor, Harmonic Minor, Melodic Minor, Dorian, Phrygian, Lydian, Mixolydian, Aeolian, Locrian, Major Pentatonic, Minor Pentatonic, Blues, Whole Tone, Diminished, Major Blues, Minor Blues, Hirajoshi, In)

**CC 205: Chord Type (0-14)**
- Cycle through all 15 chord types directly from hardware
- Supports Incremental (lever), Toggle (lever push/touch), and Continuous (touch) modes
- Discrete stepping with intelligent range mapping
- Range: 0-14 (Major, Minor, Diminished, Augmented, Sus2, Sus4, Power, Major7, Minor7, Dominant7, Major6, Minor6, Diminished7, Half-Diminished7, Augmented7)

**CC 206: Root Note (60-71)**
- Cycle through all 12 chromatic root notes (C through B)
- Supports Incremental (lever), Toggle (lever push/touch), and Continuous (touch) modes
- MIDI note values 60-71 (C4 through B4)
- Discrete stepping with intelligent range mapping

---

## 🎛️ Implementation Details

### Control Type Support

All three control types support the new MIDI CCs with specialized behavior:

**Lever Controls (Incremental Mode)**
- Left lever position: Step down (previous scale/chord/note)
- Right lever position: Step up (next scale/chord/note)
- Direct parameter updates without MIDI remapping
- Automatic wrapping at min/max boundaries

**Lever Push Controls (Toggle Mode)**
- Single press: Cycle to next value
- Forward/Reverse cycling based on `offsetTime` setting (0=forward, >0=reverse)
- Configurable min/max range constrains available values
- Automatic wrapping for seamless cycling

**Touch Sensor (Toggle/Continuous Mode)**
- **Toggle Mode**: Single touch cycles to next value with LED feedback (pink=forward, blue=reverse)
- **Continuous Mode**: Touch and hold to sweep through values
- Touch debounce: 50ms (Toggle) or 250ms (Hold/Continuous)
- Full MIDI CC output for DAW automation

### BLE Synchronization

- New `notifyScaleSettings()` method automatically syncs scale changes to web app
- Existing `notifyChordSettings()` extended to sync chord type changes
- Real-time bidirectional updates between hardware and web app
- No manual refresh required

---

## 🎹 Musical Applications

### Performance Workflows

**Live Scale Modulation**
- Assign CC 204 to a lever push button
- Quickly cycle through scales during performance
- Perfect for modal jazz, film scoring, or experimental music

**Chord Progression Building**
- Map CC 205 to lever incremental mode
- Push left/right to step through chord qualities
- Build progressions by moving through the keyboard with different chord types

**Key Center Control**
- Assign CC 206 to touch sensor toggle mode
- Tap to cycle through keys (C, C#, D, D#, etc.)
- Transpose compositions on the fly

**DAW Automation**
- All three CCs send standard MIDI CC messages
- Record parameter changes as automation in your DAW
- Use with external MIDI controllers via MIDI learn

---

## 📊 Technical Implementation

### Discrete Parameter Handling

The new CCs use **discrete stepping in parameter space** rather than MIDI space to ensure accurate control:

```
Scale Type Example:
- Hardware: Step from scale 5 → 6 (direct parameter increment)
- MIDI: Maps scale index (0-20) to CC range (minCCValue-maxCCValue)
- Result: Perfect 1:1 correspondence, no rounding errors
```

### Range Constraints

All three CCs support configurable min/max ranges:

```
Example: Limit Scale Type to Pentatonic scales only
- Min CC Value: 61 (maps to scale 13: Major Pentatonic)
- Max CC Value: 67 (maps to scale 14: Minor Pentatonic)
- Cycling: Wraps between these two scales only
```

### LED Feedback (Touch Sensor)

Touch sensor cycling provides directional LED feedback:
- **Pink LED**: Forward cycling (0 → max)
- **Blue LED**: Reverse cycling (max → 0)
- **300ms fade**: Smooth visual transition

---

## 🐛 Bug Fixes

- Fixed callback initialization for scale settings notifications
- Improved discrete parameter stepping to eliminate MIDI remapping errors
- Added ScaleManager forward declarations to prevent compilation issues

---

## 🔧 Compatibility

**Firmware:** v1.7.1  
**Web App:** KB1 Config v1.7.1+ (required for full CC 204/205/206 support)  
**Hardware:** All KB1 devices with firmware v1.7.0+  
**Backward Compatibility:** Full backward compatibility with v1.7.0 configurations

---

## 📝 Upgrade Notes

### For Users

1. Flash firmware v1.7.1 to your KB1 device
2. Update to KB1 Config web app v1.7.1
3. New CC options (204, 205, 206) will appear in control settings
4. Existing presets remain fully compatible

### For Developers

- New callback: `notifyScaleSettingsCallback()` must be initialized in main.cpp
- LeverControls, LeverPushControls, and TouchControl constructors now require `ScaleManager&` parameter
- Discrete parameter stepping logic available for reference in control headers

---

## 🎯 Known Limitations

- CC 204/205/206 are not part of standard MIDI CC—they are KB1 Expression controls
- External MIDI controllers sending these CCs will control device parameters (by design)
- Maximum parameter values: Scale Type=20, Chord Type=14, Root Note=71
- Continuous mode for CC 204/205/206 maps parameter space to touch pressure (works but toggling recommended)

---

## 📖 Related Documentation

- [KB1 Expression CCs](../KB1-config/README.md#kb1-expression-ccs) - Full parameter list
- [Control Configuration Guide](../KB1-config/README.md#control-surface) - Setup instructions
- [MIDI Implementation](MIDI_IMPLEMENTATION.md) - Complete MIDI CC reference

---

## 🙏 Credits

Special thanks to the KB1 community for feature requests and testing feedback!

**Next Release:** TBD  
**Report Issues:** [GitHub Issues](https://github.com/pocketmidi/kb1/issues)
