# KB1 Firmware

The KB1 firmware is a feature-rich, production-ready embedded system for the PocketMidi KB1 MIDI controller. This release delivers a comprehensive suite of musical capabilities including dual-mode keyboard operation (Scale/Chord modes with chord/strum options), flexible lever controls with advanced interpolation, customizable touch sensing, intelligent power management, and 12-channel performance sliders—all controllable wirelessly via Bluetooth Low Energy.

**Latest Release:** v1.2.5 — [Release Notes](RELEASE_NOTES_v1.2.5.md) | [v1.2.0](RELEASE_NOTES_v1.2.0.md) | [v1.1.4](RELEASE_NOTES_v1.1.4.md) | [v1.1.3](RELEASE_NOTES_v1.1.3.md) | [v1.1.2](RELEASE_NOTES_v1.1.2.md) | [v1.1.1](RELEASE_NOTES_v1.1.1.md)

## Features

### Keyboard Modes
- **Scale Mode**: Quantized note output with multiple scale types (Chromatic, Major, Minor, Pentatonic, Blues, and more)
- **Chord Mode**: Full chord playback with 10 chord types (Major, Minor, Diminished, Augmented, Sus2, Sus4, Power, Major7, Minor7, Dominant7)
- **Chord/Strum Toggle**: Switch between simultaneous chord notes or cascading strum
- **Smart Slider**: Dynamic velocity spread (Chord mode) or strum timing (Strum mode)
- **Natural/Compact Key Mapping**: Choose between spaced or dense key layouts

### Control Surface
- **2 Analog Levers**: Fully configurable CC output with range, step quantization, function modes (uni/bi-directional, momentary, toggle), value modes (jump, hook, pickup, latch), and interpolation curves (linear, S-curve, logarithmic)
- **2 Lever Push Buttons**: Independent CC mapping with trigger/momentary/toggle modes and interpolation
- **Capacitive Touch Sensor**: Adjustable threshold with CC output and multiple function modes
- **12 Performance Sliders**: Real-time CC control (51-62) with bipolar/unipolar and momentary/latched modes

### Power Management
- **Light Sleep**: Configurable timeout (3-10 minutes, default: 5 min) - triggers pulsing LED feedback and low power mode
- **Deep Sleep**: Fixed at light sleep + 90s - ensures consistent 90-second LED warning before deep sleep
- **BLE Timeout**: Adjustable Bluetooth keep-alive (5-20 minutes, default: 10 min) - web app pings prevent sleep while connected
- **Sleep Behavior**: 
  - Without web app: Automatic sleep progression after idle timeouts
  - With web app connected: Keepalive pings reset sleep timers, keeping device awake during configuration
  - BLE radio disabled before entering sleep modes
- **Automatic Wake**: Touch sensor wakes from deep sleep; any control interaction prevents sleep entry

### Bluetooth Low Energy
- **Standard BLE MIDI**: Compatible with all major DAWs and MIDI applications
- **Configuration Service**: Full wireless configuration via companion web app
- **Preset Management**: 8 on-device preset slots for storing complete configurations
- **Keep-Alive Protocol**: Connection maintenance with 10-minute firmware grace period
- **Security**: Optional pairing with secure bonding

### Storage & Presets
- **Non-Volatile Settings**: All configuration persisted to flash memory
- **8 Device Presets**: Store and recall complete configurations on-device
- **Flash Management**: Automatic save/load with wear leveling

## Technical Details

### Platform
- **Framework**: [PlatformIO](https://platformio.org) with Arduino framework
- **Target**: ESP32-based hardware (optimized for ESP32-S3)
- **Language**: C++17
- **Architecture**: Modular, object-oriented design

### Project Structure

```
src/
├── main.cpp                      # Main program loop and initialization
├── bt/                           # Bluetooth Low Energy subsystem
│   ├── BluetoothController.*     # BLE stack management
│   ├── CharacteristicCallbacks.* # BLE characteristic handlers
│   ├── PresetCallbacks.*         # Preset management handlers
│   ├── SecurityCallbacks.*       # BLE security/pairing
│   └── ServerCallbacks.*         # BLE connection lifecycle
├── controls/                     # Hardware control interfaces
│   ├── KeyboardControl.h         # Scale/Chord keyboard implementation
│   ├── LeverControls.h           # Analog lever logic
│   ├── LeverPushControls.h       # Push button handling
│   ├── OctaveControl.h           # Octave shift control
│   ├── SleepControl.h            # Power management
│   └── TouchControl.h            # Capacitive touch sensing
├── led/                          # LED feedback system
│   ├── LEDController.*           # RGB LED control
│   └── patterns/                 # Status indication patterns
├── music/                        # Music theory engine
│   └── ScaleManager.*            # Scale/chord generation
└── objects/                      # Core data structures
    ├── Constants.h               # System-wide constants
    ├── Globals.h                 # Shared state
    └── Settings.h                # Configuration structures
```

### Building & Flashing

1. Install PlatformIO:
```bash
# Via PlatformIO IDE extension for VS Code
# Or via CLI:
pip install platformio
```

2. Build the project:
```bash
cd firmware
pio run
```

3. Upload to device:
```bash
pio run --target upload
```

4. Monitor serial output:
```bash
pio device monitor
```

### Building Release Firmware

**⚠️ CRITICAL:** For distribution, always build **complete images** that include bootloader, partitions, and app.

```bash
./build_factory.sh
```

This prevents SHA-256 verification failures and bricking. See [BUILD_INSTRUCTIONS.md](BUILD_INSTRUCTIONS.md) for details.

> **Troubleshooting**: See [docs/INSTALL.md](docs/INSTALL.md) for PlatformIO upload fixes (e.g. installing `intelhex`).

### First Time Setup

**IMPORTANT: Enable Bluetooth Before Connecting**

After flashing firmware, you must enable Bluetooth to connect with the configuration app:

1. **Cross-lever gesture**: Push both levers toward each other (left lever → right, right lever → left) and **hold for 3 seconds**
2. **Watch for progressive LED feedback:**
   - Octave arrow LEDs turn ON immediately (gesture detected)
   - Pink + blue LEDs pulse with increasing speed as you hold
   - **All LEDs turn OFF** = activation complete, release levers
3. Repeat the same gesture anytime to toggle Bluetooth on/off

**Note:** The gesture is automatically cancelled if any keyboard key is pressed, preventing accidental triggers during performance.

Without enabling Bluetooth, the web configuration app will not detect your device.

### ⚠️ Battery Calibration Required

**IMPORTANT:** After flashing new firmware, the battery meter will show "uncalibrated" (gray `?` icon) until you complete a **full charge cycle**.

**Why?** The firmware cannot measure battery voltage directly—it estimates battery life by tracking usage time. A fresh firmware install doesn't know if your battery is at 100%, 50%, or 20%.

**How to Calibrate:**
1. Connect USB cable to your KB1 device
2. Leave charging for **ONE continuous 5.5+ hour session**
3. **Do NOT unplug during this time!**
4. Battery meter will automatically mark as "calibrated" and show accurate percentage
5. From this point forward, the meter tracks discharge accurately

**⚠️ CRITICAL - All-or-Nothing Calibration:**
- The 5.5 hour charge **MUST be continuous** in ONE session
- If you unplug before 5.5 hours, the timer **resets to ZERO**
- Partial charges **do NOT accumulate** (3hrs + 2.5hrs ≠ calibrated)
- This is **intentional** to ensure genuine full charge and accurate baseline
- **Only needs to happen ONCE** - future USB connections just pause discharge tracking

**Until Calibrated:** The battery meter will display a gray `?` icon and "Needs Calibration" message. Ignore battery percentage estimates until you've completed a full charge cycle.

**After Calibration:** The battery meter provides accurate estimates based on measured power consumption:
- Active mode: 95mA drain
- Light sleep: 2mA drain  
- Deep sleep: 0.014mA drain

### Configuration

Edit `platformio.ini` to customize build settings, upload port, or target board.

## Companion Web App

The firmware is designed to work seamlessly with the [KB1-config](../KB1-config) web application, which provides wireless configuration of all parameters via Web Bluetooth API. The companion app offers an intuitive interface for:

- Keyboard mode selection and chord/scale configuration
- Lever and control parameter tuning
- Performance slider setup and preset management
- Power management settings
- Device preset save/load/management

## License

See [LICENSE](LICENSE) file for details.

## Development

This firmware represents the first production release of the KB1 system. It provides a complete, feature-rich foundation for expressive MIDI performance with extensive configurability and wireless control.

