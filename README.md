# KB1 - Pocket MIDI Keyboard Controller

A pocket-sized, feature-rich MIDI keyboard controller designed for the [Polyend Tracker Mini](https://polyend.com/tracker-mini/). This first release delivers professional-grade performance controls, wireless configuration, and extensive musical capabilities in an ultra-portable package.

![KB1 banner](assets/banner_1.jpg)

## System Overview

The KB1 system consists of three integrated components working together to provide a complete MIDI performance solution:

**Hardware**
- 19-key velocity-sensitive keyboard with dual operating modes (Scale/Chord)
- 2 analog levers with push buttons for expressive control
- Capacitive touch sensor for additional expression
- Built-in mini speakers for audio monitoring
- ESP32-based with Bluetooth Low Energy connectivity
- Rechargeable battery with intelligent power management

**Firmware** ([details](firmware/README.md))
- Dual keyboard modes: Scale (quantized to musical scales) and Chord (10 chord types with chord/strum options)
- Fully configurable controls with advanced interpolation curves and function modes
- 12 real-time performance sliders (CC 51-62) with bipolar/unipolar and momentary/latched modes
- 8 on-device preset slots for storing complete configurations
- Wireless BLE configuration and standard MIDI output

**Web Configuration App** ([details](KB1-config/README.md))
- Browser-based configuration tool (no installation required)
- Real-time parameter editing over Bluetooth Low Energy
- 12-slider performance interface with mobile live mode
- Preset management with unlimited browser-stored configurations
- Works on desktop and mobile (Chrome, Edge, Opera)

## Quick Start

### 1. Flash Firmware to Your Device

Download the latest firmware from [Releases](https://github.com/PocketMidi/KB1/releases) (e.g., `KB1-firmware-v1.1.1.bin`)

**Flash using ESPConnect (Chrome/Edge):**
1. Connect KB1 to your computer via USB-C
2. Open [ESPConnect](https://thelastoutpostworkshop.github.io/ESPConnect/)
3. Click **CONNECT** and select your KB1 device
4. Upload the .bin file using the upload icon
5. Wait ~30 seconds for completion

Tutorial: [ESPConnect video guide](https://youtu.be/-nhDKzBxHiI)

### 2. Configure Over Bluetooth

**Launch the web app**: [pocketmidi.github.io/KB1/KB1-config](https://pocketmidi.github.io/KB1/KB1-config)

1. Click **DISCONNECTED** (top-right) and pair with "KB1"
2. Settings load automatically from device
3. Configure keyboard modes, controls, scales, and performance sliders
4. Click **Save to Device** to persist changes to flash memory

**Performance Sliders**: Switch to the SLIDERS tab for 12 real-time CC controllers. On mobile, rotate to landscape for fullscreen live mode.

## 📖 Documentation

- **[Configuration App Guide](KB1-config/README.md)** - Complete web app documentation
- **[Firmware Development](firmware/README.md)** - Build and development setup
- **[Hardware Design](hardware/)** - Schematics and PCB files

## 🛠️ Advanced: Build from Source

**Firmware:**
```Documentation

- [Configuration App Guide](KB1-config/README.md) - Complete web app documentation with usage examples
- [Firmware Documentation](firmware/README.md) - Technical details, features, and build instructions
- [Hardware Design](hardware/) - Schematics and PCB files

## Building from Source

**Firmware** (PlatformIO):
```bash
cd firmware
pio run --target upload
```

**Web App** (Vite + Vue 3):
```bash
cd KB1-config
npm install
npm run build
```e

- **Software & Firmware**: MIT License (see LICENSE)
- **Hardware Designs**: CERN Open Hardware Licence v2 – Strongly Reciprocal (see hardware/LICENSE-CERN-OHL-S.txt)

---

Built for the Polyend Tracker Mini community

