# KB1 I2C Bus Efficiency Analysis

**Date:** April 12, 2026  
**Current Firmware:** v1.6.x series  
**Issue:** Investigating I2C bus efficiency and power consumption

---

## Current I2C Configuration

### Hardware Setup (from schematic)
- **Bus Speed:** 400kHz (I2C Fast Mode)
- **Devices:** 2× MCP23017 GPIO expanders
  - **U1 (mcp_U1):** Address 0x20
  - **U2 (mcp_U2):** Address 0x21
- **MCP23017 Spec:** Supports up to 1.7MHz (from datasheet)
- **Bus Topology:** Shared SDA/SCL, both devices on Core 1

### Software Configuration (main.cpp:1013)
```cpp
Wire.setClock(400000);  // 400kHz fast mode
```

**Status:** ✅ Clock speed is appropriate (see analysis below)

---

## Critical Inefficiency Found: I/O Read Pattern

### Current Implementation (INEFFICIENT)

The firmware reads GPIO pins **individually**, one at a time:

#### Keyboard Scanning (KeyboardControl.h:397)
```cpp
for (int i = 0; i < MAX_KEYS; ++i) {
    auto & key = _keys[i];
    bool raw = !key.mcp->digitalRead(key.pin);  // ❌ Individual I2C transaction
    // ...
}
```
- **19 keys** = **19 separate I2C transactions**

#### Lever Controls (main.cpp:1380-1385)
```cpp
if (xSemaphoreTake(i2cMutex, portMAX_DELAY) == pdTRUE) {
    lever1Right = mcp_U2.digitalRead(SWD1_RIGHT_PIN) == LOW;      // ❌ Individual
    lever2Right = mcp_U2.digitalRead(SWD2_RIGHT_PIN) == LOW;      // ❌ Individual
    lever1Left = mcp_U1.digitalRead(SWD1_LEFT_PIN) == LOW;        // ❌ Individual
    lever2Left = mcp_U2.digitalRead(SWD2_LEFT_PIN) == LOW;        // ❌ Individual
    leverPush1Pressed = mcp_U1.digitalRead(SWD1_CENTER_PIN) == LOW;  // ❌ Individual
    leverPush2Pressed = mcp_U2.digitalRead(SWD2_CENTER_PIN) == LOW;  // ❌ Individual
    xSemaphoreGive(i2cMutex);
}
```
- **6 lever inputs** = **6 separate I2C transactions**

### Total I2C Traffic Per Scan Cycle
- **25+ I2C read transactions** (19 keyboard + 6 levers + LED writes)
- Each `digitalRead()` is a full I2C transaction:
  1. START condition
  2. Device address (7 bits) + R/W bit
  3. Register address
  4. Repeated START
  5. Read 1 byte
  6. STOP condition

**At 400kHz I2C:**
- Single pin read: ~200-250μs per transaction
- Full scan cycle: **~5-6ms of I2C bus time alone**

---

## Optimal Implementation (BULK READS)

### MCP23017 Bulk Read Capability

The MCP23017 has bulk read functions that read **all 16 GPIO pins in a single I2C transaction:**

```cpp
uint16_t readGPIOAB();  // Read all 16 pins (both ports) at once
uint8_t readGPIOA();    // Read port A (8 pins)
uint8_t readGPIOB();    // Read port B (8 pins)
```

### Proposed Implementation

```cpp
// Read ALL inputs from both MCP chips in just 2 I2C transactions
if (xSemaphoreTake(i2cMutex, portMAX_DELAY) == pdTRUE) {
    uint16_t u1_pins = mcp_U1.readGPIOAB();  // Read all 16 pins from U1
    uint16_t u2_pins = mcp_U2.readGPIOAB();  // Read all 16 pins from U2
    xSemaphoreGive(i2cMutex);
}

// Then extract individual pin states with bitwise operations (no I2C overhead)
bool lever1Left = !(u1_pins & (1 << SWD1_LEFT_PIN));
bool lever1Center = !(u1_pins & (1 << SWD1_CENTER_PIN));
bool key0_pressed = !(u1_pins & (1 << _keys[0].pin));
// etc...
```

### Performance Improvement

| Metric | Current (Individual) | Optimized (Bulk) | Improvement |
|--------|---------------------|------------------|-------------|
| I2C Transactions | 25+ per scan | **2 per scan** | **92% reduction** |
| I2C Bus Time | ~5-6ms | **~0.4ms** | **>10× faster** |
| CPU Overhead | High (mutex + I2C wait) | Minimal | Significant |
| Power Consumption | Higher (more I2C activity) | Lower | ~5-10% reduction |

---

## Clock Speed Analysis

### Should We Increase Clock Beyond 400kHz?

**Recommendation: NO** — Here's why:

#### Current 400kHz is Optimal for This Design

1. **Diminishing Returns**
   - Going from 400kHz → 1MHz = 2.5× speed increase
   - But with bulk reads (25 trans → 2 trans), we get **12.5× improvement**
   - Bulk reads give far more benefit than clock speed increase

2. **Reliability Concerns at Higher Speeds**
   - ESP32S3 I2C hardware has known flakiness above 400kHz
   - Pull-up resistor values may not be optimized for 1MHz
   - Wire capacitance on PCB traces can cause signal integrity issues
   - MCP23017 interrupt lines (INTA/INTB) may experience glitches

3. **Power Consumption**
   - Higher I2C clock = higher edge rate = more current spikes
   - 400kHz is the sweet spot for battery-powered devices

4. **No Real-World Benefit**
   - With bulk reads: 0.4ms bus time is **negligible** (0.04% of 10ms scan cycle)
   - Going to 1MHz saves only ~0.2ms — not meaningful for human input latency

#### What Was "Ignorance" About?

You previously upgraded from **100kHz → 400kHz** (v1.3.0), which was **absolutely correct**:
- v1.2.6: 100kHz, ~10ms per GPIO read
- v1.3.0: 400kHz, ~2.5ms per GPIO read (RELEASE_NOTES_v1.3.0.md:126)

That was not ignorance — that was a **4× improvement** and exactly the right move.

---

## Power Efficiency Impact

### Current Power Draw from I2C Activity

Estimated I2C-related power consumption:
- **Active I2C bus time:** ~5-6ms per 10ms scan cycle = 50-60% bus utilization
- **MCP23017 active current:** ~1mA per chip during I2C transaction
- **ESP32 I2C peripheral:** ~5-8mA when active
- **Total I2C overhead:** ~10-15mA average during scanning

### With Bulk Reads

- **Active I2C bus time:** ~0.4ms per 10ms scan cycle = 4% bus utilization
- **Reduced active time:** 92% reduction
- **Power savings:** ~8-12mA reduction during active use
- **Battery life impact:** ~5-10% longer runtime (currently ~8hr active BLE mode)

---

## Implementation Recommendations

### Priority 1: Bulk Read Optimization (HIGH IMPACT)

1. **Create GPIO read cache structure:**
   ```cpp
   struct MCPInputCache {
       uint16_t u1_pins;
       uint16_t u2_pins;
       unsigned long timestamp;
   };
   ```

2. **Single bulk read function:**
   ```cpp
   MCPInputCache readAllInputs() {
       MCPInputCache cache;
       if (xSemaphoreTake(i2cMutex, portMAX_DELAY) == pdTRUE) {
           cache.u1_pins = mcp_U1.readGPIOAB();
           cache.u2_pins = mcp_U2.readGPIOAB();
           cache.timestamp = micros();
           xSemaphoreGive(i2cMutex);
       }
       return cache;
   }
   ```

3. **Refactor KeyboardControl to use cached reads:**
   - Pass cached pin states instead of calling digitalRead()
   - Extract pin values with bitwise operations
   - Zero I2C overhead for keyboard scanning

4. **Refactor lever/octave controls similarly**

### Priority 2: LED Write Optimization (MEDIUM IMPACT)

LEDs (octave up/down) also use individual GPIO writes. Consider:
- Batch LED state changes
- Use `writeGPIOAB()` for bulk writes
- Or migrate octave LEDs to ESP32 GPIO pins (no I2C needed)

### Priority 3: I2C Speed (LOW PRIORITY — SKIP)

**Do NOT increase I2C clock speed:**
- Current 400kHz is optimal
- Focus on bulk reads instead (far better ROI)
- Only consider if you add more I2C devices in future hardware revision

---

## Expected Results After Optimization

### Performance
- ✅ Input latency: Reduced from ~5-6ms to <0.5ms I2C overhead
- ✅ CPU overhead: Significant reduction (less mutex contention)
- ✅ Smoother LED animations (less I2C blocking)

### Power Consumption
- ✅ Active current: ~8-12mA reduction
- ✅ Battery life: ~30-45 minutes additional active runtime
- ✅ Sleep modes: No impact (I2C idle)

### Code Quality
- ✅ Cleaner architecture (centralized I/O read)
- ✅ Less mutex contention
- ✅ Easier to add new inputs without I2C overhead

---

## Summary

### What's Good
- ✅ **400kHz I2C speed is perfect** — don't change it
- ✅ Mutex protection is correct
- ✅ Pin assignments are efficient (distributed across both MCPs)

### What's Inefficient
- ❌ **Individual pin reads** (25+ I2C transactions per scan)
- ❌ Should use **bulk reads** (2 I2C transactions per scan)

### Action Items
1. Implement bulk read optimization (**12× speedup, 92% I2C reduction**)
2. Refactor keyboard/lever scanning to use cached pin states
3. Keep I2C clock at 400kHz (optimal for reliability and power)

### Previous "Ignorance" Was Actually Smart
The v1.3.0 upgrade from 100kHz → 400kHz was **exactly correct** and gave you a 4× improvement. The real opportunity now is the **data flow pattern**, not the clock speed.

---

## References
- Schematic: kb1-full-schematic.md
- Current implementation: main.cpp:1374-1386, KeyboardControl.h:386-410
- MCP23017 datasheet: Microchip DS20001952
- v1.3.0 release notes: RELEASE_NOTES_v1.3.0.md (I2C speed upgrade)
