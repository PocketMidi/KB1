# KB1 Firmware v1.5.0 Release Notes
**Release Date:** March 31, 2026

## 🎯 What's New

### Non-Blocking Strum System
- **Monophonic strum behavior** - New chord/strum now interrupts previous one
- **Non-blocking cascade** - Strum notes play over time without blocking main loop
- **Improved responsiveness** - No more delay() calls blocking input processing
- **Consistent behavior** - Both chord and strum modes now operate monophonically
- **Immediate interruption** - Starting new strum stops previous cascade instantly

**Technical Details:**
- Strum notes cascade using time-based state machine (updateStrum)
- First note plays immediately on key press, subsequent notes scheduled
- Key release stops all notes from the chord/strum
- No more blocking delays in hotpath

### Voicing Control (Octave Expansion)
- **1-3 octave voicing** - Expand chords across multiple octaves
- **Root-forward voicing** - Chord pattern repeats at +12, +24 semitones
- **Up to 15 notes** - 5-note chord × 3 octaves = massive polyphonic spread
- **MAX_CHORD_NOTES increased** from 4 to 16 to support expanded voicings
- **Configurable via web app** - Voicing slider in Chord & Strum settings

**Example:**
- Major chord (C-E-G) with voicing=1: plays C4-E4-G4 (3 notes)
- Major chord (C-E-G) with voicing=2: plays C4-E4-G4-C5-E5-G5 (6 notes)
- Major chord (C-E-G) with voicing=3: plays C4-E4-G4-C5-E5-G5-C6-E6-G6 (9 notes)

### USB State Persistence
- **Deep sleep/wake support** - USB connection state persists across sleep cycles
- **Fresh boot detection** - Distinguishes fresh power-on from wake-from-sleep
- **Proper charging detection** - Bypass mode flag preserved across reboots
- **NVS storage** - lastUsbState and usbConnectedAtBoot saved to flash

**Impact:**
- Sleep/wake cycles no longer lose bypass mode state
- Charging detection more reliable after wake
- Battery monitoring continues correctly after deep sleep

### Default Value Tuning
- **Velocity Spread minimum** increased from 8% to 10% for better musical range
- **Strum Speed default** changed from 30ms to 80ms (moderate-fast)
- **Touch Threshold** increased from 24000 to 36800 (20% sensitivity)
- **Touch Minimum** lowered to 30000 (from 24000) for better dynamic range

### Improved Strum Timing
- **Swing calculation** now uses absolute value of strum speed
- **Negative speed = reverse** - Strum direction controlled by speed sign
- **Consistent swing behavior** regardless of strum direction

## 🔧 Technical Changes

### Architecture
- Added `esp_system.h` include for reset reason detection
- Expanded ChordSettings struct with voicing field
- Added strum state tracking (_strumActive, _strumInProgress, etc.)
- Implemented updateStrum() for non-blocking cascade playback

### Persistence
- Battery state saves lastUsbState and usbConnectedAtBoot to NVS
- Fresh boot clears USB state flags for proper detection
- Deep sleep wake restores flags from NVS

### Code Quality
- Removed all blocking delay() calls from strum pathcod
- Time-based state machines for both basic strum and arpeggiator
- Better separation of concerns (strum vs arpeggiator logic)

## 📊 Performance Impact

**Improved:**
- No blocking delays in input processing (strum now non-blocking)
- Chord/strum key presses more responsive
- Main loop runs consistently without interruption

**Neutral:**
- Voicing expansion adds minimal CPU overhead (just interval calculation)
- USB persistence adds <1ms to boot time (NVS read)

## 🎵 Musical Impact

**Voicing Feature:**
- Create lush, rich chord arrangements with 1-3 octave spread
- Useful for cinematic pads, orchestral arrangements
- Increases polyphonic chord count up to 15 simultaneous notes

**Monophonic Behavior:**
- More predictable for live performance (no overlapping chords)
- Matches expected behavior of physical instruments
- Cleaner sound (previous chord stops when new one starts)

## 🔄 Migration Notes

**From v1.4.x:**
- Chord/strum behavior now monophonic (previous chord stops)
- Velocity spread minimum raised to 10% (was 8%)
- Strum speed default changed to 80ms (was 30ms)
- Touch threshold changed to 36800 (was 24000)
- Existing presets will work but may sound slightly different

**Settings Affected:**
- Chord Settings: Added voicing field (defaults to 1)
- Touch Settings: threshold changed from 24000 to 36800
- Chord Settings: velocitySpread minimum now 10% (was 8%)
- Chord Settings: strumSpeed default now 80ms (was 30ms)

## 📝 Known Limitations

- Voicing only applies to chord mode (not individual scale notes)
- Strum pattern direction controlled by speed sign (not separate setting)
- Maximum 16 simultaneous chord notes (hardware MIDI limit)

## 🚀 Upgrade Instructions

1. **Flash firmware**: Use combined binary `KB1-firmware-v1.5.0.bin`
2. **Update web app**: Rebuild or deploy latest KB1-config
3. **Test functionality**: Verify voicing slider in Chord & Strum settings
4. **Adjust defaults**: May need to tweak touch threshold or strum speed for your preference

---

**Firmware**: v1.5.0  
**Web App**: v1.5.0  
**Compatibility**: ESP32-S3 (Seeed XIAO)  
**Flash Size**: 8MB (max_app_8MB.csv partition scheme)
