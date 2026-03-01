# KB1 Firmware

The KB1 firmware is a feature-rich, production-ready embedded system for the PocketMidi KB1 MIDI controller. This release delivers a comprehensive suite of musical capabilities including dual-mode keyboard operation (Scale/Chord modes with chord/strum options), flexible lever controls with advanced interpolation, customizable touch sensing, intelligent power management, and 12-channel performance sliders—all controllable wirelessly via Bluetooth Low Energy.

**Latest Release:** v1.1.3 — [Release Notes](RELEASE_NOTES_v1.1.3.md) | [v1.1.2](RELEASE_NOTES_v1.1.2.md) | [v1.1.1](RELEASE_NOTES_v1.1.1.md)

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
- **Light Sleep**: Configurable timeout (30-300s) for power conservation
- **Deep Sleep**: Extended timeout (120-1800s) for battery preservation
- **BLE Timeout**: Adjustable Bluetooth keep-alive (30-600s) with 10-minute grace period
- **Automatic Wake**: Resume on any control interaction

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

1. **Hold both octave buttons** (up + down) simultaneously for **3 seconds**
2. **Watch for LED confirmation:**
   - Fast blinking (pink + blue LEDs) = **Bluetooth enabled** ✓
   - Slow blinking = Bluetooth disabled
3. Repeat the same process anytime to toggle Bluetooth on/off

Without enabling Bluetooth, the web configuration app will not detect your device.

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

