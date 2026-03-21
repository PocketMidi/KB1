# KB1 Firmware v1.4.0 Release Notes
**Release Date:** March 21, 2026

## 🎯 What's New

### Battery Monitoring System
- **Automatic battery tracking** with USB charging calibration
- **5.5 hour USB charging** required for initial calibration (one-time)
- **Real-time battery percentage** and remaining runtime estimates
- **Multi-session charging** - charge progress persists across power cycles
- **Host-agnostic charging** - works with Mac, PC, Android, iOS, power banks, or any USB power source
- **Pink ↔ Blue LED breathing** during USB charging mode
- **Web app integration** - view status, calibrate, and recalibrate from configurator

### Multi-Session Charging Intelligence
The charging system is designed for real-world usage:
- **Charge anywhere, anytime** - progress accumulates toward 5.5hr goal
- **Unplug freely** - charging time persists in flash memory
- **Resume seamlessly** - plug back in hours or days later, continues from where it left off
- **Cross-device compatible** - start charging on Mac, continue on PC, finish on power bank
- **Power-loss resilient** - saves progress every 30 seconds during charging

**Example Usage:**
1. Plug into laptop USB → charge 2 hours while working
2. Unplug and move to another location
3. Return home, plug into desktop → continues from 2 hours
4. Unplug for the night
5. Next day, plug into power bank → completes remaining 1.5 hours
6. Total accumulated: 5.5 hours → calibration complete! ✓

### Speaker Compensation (Web App)
- **Manual usage reporter** - estimate battery drain from speaker amp
- **0-120 minute range** in 5-minute increments
- **135mA average drain** calculation for more accurate runtime
- **App-only tracking** - analog speaker usage cannot be auto-detected

### Battery Modal (Web App)
- **Visual battery indicator** with percentage or icon display
- **Charging progress** with estimated completion time
- **Calibrate/Recalibrate buttons** for one-click battery reset
- **Speaker compensation slider** with purple/pink gradient meter
- **Collapsible charging instructions** for first-time users
- **Auto-opens when monitoring enabled** for immediate feedback

### UI Improvements (Web App)
- **Battery monitoring toggle** (System Settings) - hide/show battery icon
- **Toggle defaults to OFF** - user must explicitly enable
- **Battery icon conditionally visible** in navigation bar
- **Battery data always tracked** in background (regardless of UI toggle)

## ⚡ Battery Features

### Charging Mode
- **Pink ↔ Blue breathing LEDs** (1200ms pink, 600ms blue cycle)
- **Sleep prevented** during charging to allow full 5.5hr calibration
- **30-second progress updates** via serial (e.g., `Chg: 476/19800s`)
- **Automatic completion** - exits charging mode when 5.5hr reached
- **Top-up charging supported** - partial charges on calibrated battery update percentage

### Power Management
- **95mA active drain** - normal playing mode
- **10mA light sleep** - idle but ready to wake
- **1mA deep sleep** - power-saving mode after timeout
- **135mA speaker amp** - PAM8406 average during playback

### Battery States
- **254 = Uncalibrated** (?) - needs full 5.5hr charge
- **0-100 = Calibrated** (%) - shows accurate percentage and runtime

### Charging Sequence Requirements
**Valid charging sequence:**
1. Power ON device first (press power button)
2. THEN plug in USB cable
3. Pink ↔ Blue LEDs start breathing immediately
4. Unplug anytime - progress saved
5. Re-plug anytime - resumes from saved progress

**Invalid (bypass mode):**
- USB plugged in at boot → device enters bypass mode
- No charging, no LEDs, no progress accumulation
- Unplug USB → power cycle device → plug back in (correct sequence)

## 🔧 Technical Details

### NVS Persistence
Battery state saved to flash every 30 seconds during charging:
- `batPct` - Battery percentage (0-100 or 254=uncalibrated)
- `batFull` - Full charge flag
- `batDischMs` - Accumulated discharge time
- `batCalTime` - Calibration timestamp
- `batAccChgMs` - **Accumulated charge time across all sessions**

### Multi-Session Architecture
- **Accumulated charge time** (`accumulatedChargeMs`) - persists in NVS
- **Session start time** (`chargeSessionStartMs`) - resets each plug-in
- **Total charge** = accumulated + (now - sessionStart)
- **Saves every 30s** - worst-case loss: 30 seconds on power failure
- **Never clears** except on: calibration complete, partial charge on calibrated battery, or manual reset

### Charging Progress Calculation
- **Uncalibrated battery**: Tracks time toward 5.5hr goal, no percentage shown
- **Calibrated battery charging**: Updates percentage based on mAh gained (100mA * time)
- **Calibrated battery discharging**: Real-time tracking based on active/sleep drain rates
- **Speaker compensation**: Subtracts user-reported speaker minutes from runtime estimate

### Serial Output Format (Compact)
```
USB plug:       USB connected - charging started (valid sequence)
Charging:       Chg: 476/19800s
Save:           Saved
Event marker:   CHARGE_60S | USB:1 | Chg:1 | Slp:1
Completion:     Battery fully charged! Calibration complete.
Mode end:       Charging mode ended - LEDs off, sleep enabled
```

## 📊 Performance Impact

### Serial Output Optimization
- Battery messages use compact format consistent with v1.3.0 philosophy
- **Before**: `Charging: 476s / 19800s (19323s remaining)` + `Battery state saved: Uncalibrated`
- **After**: `Chg: 476/19800s` + `Saved`
- **Impact**: ~40ms saved per update (every 30s) = <0.01% overhead
- **Zero overhead for end users**: Serial output only active when terminal connected

### LED Breathing Performance
- Pink/Blue LEDs use Task 1 (priority 1) on Core 1
- 1200ms pink, 600ms blue fade cycles
- Stops immediately when charging complete or USB unplugged
- Triple-clear LED buffer ensures clean stop

### BLE Characteristics (Web App Integration)
- **Battery Status** (read + notify) - percentage, runtime, USB state
- **Battery Control** (write) - reset/recalibrate command (0x01)
- **Firmware Version** (read) - "1.4.0"

## 🔄 Upgrade Notes

### From v1.3.0 or earlier:
- Flash v1.4.0 firmware
- Battery starts in uncalibrated state (254)
- Charge for 5.5 hours total (can be split across multiple sessions)
- After calibration, battery percentage and runtime estimates available

### Recalibration:
- Click "Recalibrate" in web app battery modal
- Clears all battery state to uncalibrated
- If USB already connected, charging mode starts immediately
- Otherwise, follow standard charging sequence (power ON → plug USB)

## 📝 Architecture Notes

### Core Allocation (unchanged from v1.3.0)
- **Core 1**: BLE, I2C, Touch, Input Reading, LED Control
- **Core 0**: MIDI, Arpeggiator, Sleep Management

### Compatibility
- Fully backwards compatible with v1.3.x
- No breaking changes to BLE protocol
- Existing MIDI implementation unchanged
- New battery characteristics are optional (web app feature)

### Web App Version Required
- **KB1 Configurator v2.0+** for full battery UI support
- Older web app versions will not show battery features but firmware still functions normally

## 🎨 LED Conventions (Updated)

### Existing Conventions (from v1.3.0)
- **Pink LED** = Up / Forward / Increase / Positive direction
- **Blue LED** = Down / Reverse / Decrease / Negative direction

### New: Charging Mode
- **Pink ↔ Blue breathing** = USB charging active
- 1200ms pink fade → 600ms blue fade → repeat
- Overrides all other LED modes until charging complete

## ⚠️ Known Limitations

### Charging Requirements
 Must use a computer USB port or a charger that provides valid USB data signaling (not just 5V power)
 Device must be powered ON before plugging USB
 Total 5.5 hours charging required for initial calibration
 Charging LEDs require valid power-on sequence (not bypass mode)
 **Host-agnostic charging** - works with Mac, PC, Android, iOS, or any USB port that provides valid USB data signaling (not just power)
 **Cross-device compatible** - start charging on Mac, continue on PC, finish on any computer USB port
- Speaker usage is analog and cannot be auto-detected
- User must manually report speaker minutes in web app
 5. Next day, plug into another computer USB port → completes remaining 1.5 hours
- Compensation range: 0-120 minutes maximum
- Toggle only controls UI visibility (icon + modal)
- Battery tracking always runs in background
- Web app connection required to view battery status

---

**Firmware Size:**
- Flash: 1,021,389 bytes (77.9% of 1,310,720 bytes)
- RAM: 51,388 bytes (15.7% of 327,680 bytes)

**Build Info:**
- Platform: ESP32-S3 @ 240MHz
- Framework: Arduino ESP32 v3.20017.241212
- Build Date: March 21, 2026
