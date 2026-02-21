# KB1 Chord Mode & Arpeggiator Planning - Version 2.0

**Date:** February 21, 2026  
**Status:** Planning Phase  
**Target Version:** 2.0.0

---

## Executive Summary

This document outlines the implementation strategy for adding **Chord Mode** and **Arpeggiator Mode** to the KB1 keyboard controller. The approach prioritizes firmware-side processing to minimize Bluetooth overhead and maximize responsiveness.

---

## Core Strategy: Firmware-Side Implementation

The most efficient approach is to handle both chord and arpeggiator logic **in the firmware**. This minimizes BLE traffic to a one-time configuration send, then all the musical magic happens locally on the device.

### Key Benefits

| Aspect | This Design | Alternative (App-based) |
|--------|-------------|------------------------|
| **BLE Traffic** | 1 write to configure | Constant MIDI stream |
| **Latency** | None (local) | 20-50ms delay |
| **Battery Life** | Minimal impact | High drain |
| **Reliability** | Rock solid | Bluetooth drops = disaster |
| **Offline Use** | Works without app | Requires connection |
| **Preset Storage** | Auto-saved in PresetData | Complex sync |

---

## 1. CHORD MODE Implementation

### Overview
Chord mode allows a single key press to trigger multiple notes simultaneously, creating full chords based on the selected chord type.

### Firmware Changes

#### Settings.h Additions

```cpp
enum class ChordType : uint8_t {
    OFF = 0,           // No chording (current behavior)
    MAJOR = 1,         // Root + Major 3rd + Perfect 5th
    MINOR = 2,         // Root + Minor 3rd + Perfect 5th
    DIMINISHED = 3,    // Root + Minor 3rd + Tritone
    AUGMENTED = 4,     // Root + Major 3rd + Minor 6th
    SUS2 = 5,          // Root + Major 2nd + Perfect 5th
    SUS4 = 6,          // Root + Perfect 4th + Perfect 5th
    MAJOR7 = 7,        // Root + M3 + P5 + M7
    MINOR7 = 8,        // Root + m3 + P5 + m7
    DOMINANT7 = 9,     // Root + M3 + P5 + m7
    POWER = 10,        // Root + Perfect 5th (+ octave)
};

struct ChordSettings {
    ChordType chordType;        // Which chord voicing
    uint8_t velocitySpread;     // 0-127: amount to reduce velocity for upper notes
    bool arpMode;               // false = simultaneous, true = arpeggiate
};
```

#### KeyboardControl.h Modifications

When a key is pressed:
1. Calculate chord intervals based on `ChordType`
2. Send multiple `noteOn` messages (root, 3rd, 5th, 7th as needed)
3. Track all notes per key press for proper note-off handling

**Key Implementation Points:**
- Integrate with existing scale quantization
- Velocity spread reduces velocity for upper chord notes (natural expression)
- Each physical key tracks its associated chord notes for clean note-off

### Web App Changes

#### Configuration UI

```
┌─────────────────────────────────┐
│ 🎹 KEYBOARD MODE                │
├─────────────────────────────────┤
│ ┌─── CHORD SETTINGS ──────────┐│
│ │ Type: [Major ▾]             ││
│ │   • Off                     ││
│ │   • Major                   ││
│ │   • Minor                   ││
│ │   • Diminished              ││
│ │   • Augmented               ││
│ │   • Sus2 / Sus4             ││
│ │   • Major7 / Minor7 / Dom7  ││
│ │   • Power                   ││
│ │                             ││
│ │ Velocity Spread: [●────] 20 ││
│ │ (Reduces velocity for       ││
│ │  upper chord notes)         ││
│ └─────────────────────────────┘│
└─────────────────────────────────┘
```

#### BLE Protocol

```typescript
// In kb1Protocol.ts
export interface ChordModeSettings {
  chordType: number;           // 0-10 (ChordType enum)
  velocitySpread: number;      // 0-127
}

export function encodeChordMode(settings: ChordModeSettings): Uint8Array {
  const buffer = new Uint8Array(2);
  buffer[0] = settings.chordType;
  buffer[1] = settings.velocitySpread;
  return buffer;
}
```

### Implementation Advantages

✅ Single BLE write to change chord type  
✅ Instant response (no latency)  
✅ Works offline from the app  
✅ Integrates with existing scale quantization  
✅ Minimal memory footprint (~100 bytes)  

---

## 2. ARPEGGIATOR MODE Implementation

### Overview
Arpeggiator mode plays held notes sequentially in various patterns, with configurable timing and rhythm.

### Firmware Changes

#### Settings.h Additions

```cpp
enum class ArpPattern : uint8_t {
    OFF = 0,
    UP = 1,             // C-E-G-C
    DOWN = 2,           // C-G-E-C (reverse)
    UP_DOWN = 3,        // C-E-G-E
    DOWN_UP = 4,        // C-G-E-G
    RANDOM = 5,
    AS_PLAYED = 6,      // Order keys were pressed
};

struct ArpeggiatorSettings {
    ArpPattern pattern;
    uint16_t bpm;           // 20-300 BPM
    uint8_t division;       // Note division: 1=quarter, 2=eighth, 3=16th, 4=triplet
    uint8_t gateLength;     // 1-100% (how long note stays on)
    uint8_t octaveRange;    // 0-4: how many octaves to span
    bool holdMode;          // true = latch notes until clear
};
```

#### ArpeggiatorEngine Class (New)

**Key Responsibilities:**
- Uses FreeRTOS timer for precise timing based on BPM
- Maintains buffer of currently held keys
- Advances through pattern on each clock tick
- Sends note on/off at appropriate intervals
- Handles octave spanning (plays pattern across multiple octaves)
- Hold/latch mode keeps notes active after release

**Technical Details:**
- Uses `esp_timer` for microsecond accuracy
- Max polyphony: 8 held notes
- Memory estimate: 2-4KB for note buffer + state management
- Timer precision ensures solid timing even at 300 BPM

### Web App Changes

#### Configuration UI

```
┌─────────────────────────────────┐
│ 🎹 KEYBOARD MODE                │
├─────────────────────────────────┤
│ Play Mode: [Arpeggiator ▾]     │
│                                 │
│ ┌─── ARPEGGIATOR ─────────────┐│
│ │ Pattern: [Up ▾]             ││
│ │   • Off                     ││
│ │   • Up / Down               ││
│ │   • Up-Down / Down-Up       ││
│ │   • Random                  ││
│ │   • As Played               ││
│ │                             ││
│ │ BPM: [120] ●───────         ││
│ │ Division: [1/8▾]            ││
│ │   • 1/4 • 1/8 • 1/16        ││
│ │   • Triplets                ││
│ │                             ││
│ │ Gate Length: [75%] ●───     ││
│ │ Octave Range: [1] ●─────    ││
│ │                             ││
│ │ ☑ Hold Mode (Latch)        ││
│ └─────────────────────────────┘│
└─────────────────────────────────┘
```

#### BLE Protocol

```typescript
// In kb1Protocol.ts
export interface ArpeggiatorSettings {
  pattern: number;       // 0-6 (ArpPattern enum)
  bpm: number;          // 20-300
  division: number;     // 0-4
  gateLength: number;   // 1-100
  octaveRange: number;  // 0-4
  holdMode: boolean;
}

export function encodeArpeggiator(settings: ArpeggiatorSettings): Uint8Array {
  const buffer = new Uint8Array(8);
  buffer[0] = settings.pattern;
  buffer[1] = (settings.bpm >> 8) & 0xFF;  // High byte
  buffer[2] = settings.bpm & 0xFF;         // Low byte
  buffer[3] = settings.division;
  buffer[4] = settings.gateLength;
  buffer[5] = settings.octaveRange;
  buffer[6] = settings.holdMode ? 1 : 0;
  return buffer;
}
```

### Implementation Advantages

✅ Single BLE write to change all arp settings  
✅ Precise timing (hardware timers)  
✅ Can combine with chord mode (arpeggio chords!)  
✅ Battery-efficient (uses hardware timers)  
✅ Works completely offline once configured  

---

## 3. COMBINED MODE: Chord + Arpeggiator

One of the most powerful features is combining both modes:

**Example:**
- Chord Type = `MAJOR`
- Arp Pattern = `UP`
- Result: Plays C-major chord notes in sequence (C → E → G → C...)

This creates complex musical patterns from simple key presses, perfect for live performance and composition.

---

## 4. FEATURE INTERACTION MATRIX

| Feature Combo | Result | Implementation Notes |
|---------------|--------|---------------------|
| **Scale + Chord** | Chord notes quantized to scale | Chain quantization after chord generation |
| **Chord + Velocity Lever** | All chord notes use current velocity | Simple - already have `setVelocity()` |
| **Arp + Octave Buttons** | Shifts arp range in real-time | Requires note retrigger on octave change |
| **Arp + Touch Sensor** | Touch = gate/accent/filter | Map touch CC to arp gate % |
| **Compact Mode + Chord** | Dense chord voicings | Works automatically with existing logic |
| **Arp + Hold + Touch** | Touch toggles latch on/off | Natural performance control |

---

## 5. IMPLEMENTATION PRIORITY

### Phase 1: Chord Mode (Week 1)
**Estimated Time:** ~1 week

- **Firmware:** 2-3 days
  - Add `ChordSettings` struct
  - Modify `KeyboardControl` to handle chord generation
  - Test all chord types
  
- **Web UI:** 1 day
  - Add chord configuration section
  - Implement BLE characteristic
  
- **Testing:** 1 day
  - Verify all chord types
  - Test with scale quantization
  - Validate velocity spread

**Why first:** Simplest to implement, no timing concerns, instant user gratification

### Phase 2: Arpeggiator (Weeks 2-3)
**Estimated Time:** ~2 weeks

- **Firmware:** 5-7 days
  - Create `ArpeggiatorEngine` class
  - Implement timer system
  - Note buffer management
  - Pattern algorithms
  
- **Web UI:** 2 days
  - Add arpeggiator configuration
  - BPM/timing controls
  
- **Testing:** 2-3 days
  - Timing accuracy at BPM extremes
  - Buffer overflow handling
  - Pattern correctness

**Why second:** More complex, requires careful timer management and testing

### Phase 3: Integration & Polish (Week 4)
- Combined chord + arp mode testing
- Preset system integration
- Documentation
- User testing and feedback

**Total estimated time: 3-4 weeks**

---

## 6. TESTING STRATEGY

### Critical Test Scenarios

#### Chord Mode Tests
1. ✓ All chord types produce correct intervals
2. ✓ Velocity spread works correctly (0-127 range)
3. ✓ Scale quantization + chords = correct notes
4. ✓ Note off clears all chord notes
5. ✓ Multiple simultaneous chord keys

#### Arpeggiator Tests
1. ✓ **Buffer overflow**: Hold 10+ keys → graceful handling
2. ✓ **BPM extremes**: 20 BPM (very slow) and 300 BPM (very fast)
3. ✓ **Preset switching**: Arp running → load preset → clean reset
4. ✓ **Sleep/wake**: Arp active → device sleeps → stops notes properly
5. ✓ **Scale changes**: Mid-arp scale change → re-quantize behavior
6. ✓ **Pattern accuracy**: Each pattern produces correct note order
7. ✓ **Gate timing**: 1% to 100% gate produces expected durations
8. ✓ **Hold mode**: Notes latch correctly, clear function works

#### Integration Tests
1. ✓ Chord + Arp combination produces expected results
2. ✓ Touch sensor interactions work smoothly
3. ✓ Octave changes during arp operation
4. ✓ BLE disconnection during arp (should continue)
5. ✓ Preset recall with chord/arp settings

---

## 7. BACKWARD COMPATIBILITY

### Version Strategy

```cpp
// In firmware
#define FIRMWARE_VERSION_MAJOR 2
#define FIRMWARE_VERSION_MINOR 0
#define PROTOCOL_VERSION 2  // Bump when adding chord/arp features
```

### Compatibility Matrix

| Firmware | Web App | Result |
|----------|---------|--------|
| v1.x | v1.x | Works normally (current behavior) |
| v2.x | v1.x | Works, advanced features unavailable |
| v1.x | v2.x | App detects old firmware, hides new features |
| v2.x | v2.x | Full feature set available |

**Implementation:**
- Old apps see chord/arp settings as "unknown" but device still functions
- New apps detect firmware version and conditionally show features
- Graceful degradation ensures no breaking changes

---

## 8. ALTERNATIVE APPROACHES (NOT RECOMMENDED)

### Option A: Hybrid Processing
- **Chord logic in firmware** (low latency)
- **Arp clock in app** (easier BPM sync with DAW)
- **Tradeoff**: Requires constant BLE connection for arp
- **Verdict:** ❌ Defeats the purpose of autonomous operation

### Option B: Preset-Based Chords
- Define 8 custom chord voicings in presets
- Use lever to morph between them
- **Tradeoff:** More flexible but complex UI
- **Verdict:** 🤔 Consider for v2.1 as "advanced mode"

### Option C: "Chord Memory" Mode
- Press multiple keys → device stores as custom chord
- Single key plays that chord back
- **Tradeoff:** Great for unique voicings/inversions
- **Verdict:** ⭐ Excellent v2.2 feature!

---

## 9. FUTURE EXTENSIBILITY (v2.1+)

### Roadmap

#### v2.0: Core Implementation
- Basic chord types (10 types)
- Basic arp patterns (6 patterns)
- BPM/timing control
- Hold/latch mode

#### v2.1: Refinements
- **Chord inversions** (1st, 2nd, 3rd inversions)
- **Arp swing** (groove/shuffle timing)
- **Velocity ramp** (crescendo/diminuendo patterns)

#### v2.2: Advanced Features
- **Custom chord editor** in web app
- **User-definable arp patterns**
- **Chord memory** mode (record custom chords)

#### v2.3: External Sync
- **MIDI clock sync** (external tempo source)
- **Tap tempo** (via touch sensor multiple taps)
- **BPM lock to DAW**

#### v2.4: Sequencer Mode
- **Step sequencer** (8-16 step patterns)
- **Pattern chaining**
- **Euclidean rhythms**

### Data Structure Future-Proofing

```cpp
// Expandable structure with reserved bytes
struct AdvancedPlayMode {
    ChordSettings chord;
    ArpeggiatorSettings arp;
    uint8_t mpeEnabled;        // For future MPE support
    uint8_t perNoteCC;         // Polyphonic expression
    uint8_t reserved[4];       // Future expansion without breaking changes
};
```

---

## 10. REAL-WORLD USE CASES

### Live Performance
- **Quick chord changes** between songs
- **Arp sync** to backing tracks
- **Hold mode** for hands-free loops
- **Touch sensor** for dynamic expression

**Example Workflow:**
1. Load "Verse" preset (Minor chords, slow arp)
2. Play melody line with chord backing
3. Switch to "Chorus" preset (Major chords, fast arp)
4. Toggle hold mode via touch for sustained pads

### Practice & Learning
- **Display active notes** in web app (chord visualization)
- **Scale + Chord = Smart chords** (only in-scale chord types)
- **Arp for ear training** (identify intervals)

**Example:**
- Set scale to C Major
- Chord mode automatically limits to I, IV, V chords
- Perfect for learning progressions

### Composition & Sketching
- **Arp + Hold = Loop machine** for layering ideas
- **Record MIDI** via browser MIDI API
- **Preset per section** (verse/chorus/bridge different settings)

**Workflow:**
1. Enable arp + hold mode
2. Play progression (notes latch)
3. Record output to DAW via MIDI
4. Layer multiple passes

---

## 11. COMPETITIVE ANALYSIS

### Comparable Devices

#### Arturia Keystep
- Simple arp with hold/latch ✓
- Fixed chord types (limited)
- MIDI clock sync ✓
- **KB1 Advantage:** Scale-aware chords, wireless control

#### Novation Launchpad
- Chord mode in scale mode ✓
- Grid-based interface
- No arpeggiator on basic notes
- **KB1 Advantage:** Traditional keyboard layout, arp+chord combo

#### ROLI Seaboard
- Arp implementation over MPE ✓
- Per-note expression
- Expensive ($800+)
- **KB1 Advantage:** Affordable, simpler interface

#### Teenage Engineering OP-1
- Compact arp patterns ✓
- Built-in sequencer
- Limited connectivity
- **KB1 Advantage:** Full wireless control, easier parameter access

### KB1 Unique Selling Points

✅ **Lever-controlled velocity per chord note** (no other device has this)  
✅ **Touch-controlled arp gate** (dynamic performance control)  
✅ **Scale-aware chords** (intelligent music theory implementation)  
✅ **Full wireless control** over all parameters via elegant web UI  
✅ **Preset system** with instant recall  
✅ **Open source** (community can extend)  

---

## 12. QUESTIONS FOR IMPLEMENTATION PHASE

When ready to implement, decide on:

### 1. Preset Integration
**Option A: Global settings**
- Chord/arp settings apply to all presets
- Simpler, saves memory
- Less flexible

**Option B: Per-preset settings** ⭐ RECOMMENDED
- Each preset stores its own chord/arp config
- More powerful (different modes per song section)
- Adds ~10 bytes to `PresetData` struct
- Better user experience

### 2. Arp Sync Source
**Phase 1: Internal only** ⭐ START HERE
- BPM setting in firmware
- Simplest implementation
- Sufficient for 90% of use cases

**Phase 2: Multiple sources** (v2.3)
- MIDI clock in (requires hardware)
- App sends clock (requires connection)
- Tap tempo via touch sensor

### 3. UI Organization
**Option A: New "Playback" tab**
- Dedicated space for chord/arp
- Clearer organization
- More clicks to access

**Option B: Expand "Keys" tab** ⭐ RECOMMENDED
- All keyboard settings in one place
- Fewer navigation clicks
- Slightly more crowded

**Option C: Quick-access overlay**
- Floating control panel
- Always accessible
- More complex UI state

### 4. Hold Mode Clear Function
**Trigger Options:**
- Touch sensor double-tap ⭐ RECOMMENDED
- Octave down + octave up simultaneously
- Dedicated button in web app
- Long-press any key

### 5. Visual Feedback (LEDs)
**Arp running state:**
- Pulse LED on each note
- Solid color when armed
- Blink on BPM clock
- **Recommendation:** Subtle pulse, not distracting

---

## 13. MEMORY & PERFORMANCE BUDGET

### Resource Estimates

#### Chord Mode
- **RAM:** ~100 bytes (settings + state)
- **Flash:** ~2 KB (code)
- **CPU:** Negligible (only on key events)
- **Impact:** ✅ Minimal

#### Arpeggiator
- **RAM:** 2-4 KB (note buffer + timer state)
- **Flash:** ~8 KB (code + pattern logic)
- **CPU:** ~1-2% (timer interrupts at 300 BPM)
- **Impact:** ✅ Well within ESP32-S3 capabilities

#### Total Added Resources
- **RAM:** ~4-5 KB
- **Flash:** ~10 KB
- **CPU:** <2% average
- **Battery:** <1% additional drain

**ESP32-S3 Available:**
- 512 KB RAM (plenty of headroom)
- 8 MB Flash (more than enough)
- 240 MHz dual-core CPU (barely touched)

### Optimization Strategies

1. **Note buffer:** Use circular buffer, max 8 notes
2. **Timer:** Use hardware timers (zero CPU when idle)
3. **Pattern lookup:** Pre-calculated tables vs. runtime computation
4. **Memory pooling:** Reuse allocated buffers

---

## 14. TECHNICAL IMPLEMENTATION DETAILS

### Chord Generation Algorithm

```cpp
// Pseudo-code
void KeyboardControl::generateChord(int rootNote, ChordType type) {
    std::vector<int> intervals;
    
    switch(type) {
        case ChordType::MAJOR:
            intervals = {0, 4, 7};  // Root, M3, P5
            break;
        case ChordType::MINOR:
            intervals = {0, 3, 7};  // Root, m3, P5
            break;
        case ChordType::MAJOR7:
            intervals = {0, 4, 7, 11};  // Root, M3, P5, M7
            break;
        // ... other types
    }
    
    int baseVelocity = getCurrentVelocity();
    for (int i = 0; i < intervals.size(); i++) {
        int note = rootNote + intervals[i];
        int velocity = baseVelocity - (velocitySpread * i / intervals.size());
        sendNoteOn(note, velocity);
        trackChordNote(rootNote, note);  // For proper note-off
    }
}
```

### Arpeggiator Timer Setup

```cpp
// Pseudo-code using ESP32 timer
void ArpEngine::start() {
    // Calculate interval in microseconds from BPM
    uint32_t intervalUs = (60.0 / bpm) * 1000000.0 / division;
    
    esp_timer_create_args_t timerArgs = {
        .callback = &arpTimerCallback,
        .arg = this,
        .name = "arp_clock"
    };
    
    esp_timer_create(&timerArgs, &arpTimer);
    esp_timer_start_periodic(arpTimer, intervalUs);
}

void ArpEngine::arpTimerCallback(void* arg) {
    ArpEngine* engine = (ArpEngine*)arg;
    engine->advancePattern();
}

void ArpEngine::advancePattern() {
    // Get next note based on pattern
    int nextNote = getNextNoteInPattern();
    
    // Stop previous note (if gate time elapsed)
    if (noteIsPlaying && gateTimeElapsed()) {
        sendNoteOff(currentNote);
    }
    
    // Start next note
    sendNoteOn(nextNote, velocity);
    currentNote = nextNote;
    noteIsPlaying = true;
}
```

---

## 15. DOCUMENTATION REQUIREMENTS

### User Documentation
- [ ] Feature overview page
- [ ] Chord type reference chart
- [ ] Arp pattern demonstrations (video/audio)
- [ ] Example use cases
- [ ] Troubleshooting guide

### Developer Documentation
- [ ] API reference for new BLE characteristics
- [ ] Code architecture diagrams
- [ ] Timer system documentation
- [ ] Testing procedures
- [ ] Contributing guidelines for new chord types/patterns

### In-App Help
- [ ] Tooltip explanations for each setting
- [ ] Interactive tutorial for first-time users
- [ ] Preset examples showcasing features

---

## 16. SUCCESS METRICS

### Performance Targets
- Arp timing accuracy: ±1ms at all BPM settings
- Chord latency: <5ms from key press to all notes playing
- BLE configuration time: <50ms for full settings update
- Battery impact: <1% additional drain with arp running

### User Experience Goals
- 90% of users understand chord mode in <5 minutes
- 70% of users successfully configure arpeggiator in <10 minutes
- Zero reports of timing drift after 1 hour operation
- <5% bug reports related to note stuck issues

---

## 17. RISK ASSESSMENT

### Technical Risks

| Risk | Probability | Impact | Mitigation |
|------|-------------|---------|-----------|
| Timer drift at extreme BPM | Low | Medium | Use hardware timers, extensive testing |
| Note buffer overflow | Medium | High | Implement max polyphony limit (8 notes) |
| Sleep/wake state issues | Medium | Medium | Clear all notes on sleep entry |
| Preset switch timing bug | Low | Medium | Reset arp engine on preset load |
| BLE packet loss during config | Low | Low | Add checksum/retry logic |

### User Experience Risks

| Risk | Probability | Impact | Mitigation |
|------|-------------|---------|-----------|
| UI too complex | Medium | High | User testing, iterative design |
| Chord types not intuitive | Low | Medium | Clear labeling, tooltips |
| Arp patterns confusing | Medium | Medium | Visual/audio examples |
| Hold mode unclear | High | Low | Clear toggle indicator, docs |

---

## APPENDIX A: Chord Type Interval Reference

| Chord Type | Intervals | Semitones | Example (C) |
|------------|-----------|-----------|-------------|
| Major | R, M3, P5 | 0, 4, 7 | C, E, G |
| Minor | R, m3, P5 | 0, 3, 7 | C, Eb, G |
| Diminished | R, m3, d5 | 0, 3, 6 | C, Eb, Gb |
| Augmented | R, M3, A5 | 0, 4, 8 | C, E, G# |
| Sus2 | R, M2, P5 | 0, 2, 7 | C, D, G |
| Sus4 | R, P4, P5 | 0, 5, 7 | C, F, G |
| Major7 | R, M3, P5, M7 | 0, 4, 7, 11 | C, E, G, B |
| Minor7 | R, m3, P5, m7 | 0, 3, 7, 10 | C, Eb, G, Bb |
| Dominant7 | R, M3, P5, m7 | 0, 4, 7, 10 | C, E, G, Bb |
| Power | R, P5, R+8ve | 0, 7, 12 | C, G, C |

---

## APPENDIX B: Arpeggiator Pattern Examples

### Pattern: UP
**Notes held:** C, E, G  
**Output:** C → E → G → C → E → G → ...

### Pattern: DOWN
**Notes held:** C, E, G  
**Output:** G → E → C → G → E → C → ...

### Pattern: UP_DOWN
**Notes held:** C, E, G  
**Output:** C → E → G → E → C → E → G → ...

### Pattern: DOWN_UP
**Notes held:** C, E, G  
**Output:** G → E → C → E → G → E → C → ...

### Pattern: RANDOM
**Notes held:** C, E, G  
**Output:** E → C → G → C → E → G → ... (randomized each step)

### Pattern: AS_PLAYED
**Notes held in order:** E, C, G  
**Output:** E → C → G → E → C → G → ... (preserves input order)

---

## APPENDIX C: BLE Characteristic Reference

### Chord Mode Characteristic
- **UUID:** TBD (e.g., `d3a7b321-0001-4000-8000-00000000000d`)
- **Type:** Write
- **Size:** 2 bytes
- **Format:** `[chordType][velocitySpread]`

### Arpeggiator Characteristic
- **UUID:** TBD (e.g., `d3a7b321-0001-4000-8000-00000000000e`)
- **Type:** Write
- **Size:** 8 bytes
- **Format:** `[pattern][bpm_hi][bpm_lo][division][gate][octaves][hold][reserved]`

### Combined Mode Characteristic (Optional)
- **UUID:** TBD
- **Type:** Write
- **Size:** 10 bytes
- **Format:** Chord (2 bytes) + Arp (8 bytes)

---

## APPENDIX D: Recommended Development Tools

### Firmware Development
- PlatformIO (already in use)
- Logic analyzer for timer verification
- MIDI monitor for output verification
- Serial plotter for debugging timing

### Web App Development
- Chrome DevTools (BLE debugging)
- Web MIDI API browser extension
- Audio/MIDI recording for pattern verification

### Testing Tools
- Automated MIDI test suite
- BPM accuracy measurement script
- Battery drain profiler
- User testing feedback forms

---

## CONCLUSION

This comprehensive plan provides a solid foundation for implementing Chord Mode and Arpeggiator functionality in the KB1 v2.0 firmware and web application. The approach prioritizes:

1. ✅ **Minimal BLE overhead** - Configuration, not streaming
2. ✅ **Maximum reliability** - Firmware-side processing
3. ✅ **User-friendly UX** - Intuitive controls and presets
4. ✅ **Future extensibility** - Clear roadmap for v2.1+
5. ✅ **Comprehensive testing** - Robust quality assurance

The phased implementation approach (Chord Mode first, then Arpeggiator) allows for iterative development and user feedback while maintaining a manageable scope for each milestone.

**Ready for implementation when you are!** 🚀

---

**Document Version:** 1.0  
**Last Updated:** February 21, 2026  
**Next Review:** Upon v2.0 implementation kickoff
