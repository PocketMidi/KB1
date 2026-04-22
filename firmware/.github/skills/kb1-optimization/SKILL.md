---
name: kb1-optimization
description: Performance optimization strategies for KB1 firmware. Use when optimizing code, improving responsiveness, reducing latency, debugging performance issues, or planning efficiency improvements. Contains I2C bulk read strategy (12× speedup), serial output optimization (80% reduction), touch sensor debouncing, and performance profiling data. Includes implementation plans and measured results.
---

# KB1 Performance Optimization

Proven strategies and implementation plans for optimizing KB1 firmware performance.

---

## I2C Bus Optimization Strategy

**Current Status:** Analysis complete, implementation ready, **NOT yet deployed**  
**Expected Benefit:** 12× speedup in input scanning (5-6ms → 0.4ms per cycle)  
**Files:** `I2C_EFFICIENCY_ANALYSIS.md`, `I2C_BULK_READ_IMPLEMENTATION_PLAN.md`

### Critical Finding: Bulk Read Opportunity

**Current Implementation (Inefficient):**
- **25+ individual I2C transactions per scan cycle**
  - 19 keyboard keys (`KeyboardControl.h:397`)
  - 6 lever/push inputs (`main.cpp:1380-1385`)
- **I2C bus time:** ~5-6ms per scan (50-60% utilization)
- **Power overhead:** ~10-15mA from I2C activity
- **Each transaction:** Mutex lock → START → address → register → data → STOP → unlock

**Optimal Implementation (Not Yet Applied):**
- **2 bulk reads:** Use `mcp.readGPIOAB()` to read all 16 pins at once
- **Only 2 I2C transactions total:** One per MCP23017 chip
- **I2C bus time:** ~0.4ms per scan (**92% reduction**)
- **Power savings:** ~8-12mA reduction
- **Method:** Single I2C transaction reads both GPIO ports (GPIOA + GPIOB = 16 bits)

### Implementation Pattern

```cpp
// Centralized bulk read function
struct MCPInputCache {
    uint16_t u1_pins;  // All 16 pins from MCP @ 0x20
    uint16_t u2_pins;  // All 16 pins from MCP @ 0x21
};

MCPInputCache readAllInputs() {
    MCPInputCache cache;
    if (xSemaphoreTake(i2cMutex, portMAX_DELAY) == pdTRUE) {
        cache.u1_pins = mcp_U1.readGPIOAB();  // 1 I2C transaction
        cache.u2_pins = mcp_U2.readGPIOAB();  // 1 I2C transaction
        xSemaphoreGive(i2cMutex);
    }
    return cache;
}

// Extract individual pin states with bitwise operations (no I2C overhead)
bool lever1Left = !(cache.u1_pins & (1 << SWD1_LEFT_PIN));
bool lever1Right = !(cache.u1_pins & (1 << SWD1_RIGHT_PIN));
// ... etc for all 32 pins
```

**Key Insight:** Bitwise operations on cached data are ~1000× faster than I2C reads.

### Expected Benefits

**Performance:**
- Input latency: **-5ms I2C overhead** (more responsive controls)
- CPU efficiency: Less mutex blocking, smoother LED animations
- LED update contention: Reduced (less I2C bus congestion)

**Power:**
- Active current: **-8-12mA reduction**
- Battery life: **+30-45min active runtime** (~5-10% improvement)
- Heat: Negligible improvement (I2C was never a heat source)

**Code Quality:**
- Centralized I/O reading (single source of truth)
- Less mutex contention between tasks
- Easier to add new inputs (just add bitwise mask)

### I2C Clock Speed: Keep at 400kHz

**Current Setting:** 400kHz (Fast Mode I2C) — **DO NOT CHANGE**

**Why not increase to 1.7MHz?**
1. **Bulk reads give 12× improvement** vs only 2.5× from clock speed increase
2. **ESP32-S3 I2C reliability issues above 400kHz** (known driver limitations)
3. **Current 0.4ms bus time (with bulk reads) is negligible** — not worth risk
4. **400kHz is optimal for:** Signal integrity, battery life, reliability

**History:**  
- v1.0-v1.2: 100kHz (slow, conservative)
- v1.3.0: Upgraded to 400kHz (4× improvement, correct move)
- Future: Keep at 400kHz, implement bulk reads instead

### Implementation Priority

| Priority | Optimization | Speedup | Risk |
|----------|--------------|---------|------|
| **HIGH** | Bulk I2C reads | 12× | Low |
| **MEDIUM** | LED bulk writes | 3-5× | Low |
| **LOW/SKIP** | Clock speed increase | 2.5× | Medium-High |

### Implementation Status

**Ready to implement:** See `I2C_BULK_READ_IMPLEMENTATION_PLAN.md` for step-by-step guide (~2.5 hours)

**Conservative approach:**
1. Implement bulk I2C reads (12× speedup, low risk)
2. Test thoroughly before considering other changes
3. Monitor power consumption to validate savings

**Aggressive approach (optional):**
- Reduce scan rate: 2ms → 5ms (less frequent polling, still responsive)
- Reduce touch debounce: 200ms → 50ms in Toggle mode only
- LED bulk writes (parallel to I2C optimization)

---

## Serial Output Optimization

**Status:** Deployed in v1.2.6+  
**Impact:** 80% reduction in serial overhead, 6-8× faster during rapid playing  
**Philosophy:** Serial printing is **blocking I/O** (~5-10ms per call) — minimize aggressively

### Compact Serial Format

All debug output uses **ultra-compact single-character prefixes** to minimize CPU overhead.

#### Notes (KeyboardControl.h)
```cpp
// Before (v1.0-v1.2): 3 lines per note
Note On: 65 (velocity 85)
Key: F4
Note Off: 65

// After (v1.2.6+): 1 compact line
N65v85    // Note 65 on, velocity 85
N65-      // Note 65 off
C60-      // Chord note 60 off
```

#### Levers (LeverControls.h)
```cpp
// Before: "INCREMENTAL: 85 -> 91 (stepSize: 6)"
// After:
L91>      // Lever incremented to 91
L73<      // Lever decremented to 73
V91       // Velocity set to 91
C3=127    // CC 3, value 127
```

#### Lever Push (LeverPushControls.h)
```cpp
C24=58    // CC 24, value 58
P3=76     // Pattern 3 selected (MIDI value 76)
```

#### Touch Sensor (TouchControl.h)
```cpp
Touch CC1=72    // Continuous mode (throttled to 500ms max)
KB1 Expression: Shape set to 3    // Pattern Selector (150ms rate limit)
```

#### Arpeggiator / Pattern System (KeyboardControl.h)
```cpp
Arp:R60P3       // Arpeggiator started: Root=60, Pattern=3
ArpRoot:60@2    // Root changed to 60, continuing at index 2
S0:N60v127      // Strum note index 0: Note 60, Velocity 127
Arp0:N72v85     // Arp note index 0: Note 72, Velocity 85
Arp-72          // Arp note 72 off
Arp:Loop        // Arpeggiator loop restarted
Arp:Stop76      // Arpeggiator stopped, note 76 off
```

#### Octave Control (OctaveControl.h)
```cpp
O-1       // Octave down by 1
O+1       // Octave up by 1
```

#### BLE Connection (ServerCallbacks.cpp, BLEGestureControl.h)
```cpp
BLE+          // Client connected
BLE-          // Client disconnected
BLE:Cross     // Cross-lever gesture detected
BLE:On        // Gesture activation threshold reached
BLE:Abort     // Gesture released before activation
BLE:Ready     // Bluetooth ready, advertising
```

#### Battery Monitoring (v1.4.0+)
```cpp
Chg: 476/19800s           // Charging progress (current/total seconds)
Saved                     // Battery state saved to NVS
CHARGE_60S | USB:1 | Chg:1 | Slp:1   // Event markers (every 60s)
```

### Performance Impact

**Measured Results:**
- **Before optimization:** ~70-80% serial overhead during rapid playing
- **After optimization:** ~10-15% serial overhead
- **Speed improvement:** 6-8× faster
- **Side effect:** No more garbled output during fast keyboard/lever movements

**Why it matters:**
- Serial printing **blocks the CPU** for 5-10ms per call
- High-frequency loops (keyboard polling, touch sensing) call serial output frequently
- Queue backup during rapid input → lag → unresponsive controls

### Optimization Strategies Applied

**1. Rate Limiting:**
```cpp
static unsigned long lastPatternChange = 0;
unsigned long now = millis();
if (now - lastPatternChange < 150) {
    return;  // Skip serial output if too soon
}
lastPatternChange = now;
```

**Use for:** Pattern Selector (150ms), Touch continuous mode (500ms), CC interpolation (300ms)

**2. Condensing Multiple Prints:**
```cpp
// Before: 6 serial calls (30-60ms blocked)
SERIAL_PRINT("KB1 Expression: Pattern Selector set to pattern ");
SERIAL_PRINTLN(patternIndex);
SERIAL_PRINT("  Strum Pattern: ");
SERIAL_PRINTLN(_chordSettings.strumPattern);
SERIAL_PRINT("  MIDI CC ");
SERIAL_PRINT(_ccNumber);

// After: 1 serial call (5-10ms blocked)
char buf[32];
snprintf(buf, sizeof(buf), "KB1 Expression: Shape set to %d", patternIndex);
SERIAL_PRINTLN(buf);
```

**3. Throttling CC Changes:**
```cpp
// Only print CC changes every 300ms (reduces interpolation spam)
static unsigned long lastCCPrint = 0;
if (millis() - lastCCPrint >= 300) {
    SERIAL_PRINT("C"); SERIAL_PRINT(ccNum);
    SERIAL_PRINT("="); SERIAL_PRINTLN(value);
    lastCCPrint = millis();
}
```

**4. Auto-Detection:**
```cpp
// Only output when USB terminal is connected (CDC detection)
// End users see zero overhead when serial not in use
if ((bool)Serial) {
    SERIAL_PRINTLN("...");
}
```

### Production Release Philosophy (v1.4.0+)

**CRITICAL RULE:** All serial output in production must be compact and efficient.

**Design Principles:**
1. **Efficiency First:** Every serial print blocks 5-10ms — minimize aggressively
2. **Essential Info Only:** Print only what's needed to verify operation
3. **Compact Format:** Use shortest possible representation
4. **No Redundancy:** Don't repeat information already shown

**Example — Battery Monitoring:**

Before (verbose):
```
Charging: 476s / 19800s (19323s remaining)
Battery state saved: Uncalibrated
```

After (compact):
```
Chg: 476/19800s
Saved
```

**Impact:** ~40ms saved per update (every 30s) = negligible overhead  
**Benefit:** Cleaner logs, faster serial throughput, consistent with format philosophy

### Debug Tips

When troubleshooting, grep for these patterns:
```bash
grep "^N[0-9]" serial.log    # Notes
grep "^L[0-9]" serial.log    # Levers
grep "^C[0-9]" serial.log    # CC messages
grep "Touch" serial.log      # Touch sensor
grep "Arp:" serial.log       # Arpeggiator
grep "BLE" serial.log        # BLE events
```

---

## Touch Sensor Optimization

**Status:** Deployed in v1.2.6+  
**Impact:** Fixed Pattern Selector lag issue ("stalls after 2-3 taps")

### Pattern Selector Lag Fix

**Original Problem:**
- After 2-3 rapid taps, Pattern Selector becomes unresponsive
- User must wait several hundred milliseconds before next tap registers

**Root Cause:**
Serial printing was blocking I/O. Each Pattern Selector tap triggered **6× `SERIAL_PRINT()` calls**:

```cpp
SERIAL_PRINT("KB1 Expression: Pattern Selector set to pattern ");
SERIAL_PRINTLN(patternIndex);
SERIAL_PRINT("  Strum Pattern: ");
SERIAL_PRINTLN(_chordSettings.strumPattern);
SERIAL_PRINT("  MIDI CC ");
SERIAL_PRINT(_ccNumber);
SERIAL_PRINT(": ");
SERIAL_PRINTLN(PATTERN_VALUES[patternIndex]);
```

**Each serial call blocks for ~5-10ms** = **30-60ms total delay per tap**

After 2-3 rapid taps, serial queue backs up → perceivable lag → feels "stuck"

### Solution Applied

**File:** `src/controls/TouchControl.h` (~line 160)

**1. Reduced serial output from 6 lines to 1:**
```cpp
SERIAL_PRINT("KB1 Expression: Shape set to ");
SERIAL_PRINTLN(patternIndex);
```

**2. Added rate limiting (150ms minimum between changes):**
```cpp
static unsigned long lastPatternChange = 0;
unsigned long now = millis();
if (now - lastPatternChange < 150) {
    _wasPressed = isPressed;  // Update state to prevent re-trigger
    break;
}
lastPatternChange = now;
```

**Result:**
- ✅ No more lag after rapid taps
- ✅ Smooth, responsive Pattern Selector cycling
- ✅ 150ms feels natural (prevents accidental double-taps while allowing intentional rapid changes)

### Touch Sensor Core Affinity

**CRITICAL:** Touch sensor MUST run on Core 1

**Why:**
- Uses ESP32 capacitive touch peripheral (hardware tied to Core 1)
- When `touchRead()` called from Core 0 → returns garbage (3755997 instead of 24000-64000)

**Symptom of wrong core:**
- Flashing LEDs continuously
- No MIDI output from touch sensor
- Serial shows maxed touch values (out of range)

**Solution:** Keep all inputs including touch on Core 1 (already correct in current architecture)

### Continuous Mode Throttling

Touch in continuous mode (CC sweep) sends MIDI constantly → serial spam

**Optimization** (`src/controls/TouchControl.h` ~line 218):

```cpp
// Throttled serial output for continuous mode (print every 500ms max)
static unsigned long lastContinuousPrint = 0;
unsigned long now = millis();
if (now - lastContinuousPrint >= 500) {
    SERIAL_PRINT("Touch CC"); SERIAL_PRINT(_ccNumber);
    SERIAL_PRINT("="); SERIAL_PRINTLN(sendVal);
    lastContinuousPrint = now;
}
```

**Result:** Serial output reduced from 30+ lines/sec to ~2 lines/sec  
(Still see activity without flooding logs)

---

## Key Takeaways

### Performance Optimization Hierarchy

1. **Architecture changes** (e.g., bulk I2C reads): 10-100× improvements possible
2. **Algorithm changes** (e.g., rate limiting, debouncing): 2-10× improvements
3. **Micro-optimizations** (e.g., clock speed tuning): <2× improvements, higher risk

### Serial Printing is NOT FREE

**Cost:** 5-10ms blocking I/O per call  
**Impact:** One of the most expensive operations in embedded systems

**Always:**
1. Minimize print calls in high-frequency loops
2. Use throttling for continuous/rapid events
3. Condense multi-line messages to single compact lines
4. Consider disabling non-critical debug output in production builds

### Touch Sensor Behavior

- Hardware peripheral with Core 1 affinity
- Requires debouncing and rate limiting for responsive UX
- Serial output can cause perceivable lag if not throttled

### I2C Bus Strategy

- Bulk reads >> clock speed increases (12× vs 2.5× improvement)
- ESP32-S3 I2C driver has known reliability issues above 400kHz
- Power savings from bulk reads: ~8-12mA (significant for battery life)

---

## Implementation Plans

**Ready to Deploy:**
- `firmware/I2C_BULK_READ_IMPLEMENTATION_PLAN.md` — Step-by-step guide (~2.5hr)
- `firmware/I2C_EFFICIENCY_ANALYSIS.md` — Technical deep-dive
- `firmware/ADDITIONAL_PERFORMANCE_OPPORTUNITIES.md` — Other optimizations

**Already Deployed:**
- Serial output optimization (v1.2.6+)
- Touch sensor rate limiting (v1.2.6+)
- Pattern Selector lag fix (v1.2.6+)

**Conservative Next Step:**
1. Implement bulk I2C reads (Phase 1-4 from implementation plan)
2. Test after each phase
3. Validate power savings and latency improvements
4. Monitor for regressions

**Aggressive Next Step (if conservative succeeds):**
- Reduce scan rate to 5ms (from 2ms)
- Reduce touch debounce to 50ms in Toggle mode
- LED bulk writes (parallel optimization)

---

## Revision History

- **April 22, 2026:** Migrated from `/memories/repo/` to `.github/skills/kb1-optimization/`
- **April 12, 2026:** I2C bulk read analysis complete, implementation plan ready
- **March 15, 2026:** Touch sensor optimization (v1.2.6) — Pattern Selector lag fix
- **February 28, 2026:** Serial output optimization (v1.2.6) — 80% reduction in overhead
