---
name: kb1-pitch-bend
description: Complete implementation reference for KB1 fake/MIDI-note-based pitch bend (CC 208). Use when rebuilding, debugging, or extending the pitch bend feature. Contains full firmware + web app implementation across all affected files.
---

# KB1 Fake Pitch Bend — Complete Implementation Reference

KB1 does not send standard MIDI Pitch Bend messages (0xE0). Instead it uses **MIDI note retrigger with semitone offset** — held notes are retriggered a semitone up or down when the lever moves past a threshold. This works with sequencers, loopers, and any device that records MIDI notes. The effect is identical to pitch bend from the receiving instrument's perspective.

---

## Design Summary

- **CC 208** — reserved KB1-internal CC number for Pitch Bend (not a real MIDI CC)
- Lever pushed LEFT → semitone offset = −1 (or −2 depending on range)
- Lever pushed RIGHT → semitone offset = +1 (or +2)
- Lever released → semitone offset returns to 0, notes retrigger at original pitch
- Retrigger = NoteOn new note → schedule delayed NoteOff for old note (overlap creates smooth crossfade)
- Overlap duration = `onsetTime` field (ms), configurable 0–100ms via web app UI, default 25ms
- **Pitch bend is suppressed while Sustain is active** (CC 209 / SUSTAIN mode)
- Only active in **SCALE mode** (not Chord/ARP)

---

## Firmware — `src/objects/Globals.h`

### LeverFunctionMode enum
```cpp
enum class LeverFunctionMode {
    INTERPOLATED,
    PEAK_AND_DECAY,
    INCREMENTAL,
    PITCH_BEND,  // Lever morphs held MIDI notes ±2 semitones (recorded by sequencers)
};
```
`PITCH_BEND = 3` (zero-indexed).

---

## Firmware — `src/controls/LeverControls.h`

### `updateTargetValue()` — lever input → target value
```cpp
} else if (_settings.functionMode == LeverFunctionMode::PITCH_BEND || _settings.ccNumber == 208) {
    // PITCH_BEND: left=min semitone, right=max semitone, center=0 on release
    if (leftState) {
        _targetValue = _settings.minCCValue;
        _isPressed = true;
    } else if (rightState) {
        _targetValue = _settings.maxCCValue;
        _isPressed = true;
    } else {
        if (_isPressed) {
            _targetValue = 64; // Always return to semitone 0 (center)
        }
        _isPressed = false;
    }
}
```
`_targetValue = 64` = MIDI center = 0 semitone offset (no bend).

### `updateValue()` — convert target value → semitone offset → retrigger
```cpp
if (_settings.functionMode == LeverFunctionMode::PITCH_BEND || _settings.ccNumber == 208) {
    // Bypass interpolation — map _targetValue directly to semitone offset
    if (_targetValue == _lastSentValue) return;

    auto midiToCents = [](int midi) -> int {
        return round((midi / 127.0f) * 200.0f) - 100;
    };

    int minCents = midiToCents(_settings.minCCValue);
    int maxCents = midiToCents(_settings.maxCCValue);

    float normalizedPosition = 0.0;
    if (_settings.maxCCValue > _settings.minCCValue) {
        normalizedPosition = (float)(_targetValue - _settings.minCCValue) /
                            (float)(_settings.maxCCValue - _settings.minCCValue);
        normalizedPosition = constrain(normalizedPosition, 0.0, 1.0);
    }

    float mappedCents = minCents + (normalizedPosition * (maxCents - minCents));
    int snappedCents = (int)round(mappedCents / 10.0f) * 10;
    snappedCents = constrain(snappedCents, minCents, maxCents);

    // Retrigger threshold: ±50 cents = 1 semitone step
    int semitoneOffset = (snappedCents >= 50) ? 1 : (snappedCents <= -50) ? -1 : 0;

    // onsetTime (ms) = note overlap duration (default 25ms)
    unsigned long overlapUs = (_settings.onsetTime > 0)
        ? (unsigned long)_settings.onsetTime * 1000UL
        : 25000UL;

    _keyboardControl.setPitchBendOverlapUs(overlapUs);
    _keyboardControl.setPitchBendOffset(semitoneOffset);
    _currentValue = _targetValue;
    _lastSentValue = _targetValue;
    return;
}
```

**Key points:**
- No interpolation ramp — pitch bend bypasses the whole ramp engine
- `_settings.minCCValue` / `_settings.maxCCValue` stored as MIDI 0–127, converted to cents internally
- Semitone snapping threshold: ±50 cents → ±1 semitone
- `setPitchBendOverlapUs()` and `setPitchBendOffset()` are called every lever update

---

## Firmware — `src/controls/KeyboardControl.h`

### Member variables (in constructor init list)
```cpp
_pitchBendOffset(0),
_pitchBendOverlapUs(25000),  // default 25ms
```

### Private member declarations (near bottom of class)
```cpp
int _pitchBendOffset;
unsigned long _pitchBendOverlapUs;

// Pending NoteOff queue for pitch bend overlap
static constexpr int MAX_PB_PENDING_OFFS = 24;
byte _pbPendingOffNote[MAX_PB_PENDING_OFFS];
unsigned long _pbPendingOffDueUs[MAX_PB_PENDING_OFFS];
bool _pbPendingOffActive[MAX_PB_PENDING_OFFS];
```

### `setPitchBendOverlapUs()` — API called by LeverControls
```cpp
void setPitchBendOverlapUs(unsigned long us) { _pitchBendOverlapUs = us; }
```

### `setPitchBendOffset()` — main retrigger logic
```cpp
void setPitchBendOffset(int semitones) {
    // Sustain mode freezes pitch bend to avoid accidental lever movement side effects.
    if (_sustainActive) return;

    semitones = constrain(semitones, -2, 2);
    if (semitones == _pitchBendOffset) return;

    int oldOffset = _pitchBendOffset;
    _pitchBendOffset = semitones;

    // Retrigger held SCALE notes (NoteOn before NoteOff = smooth crossfade overlap)
    if (_chordSettings.playMode == PlayMode::SCALE) {
        for (int i = 0; i < 128; i++) {
            if (_isNoteOn[i]) {
                int quantized = _baseNote[i]; // stored pre-offset note (critical for compact mode)
                int oldNote = constrain(quantized + oldOffset, 0, 127);
                int newNote = constrain(quantized + semitones, 0, 127);
                if (oldNote != newNote) {
                    _midi.sendNoteOn(newNote, _currentVelocity, 1);
                    schedulePitchBendNoteOff(oldNote);
                }
            }
        }
    }
}
```

### `schedulePitchBendNoteOff()` — delayed NoteOff queue
```cpp
void schedulePitchBendNoteOff(byte note, unsigned long delayUs = 0) {
    // Uses _pitchBendOverlapUs if delayUs not specified
    unsigned long due = micros() + (delayUs > 0 ? delayUs : _pitchBendOverlapUs);
    for (int i = 0; i < MAX_PB_PENDING_OFFS; i++) {
        if (!_pbPendingOffActive[i]) {
            _pbPendingOffNote[i] = note;
            _pbPendingOffDueUs[i] = due;
            _pbPendingOffActive[i] = true;
            return;
        }
    }
    // Queue full — send immediately to avoid stuck note
    _midi.sendNoteOff(note, 0, 1);
}
```

### `processPendingPitchBendNoteOffs()` — called every keyboard tick
```cpp
void processPendingPitchBendNoteOffs() {
    unsigned long now = micros();
    for (int i = 0; i < MAX_PB_PENDING_OFFS; i++) {
        if (_pbPendingOffActive[i] && now >= _pbPendingOffDueUs[i]) {
            _midi.sendNoteOff(_pbPendingOffNote[i], 0, 1);
            _pbPendingOffActive[i] = false;
        }
    }
}
```
Called from `updateKeyboardState()` at the top of every loop.

### `_baseNote[]` array — critical for Compact mode
```cpp
byte _baseNote[128]; // stores pre-offset quantized note at key press time
```
Set in `playMidiNote()`:
```cpp
_baseNote[note] = quantizedNote; // Store pre-offset note for pitch bend retrigger
```
Read in `stopMidiNote()` and `setPitchBendOffset()`:
```cpp
int quantized = _baseNote[i]; // Use stored note — correct for both natural and compact mode
int bentNote = constrain(quantized + _pitchBendOffset, 0, 127);
```
**Without `_baseNote[]`, compact mode produces stuck notes** — the retrigger would compute the wrong note on release because the quantized value changes between press and release when the lever is held.

### `playMidiNote()` — NoteOn path applies bend
```cpp
int bentNote = constrain(quantizedNote + _pitchBendOffset, 0, 127);
_midi.sendNoteOn(bentNote, _currentVelocity, channel);
_isNoteOn[note] = true;
_baseNote[note] = quantizedNote; // Store pre-offset note for pitch bend retrigger
```

### `stopMidiNote()` — NoteOff path uses stored base note
```cpp
int bentNote = constrain(quantizedNote + _pitchBendOffset, 0, 127);
// (quantizedNote computed the same way as playMidiNote, including compact mode logic)
_midi.sendNoteOff(bentNote, 0, channel);
_isNoteOn[note] = false;
```

---

## Web App — `src/data/ccMap.ts`

CC 208 is added to the **hardcoded KB1 Expression group** (appended after CSV parse):
```typescript
const kb1ExpressionGroup: CCGroup = {
  category: 'KB1 Expression',
  entries: [
    {
      ccNumber: 208,
      parameter: 'Pitch Bend',
      category: 'KB1 Expression',
      range: { min: -2, max: 2, text: '-2 to 2' },
    },
    // ... other KB1 Expression entries
  ]
}
```
CC 208 is listed **lever-only** — it is filtered out of press control (LeverPushSettings) options:
```typescript
// In LeverPushSettings filteredOptions:
options = options.filter(opt => {
  const cc = opt.value
  return cc !== 200 && cc !== 202 && cc !== 203 && cc !== 207 && cc !== 208 // 208 = lever only
})
```

---

## Web App — `src/ble/kb1Protocol.ts`

### Enum
```typescript
export enum LeverFunctionMode {
  INTERPOLATED = 0,
  PEAK_DECAY = 1,
  INCREMENTAL = 2,
  PITCH_BEND = 3,  // Lever morphs held notes ±2 semitones
}
```

### Default settings for lever1
```typescript
lever1: {
  ccNumber: 208,
  minCCValue: 0,
  maxCCValue: 127,
  stepSize: 1,
  functionMode: 3, // PITCH_BEND
  valueMode: ValueMode.BIPOLAR,
  onsetTime: 25,   // 25ms overlap duration
  offsetTime: 25,
  onsetType: InterpolationType.LINEAR,
  offsetType: InterpolationType.LINEAR,
},
```

### Validator — CC 208 is valid for levers
```typescript
const ccValid = lever.ccNumber === -1 ||
                (lever.ccNumber >= 0 && lever.ccNumber <= 128) ||
                (lever.ccNumber >= 200 && lever.ccNumber <= 208); // 208 = Pitch Bend
```

---

## Web App — `src/components/LeverSettings.vue`

### `isKB1Expression` computed — CC 208 included
```typescript
const isKB1Expression = computed(() => {
  const cc = model.value.ccNumber
  return cc === 200 || cc === 201 || cc === 202 || cc === 203 || cc === 204 || cc === 205 || cc === 208
})
```

### `isPitchBendMode` computed — dual guard
```typescript
// Key off CC 208 in case functionMode is stale from older presets
const isPitchBendMode = computed(() => model.value.functionMode === 3 || model.value.ccNumber === 208)
```

### UI behavior when `isPitchBendMode = true`
- **UNI|BI toggle**: disabled (always stays Bipolar)
- **Lin, Log, P&D profile buttons**: disabled
- **Exp profile button**: shows as active (visual only, can't change)
- **Inc profile button**: disabled
- **Level meter**: hidden (`v-if="!isPitchBendMode"`)
- **MIN / MAX controls**: hidden
- **DURATION slider**: shown (0–100ms, step 5ms) — controls note overlap
- **Bipolar duration meter**: shown (blue+pink expanding bars, symmetric)

### Template — duration meter and control (pitch bend specific)
```vue
<template v-if="isPitchBendMode">
  <div class="duration-container">
    <div class="duration-meter">
      <div class="meter-bar-container" @mousedown="handleDurationBarMouseDown" @touchstart="handleDurationBarTouchStart">
        <!-- Bipolar style: blue left, pink right, both expand together -->
        <div class="meter-bar-wrapper">
          <div class="meter-bar blue-bar-base"></div>
          <div class="meter-bar blue-bar-active"
               :style="{ width: `${Math.max(5, (pitchBendOverlapMs / 100) * 100)}%` }"></div>
        </div>
        <div class="meter-divider"></div>
        <div class="meter-bar-wrapper">
          <div class="meter-bar pink-bar-base"></div>
          <div class="meter-bar pink-bar-active"
               :style="{ width: `${Math.max(5, (pitchBendOverlapMs / 100) * 100)}%` }"></div>
        </div>
      </div>
    </div>
    <div class="group">
      <label>DURATION</label>
      <div class="duration-control-wrapper">
        <ValueControl v-model="pitchBendOverlapMs" :min="0" :max="100" :step="5" :small-step="5" :large-step="25" />
        <span class="unit-label">ms</span>
      </div>
    </div>
  </div>
</template>
```

### `pitchBendOverlapMs` computed — maps to `onsetTime`
```typescript
const pitchBendOverlapMs = computed({
  get: () => model.value.onsetTime,
  set: (ms: number) => {
    model.value = { ...model.value, onsetTime: ms, offsetTime: ms }
  }
})
```
Both `onsetTime` and `offsetTime` are kept in sync (both hold the overlap duration).

### `watch(ccNumber)` — auto-configures when CC 208 is selected
```typescript
if (cc === 208) {
  const overlapMs = (model.value.onsetTime > 0 && model.value.onsetTime <= 500)
    ? model.value.onsetTime
    : 25
  model.value = {
    ...model.value,
    valueMode: VALUE_MODE_BIPOLAR,
    functionMode: 3, // Force PITCH_BEND
    minCCValue: 0,
    maxCCValue: 127,
    onsetTime: overlapMs,
    offsetTime: overlapMs
  }
} else if (model.value.functionMode === 3) {
  // Leaving Pitch Bend mode — reset to Incremental, unipolar
  model.value = {
    ...model.value,
    functionMode: 2,
    valueMode: VALUE_MODE_UNIPOLAR,
  }
}
isUpdatingInternally.value = false // MUST reset at end of watcher
```

### `filteredOptions` — CC 208 visible in Scale mode only
```typescript
if (isScaleMode) {
  // Scale mode: Scale Type (204) and Pitch Bend (208)
  return opt.value === 204 || opt.value === 208
}
```
CC 208 is **not shown** in Chord or ARP mode (firmware only implements it in SCALE mode).

---

## Interaction: Pitch Bend + Sustain

These two features interact intentionally:

- When **Sustain activates** → `setPitchBendOffset(0)` is called immediately (lever returns to center)
- While **Sustain is active** → `setPitchBendOffset()` returns early (`if (_sustainActive) return`)
- This prevents accidental lever movement mid-sustain from altering note pitches

---

## BLE Protocol — Field Reuse

Pitch bend does not require new BLE fields. It reuses the existing `LeverSettings` struct:

| Field | Pitch Bend meaning |
|---|---|
| `ccNumber` | 208 (KB1 internal marker) |
| `functionMode` | 3 = PITCH_BEND |
| `valueMode` | 1 = BIPOLAR (always) |
| `onsetTime` | Note overlap duration in ms (0–100) |
| `offsetTime` | Same as onsetTime (kept in sync) |
| `minCCValue` | 0 (full range; not user-editable in PB mode) |
| `maxCCValue` | 127 (full range; not user-editable in PB mode) |

---

## Files Changed Summary

| File | What changed |
|---|---|
| `firmware/src/objects/Globals.h` | Added `PITCH_BEND = 3` to `LeverFunctionMode` enum |
| `firmware/src/controls/LeverControls.h` | Added PITCH_BEND branch in `updateTargetValue()` and `updateValue()` |
| `firmware/src/controls/KeyboardControl.h` | Added `_pitchBendOffset`, `_pitchBendOverlapUs`, `_baseNote[]`, PB pending-offs queue; `setPitchBendOffset()`, `setPitchBendOverlapUs()`, `schedulePitchBendNoteOff()`, `processPendingPitchBendNoteOffs()`; `_baseNote[note]` set in `playMidiNote()`; suppressed during sustain |
| `KB1-config/src/data/ccMap.ts` | CC 208 "Pitch Bend" added to hardcoded KB1 Expression group |
| `KB1-config/src/ble/kb1Protocol.ts` | `PITCH_BEND = 3` in `LeverFunctionMode` enum; lever1 default to CC 208/mode 3; validator accepts 200–208 for levers |
| `KB1-config/src/components/LeverSettings.vue` | `isPitchBendMode` computed; UI locks (profile buttons, UNI/BI toggle, level meter hidden); DURATION slider added; `pitchBendOverlapMs` computed setter; CC 208 auto-configure in `watch(ccNumber)` |

---

## Common Pitfalls

1. **`isUpdatingInternally` not reset** — watcher permanently blocks category changes. Always `= false` at end of `watch(ccNumber)` in LeverSettings.vue. See `kb1-vue-patterns` skill.

2. **Missing exit path in `watch(ccNumber)`** — when switching away from CC 208, must reset `functionMode` to 0 (or 2). Without it, `isPitchBendMode` stays true for other CCs.

3. **Not guarding with `_sustainActive`** — if sustain is active and the lever is moved, held notes get retriggered at wrong pitch. The guard in `setPitchBendOffset()` prevents this.

4. **Not using `_baseNote[]`** — compact mode maps keys to non-chromatic notes. The note at press time differs from what you'd compute at release time if the lever has moved. Always use `_baseNote[i]` in the retrigger loop.

5. **Queue overflow** (`MAX_PB_PENDING_OFFS = 24`) — if exceeded, NoteOff fires immediately rather than silently dropping it. This is intentional safety behavior.
