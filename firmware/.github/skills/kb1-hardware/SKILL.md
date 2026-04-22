---
name: kb1-hardware
description: KB1 hardware specifications, schematics, and component details. Use when working on hardware integration, debugging electrical issues, understanding pin assignments, audio system, battery system, or planning hardware modifications. Contains full schematic (MCP23017 I2C expanders, PAM8406 audio amp, XIAO ESP32-S3 MCU, battery system), measured power consumption, and hardware roadmap decisions.
---

# KB1 Hardware Reference

Complete hardware specifications for the KB1 MIDI controller.

## System Overview

**MCU:** XIAO ESP32-S3 (dual-core Xtensa LX7, 240MHz)  
**Power:** 3.7V Li-ion 420mAh battery  
**I/O Expansion:** 2× MCP23017 I2C GPIO expanders  
**Audio:** PAM8406DR Class-D amplifier + 2× 8Ω 1W speakers  
**Connectivity:** BLE MIDI, Serial MIDI (UART), USB-C

---

## Full Schematic — Rev B.0 (2023-12-17)

### MCU: XIAO-ESP32S3 (H2)
- **Decoupling:** C1 (0.1uF) on power
- **I/O Pins Used:** D0-D10, SDA, SCL, MOSI, MISO
- **Touch Inputs:** Touch 1, Touch 2 (capacitive GPIO)
- **I2C Bus:** SDA/SCL shared with both MCP23017 expanders

### Power Section
- **Battery:** 3.7V Li-ion 420mAh via 4-wire connector (128-PH-SHI-TH_IF_S4)
- **Power Switch:** S6 (main power)
- **Charging:** XIAO built-in 100mA charge controller
  - **CRITICAL:** Requires USB enumeration (dumb chargers won't work)
  - **Charge Time:** ~5 hours (BATTERY_FULL_CHARGE_MS = 18000000)

### I2C GPIO Expanders (2× MCP23017)

#### U1: MCP23017-E/SS — I2C Address 0x20
- **Interrupt Lines:** INTA, INTB (not currently used in firmware)
- **Ports:** GPA0-7, GPB0-7 (16 GPIO pins total)
- **Connected To:**
  - Keyboard matrix switches
  - Lever 1 switches (left/right/push)
  - Lever 2 switches (left/right/push)
  - Some LED outputs

#### U2: MCP23017-E/SS — I2C Address 0x21
- **Ports:** GPA0-7, GPB0-7 (16 GPIO pins total)
- **Connected To:**
  - Keyboard matrix switches
  - Octave Up/Down buttons
  - Octave LED outputs (GPB0 = addr_15 LED indicator)
  - Additional control inputs

**I2C Shared Bus:**
- Clock Speed: 400kHz (Fast Mode I2C)
- Pull-ups: 4.7kΩ on SDA/SCL lines
- **CRITICAL:** Both expanders MUST be accessed from same core (Core 1)

### Lever Controls

**Lever 1:**
- Switch: S1 (H14, MSS1 type indicator)
- Slider: SW01 (KI-5203JA-05) — Left/Right positions
- Push Button: Integrated in slider mechanism

**Lever 2:**
- Switch: S2 (H15, MSS1 type indicator)
- Slider: SW02 (KI-5203JA-05) — Left/Right positions
- Push Button: Integrated in slider mechanism

### Keyboard Switches

**19-Key MIDI Keyboard** — All SPST switches via MCP23017 GPIO  
MIDI note assignments and reference frequencies:

| Switch | MIDI Note | Note Name | Frequency |
|--------|-----------|-----------|-----------|
| SW3    | 72        | C5        | 523.25 Hz |
| SW4    | 71        | B4        | 493.88 Hz |
| SW5    | 75        | Eb5       | 622.25 Hz |
| SW6    | 76        | E5        | 659.25 Hz |
| SW7    | 73        | C#5       | 554.37 Hz |
| SW8    | 74        | D5        | 587.33 Hz |
| SW9    | 67        | G4        | 392.00 Hz |
| SW10   | 68        | G#4/Ab4   | 415.30 Hz |
| SW11   | 69        | A4        | 440.00 Hz |
| SW12   | 70        | A#4/Bb4   | 466.16 Hz |
| (aux)  | 59        | B3        | 246.94 Hz |
| (aux)  | 60        | C4        | 261.63 Hz (Middle C) |
| (aux)  | 61        | C#4       | 277.18 Hz |
| (aux)  | 64        | E4        | 329.63 Hz |
| (aux)  | 65        | F4        | 349.23 Hz |

### Octave Controls

**Octave Up Button:**
- Connected to U2 (MCP23017 @ 0x21)
- LED: SP51 Blue LED with current-limiting resistor R6

**Octave Down Button:**
- Connected to U2 (MCP23017 @ 0x21)
- LED: SP51 Blue LED with current-limiting resistor R6

---

## Audio System — PAM8406 Amplifier

### Amplifier IC: U3 PAM8406DR

**Type:** Class-D, BTL (Bridge-Tied Load) stereo output  
**Input:** 3.7V Li-ion direct (no boost converter)  
**Configuration:**

| Pin | Function | Connection |
|-----|----------|------------|
| 5   | MUTE     | Via R10 (10Ω) |
| 7   | INL      | 20kΩ R12 + 1µF C7 AC coupling |
| 9   | MODE     | Pulled LOW = Class-D mode (efficient) |
| 10  | INR      | 20kΩ R13 + 1µF C8 AC coupling |
| 12  | SHDN     | Controlled by SLSW1 slide switch |
| 1/3 | +OUT_L / -OUT_L | BTL left channel → SP1 L+/L- |
| 14/16 | -OUT_R / +OUT_R | BTL right channel → SP1 R+/R- |

### Audio Connectors

- **plug_2:** 3.5mm TRS — Line In from PTM "Line Out" (input to amp)
- **J1:** 3.5mm TRS — Line Out to PTM "Line In" (pass-through)
- **J2:** 3.5mm TRS — Connected to amp input path

### Speakers: SP1 (Stereo Pair)

**Model:** Ole Wolff OWS-111535TA-8A  
**Impedance:** 8Ω  
**Power:** 1W nominal per speaker  
**Sensitivity:** 79dB SPL @ 1W/1m  
**Connector:** 4-pin (L+, L-, R+, R-)

### Speaker Control & Indicator

**SLSW1 Slide Switch:** JS202011JAQN  
- Hardware control of PAM8406 SHDN pin 12
- No firmware involvement (purely hardware on/off)

**D3 Green LED:** Speaker power indicator  
- Current limiting: R11 (100Ω)
- Illuminates when amp is active

### Power Filtering & EMI Suppression

**Input Power Filtering:**
- VDD side: C3 (10µF) + C9 (1µF) + R10 (10Ω) + C4 (1µF) + C6 (1µF)
- PVDD side: C5 (10µF) + C10 (1µF)

**Output EMI Filtering:**
- FB3-FB6: 120Ω @ 100MHz ferrite beads on all 4 BTL output lines
- C11-C14: 220pF capacitors on speaker output lines for HF noise suppression
- ~3-inch twisted wire runs to speakers to reduce radiated EMI

### Measured Power Consumption

**Amplifier Current Draw:**
- **Idle** (on, no signal): 2-10mA
- **With music** (during system sleep test): ~48mA average
  - Sleep test: 99% battery → 8hr 15min runtime
  - Calculation: 416mAh ÷ 8.25hr = 50.4mA total (close to observed)
- **Active system + amp:** 95mA base + 50mA amp ≈ 145mA total
- **Peak draw:** Up to 550mA (heavy bass at max volume)

**Battery Runtime Estimates:**
- **Active with speakers:** ~2.9 hours from full charge
- **Active without speakers:** ~4.4 hours from full charge
- **BLE connected, idle:** ~8.4 hours
- **BLE disconnected, idle:** ~8.4 hours (same, BLE drain is minimal)

---

## MIDI I/O Section

**Hardware:** 3.5mm TRS Type-A MIDI connectors (T=source, R=sink, S=shield)

**plug_3:** MIDI OUT to PTM "MIDI IN"  
- Connector: AudioJack3_1503-08 (J3)
- EMI: FB1 ferrite bead on signal line

**plug_4:** MIDI IN from PTM "MIDI OUT"  
- Connector: AudioJack3_1503-08
- EMI: FB2 ferrite bead on signal line

**Interface:** Serial UART (Serial0, 31,250 baud)  
**NOT USB MIDI** — KB1 does not support USB MIDI class device mode

---

## Battery System & Hardware Roadmap

### Current Implementation (KB1.5)

**Software-Based Monitoring:**
- Time-based capacity estimation
- Tracks: BLE on/off time, sleep time, speaker time
- Requires: Full charge calibration by user
- Accuracy: ±5-10% when calibrated
- NVS persistence for state across reboots

**Current Drain Estimates:**
- Active (BLE connected): 95mA
- Active (BLE disconnected): 50mA
- Sleep mode: 2mA
- Speaker amplifier: +50mA (flat rate, conservative)

### SY6410 Hardware Gauge Decision (April 2026)

**Conclusion:** Skip for KB1.5, defer to KB2 or >500 unit production

**Why Skip:**
1. Development cost ($3-5K) >> BOM savings at 50-100 unit scale
2. Support fragmentation: Two hardware variants to maintain
3. No user complaints about current accuracy (only persistence bugs, now fixed v1.6.2)
4. Diminishing returns: 2% vs 5% accuracy not perceptible in real use

**When to Revisit:**
- Production scale exceeds 500 units/year
- Doing full PCB redesign for other reasons (marginal cost negligible)
- User feedback shows battery accuracy is pain point
- KB2 new hardware revision with other changes

### Future Implementation (KB2)

**If Adding SY6410 I2C Battery Gauge:**

**Hardware Integration:**
- IC: SY6410AAC (SOT-23-6 package)
- I2C Address: 0x55 (no conflict with MCP23017 @ 0x20/0x21)
- Placement: Power section near battery connector
- Connections:
  - CELL pin: Direct short trace to Battery+ terminal
  - GND pin: Direct to Battery- terminal
  - VDD: 3.3V from ESP32 LDO
  - SDA/SCL: Connect to existing I2C bus (parallel with MCP23017s)
- Decoupling: 1µF on CELL, 1µF on VDD
- Pull-ups: 4.7kΩ (if not already present on I2C bus)

**Firmware Changes:**
- New driver: `src/battery/SY6410.cpp` for I2C communication
- Extend battery status BLE characteristic from 10 to 12 bytes:
  - Byte 10: Hardware version (1=KB1, 2=KB1.5, 3=KB2)
  - Byte 11: Data source (0=software, 1=hardware gauge)
- Runtime detection: Probe I2C 0x55 on boot, fall back to software if not present
- Keep software estimation code for backward compatibility

**Config App Changes:**
- Update `BatteryStatus` interface with optional fields: `hardwareVersion`, `dataSource`
- Update `decodeBatteryStatus()` for backward compatibility (10/12 byte packets)
- Add UI badge: "⚡ Hardware Gauge" vs "📊 Estimated"

**Benefits:**
- ±2-3% accuracy (vs ±5-10% software)
- No calibration required ("it just works")
- Built-in temperature compensation
- Self-calibrating algorithm
- Future features: battery health monitoring, cycle count, predictive replacement

**Power Impact:** ~75µA quiescent (negligible vs 420mAh capacity)  
**BOM Cost:** $0.30-0.50 per unit + 2× 1µF caps ($0.10)

---

## Design Notes & Constraints

### I2C Bus Constraints

- **Shared Bus:** All I2C devices on single SDA/SCL pair
- **Speed:** 400kHz (Fast Mode) — tested safe, optimal for KB1
- **Maximum Speed:** 1.7MHz (MCP23017 datasheet spec)
- **Core Affinity:** ALL I2C operations MUST occur on Core 1 (ESP32 driver limitation)
- **Cross-Core Access:** WILL cause `Unfinished Repeated Start transaction!` crash

### Power Budget Breakdown

| State | Current Draw | Runtime (420mAh) |
|-------|--------------|------------------|
| Active, BLE on, speakers on | 145mA | 2.9 hours |
| Active, BLE on, speakers off | 95mA | 4.4 hours |
| Idle, BLE connected | 50mA | 8.4 hours |
| Sleep mode | 2mA | 210 hours (8.75 days) |
| Charging (USB enumerated) | 100mA input | 5 hours to full |

### Charging Behavior (CRITICAL)

**USB at boot sequence:**
1. Device boots with USB already connected
2. Charge controller enters **bypass/power mode**
3. Device is powered, battery is NOT charging
4. Firmware detects: `usbConnectedAtBoot = true`
5. Battery monitoring: Stays "uncalibrated" (254), shows "?" in GUI

**Battery boot → USB plug sequence:**
1. Device boots on battery power only
2. User plugs USB after boot
3. Charge controller enters **charging mode**
4. Device is powered AND battery is charging
5. Firmware detects: `usbConnectedAtBoot = false`, `isChargingMode = true`
6. Battery monitoring: Runs calibration timer, tracks charge progress

**Why this matters:**
- Firmware MUST detect boot sequence to determine if battery is actually charging
- Early detection: `usbConnectedAtBoot = isUsbPowered()` BEFORE `loadBatteryState()`
- Invalid sequence (USB at boot): Hardware NOT charging → don't run timer → user must power cycle
- Valid sequence (battery → USB): Hardware IS charging → run timer → track toward 100%

---

## Revision History

- **April 22, 2026:** Migrated from `/memories/repo/` to `.github/skills/kb1-hardware/`
- **April 12, 2026:** Battery hardware roadmap — decision to skip SY6410 for KB1.5
- **March 22, 2026:** Audio hardware power measurements and speaker compensation model
- **December 17, 2023:** Schematic Rev B.0 finalized (KiCad 9.0.7)
