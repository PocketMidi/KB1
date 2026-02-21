# KB1 Firmware v1.1.1 Release Notes

## 🎹 New Features

### Chord Mode
Transform your KB1 into a chord machine! Play full chords from any key on the keyboard.

- **10 Chord Types**: Major, Minor, Diminished, Augmented, Sus2, Sus4, Power, Major7, Minor7, Dom7
- **Strum Mode**: Optional cascading note triggering with configurable speed (5-100ms)
- **Velocity Spread**: Dynamic voicing with exponential velocity reduction per note (0-100%)
- **Seamless Integration**: Easy mode switching between scale and chord modes via the config app

### Performance Improvements
- **Enhanced BLE Stability**: Throttled serial output to prevent buffer overflow during heavy slider usage
- **Optimized MIDI Processing**: Improved chord note tracking and management

## 🔧 Technical Details

**Memory Usage:**
- Flash: 29.9% (1MB / 3.34MB)
- RAM: 15.6% (51KB / 327KB)

**BLE Protocol:**
- New Chord Settings Characteristic: `4a8c9f2e-1b7d-4e3f-a5c6-d7e8f9a0b1c2`
- Backward compatible with existing presets

**Default Settings:**
- Strum speed: 45ms
- Velocity spread: 8%
- Mode: Scale (chord mode available via config app)

## 📦 Installation

### Method 1: ESPConnect (Recommended - No Tools Required)
1. Visit [ESPConnect](https://espconnect.io/)
2. Click "Connect" and select your KB1 device
3. Upload `KB1-firmware-v1.1.1.bin` from this release
4. Device will restart automatically

### Method 2: PlatformIO (For Developers)
```bash
cd firmware
pio run --target upload --environment seeed_xiao_esp32s3
```

## 🌐 Configuration App

Access the web-based configuration app at:
**[pocketmidi.github.io/KB1/KB1-config](https://pocketmidi.github.io/KB1/KB1-config)**

Configure chord settings, scales, MIDI mappings, and more directly from your browser via BLE.

## 📝 Documentation

- [Main README](../README.md)
- [Preset Implementation Guide](PRESET_IMPLEMENTATION.md)
- [Config App Documentation](../KB1-config/README.md)

## ✨ What's New from v1.0

- Complete chord mode implementation with 10 chord types
- Strum feature for guitar-like chord articulation
- Velocity spread for natural, dynamic chord voicing
- Root note sharing between scale and chord modes
- Enhanced serial output throttling for device stability
- Comprehensive UI controls in config app

## 🐛 Bug Fixes

- Fixed serial buffer overflow during heavy MIDI traffic
- Corrected BLE characteristic initialization order
- Resolved preset save/load for chord settings
- Fixed memory management for chord note tracking

## 🙏 Feedback & Support

Found a bug or have a feature request? Open an issue on [GitHub](https://github.com/pocketmidi/KB1/issues).

---

**Compatibility:** ESP32-S3 (Seeed Xiao ESP32S3)  
**Build Date:** February 21, 2026  
**Firmware Size:** 978KB  
**MD5 Checksum:** `cf9e8c7cb036ac68110d425100ac42b8`
