# KB1 Firmware v1.1.2 Release Notes

## Overview

Version 1.1.2 is a maintenance release that maintains full compatibility with v1.1.1 while enabling new web app features for enhanced MIDI control capabilities.

## Firmware Changes

**Version Identification:**
- Firmware version string: `1.1.2`
- Version characteristic reports correctly via BLE
- Full backward compatibility with v1.1.1 configuration protocol

**No functional firmware changes** — This release maintains identical hardware control behavior and BLE protocol as v1.1.1. The version bump enables the web app to detect compatibility with new UI features.

## Web App Enhancements (KB1-config)

### Performance Sliders: Master Mixer Mode

**Dual-Mode Performance Control** — The 12-slider performance interface now supports two distinct control modes:

**FX Mode (Default):**
- CC 51-62: Performance effects control
- 4 groups of 3 sliders with color coding
- Bipolar/unipolar support
- Same behavior as v1.1.1

**MIX Mode (New):**
- **Global Mixer (Sliders 1-4):**
  - Delay Send (CC 79) — Orange
  - Reverb Send (CC 80) — Orange  
  - Dry Level (CC 81) — Orange
  - Line Level (CC 82) — Orange
- **Track Mixer (Sliders 5-12):**
  - Track 1-8 Volumes (CC 71-78)
  - Color coding: Red (tracks 1-2), Green (3-4), Cyan (5-6), Violet (7-8)
- **Automatic Ungrouping:** Global mixer controls operate independently (no gang linking)
- **Unipolar Lock:** All mixer controls forced to 0-127 range (matches Polyend Tracker spec)
- **Selective Linking:** Link buttons only visible for track volume controls

**Mode Toggle:**
- Compact FX|MIX button in header
- Mode persists to localStorage
- Labels update dynamically (setup/live modes)
- Conditional link visibility based on mode

### Scale Settings Improvements

**Compact Messaging:**
- Chromatic scale lock message shortened to "Chrom locked to C"
- Saves screen space on mobile layouts

### UI Refinements

**Performance Sliders Setup Mode:**
- Mom/Lat toggle moved to left position
- Uni/Bi toggle moved to right position  
- Reduced toggle spacing (0.25rem gap)
- Tighter left alignment (-1rem margin)

**Header Optimization:**
- Reduced horizontal padding on Live/Cal buttons
- More compact mobile layout
- Consistent button sizing

## Target Hardware

- **MCU:** Seeed XIAO ESP32-S3
- **Flash Usage:** 76.4% (1,001,901 bytes)
- **RAM Usage:** 15.6% (51,156 bytes)

## Installation

### Method 1: ESPConnect (Recommended)

Flash firmware directly from your web browser using Web Serial API.

**Requirements:** Chrome, Edge, or Opera browser with Web Serial API support.

**Quick Steps:**

1. **Connect Hardware:**
   - Plug KB1 into your computer via USB-C
   - Navigate to [ESPConnect](https://thelastoutpostworkshop.github.io/ESPConnect/)
   - Click **CONNECT** and select your KB1 device

2. **Backup Flash (Important!):**
   - Click **Flash Tools** → **DOWNLOAD FLASH BACKUP**
   - Save the backup file to a safe location

3. **Load Firmware:**
   - Browse to `KB1-firmware-v1.1.2.bin`
   - Verify flash offset is `0x0` (default)

4. **Flash:**
   - ✅ **Check "Erase entire flash before writing"**
   - Click **FLASH FIRMWARE**
   - Wait 30-60 seconds for completion

5. **Verify:**
   - Open web app at `https://yourdomain.com/kb1-config/`
   - Connect via Bluetooth
   - Verify version shows `1.1.2` in device info

### Method 2: esptool.py (Advanced)

For users with PlatformIO or Python tooling installed:

```bash
# Install esptool (if not already installed)
pip install esptool

# Flash complete factory image
esptool.py --chip esp32s3 --port /dev/cu.usbmodem* \
  --baud 921600 --before default_reset --after hard_reset \
  write_flash --flash_mode dio --flash_freq 80m --flash_size 8MB \
  0x0 KB1-firmware-v1.1.2.bin
```

**Port Selection:**
- macOS: `/dev/cu.usbmodem*` or `/dev/tty.usbmodem*`
- Linux: `/dev/ttyUSB0` or `/dev/ttyACM0`  
- Windows: `COM3`, `COM4`, etc.

## Compatibility

**Firmware Backward Compatibility:**
- ✅ All v1.1.1 configurations load correctly
- ✅ All v1.1.1 presets function identically
- ✅ BLE protocol unchanged

**Web App Requirements:**
- v1.1.2 web app detects firmware version automatically
- New mixer mode only shown with v1.1.2+ firmware
- Full backward compatibility with v1.1.1 firmware (FX mode only)

## Upgrade Path

**From v1.1.1:**
1. Backup flash using ESPConnect
2. Flash KB1-firmware-v1.1.2.bin
3. Reconnect in web app — settings preserved via device flash

**From Earlier Versions:**
1. Backup your configuration in web app (export presets if available)
2. Flash KB1-firmware-v1.1.2.bin
3. Reconfigure device settings in web app

## Known Issues

None at this time. This release maintains the stability of v1.1.1.

## Version Hash

**MD5:** `3381af6c901099e1c08f0114cb09a88e`

## Build Information

- **Build Date:** Feb 25 2026 21:19:37
- **Build Environment:** PlatformIO 6.1.18
- **ESP-IDF:** 4.9.0
- **Framework:** Arduino ESP32 3.20017

## Support

For issues or questions:
- GitHub Issues: [Your Repository]
- Documentation: See `BUILD_INSTRUCTIONS.md`
- Detailed v1.1.1 guide: See `RELEASE_NOTES_v1.1.1.md` for complete hardware/firmware documentation

---

**Note:** This is a web app feature release. The firmware change is version identification only. All core MIDI functionality remains identical to v1.1.1.
