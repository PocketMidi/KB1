# KB1 Firmware v1.1.3 Release Notes

## Overview

Version 1.1.3 is a bugfix release that corrects chord mode behavior to ensure proper chromatic chord voicings regardless of the selected scale.

## Firmware Changes

**Version Identification:**
- Firmware version string: `1.1.3`
- Build date: Feb 28 2026
- Full backward compatibility with v1.1.2 and v1.1.1

### Bug Fix: Chord Mode Scale Quantization

**Issue:** In v1.1.2 and earlier, chord mode was incorrectly applying scale quantization to chord notes, resulting in incorrect chord voicings when non-chromatic scales were selected.

**Fix:** Chord mode now uses chromatic notes directly without scale quantization. This ensures:
- Correct chord intervals (e.g., Major = root + 4 semitones + 7 semitones)
- Proper chord voicings across all 10 chord types
- Scale selection only affects Scale Mode, not Chord Mode
- Chromatic harmony regardless of active scale

**Affected Code:** [KeyboardControl.h](src/controls/KeyboardControl.h)

**Technical Details:**
- Removed `_scaleManager.quantizeNote()` calls from chord mode logic
- Root note calculation simplified to: `rootNote = note + (octave * 12)`
- Applies to both note-on and note-off events
- No impact on Scale Mode behavior

## Downloads

**Factory Image:**
- File: `KB1-firmware-v1.1.3.bin` (1.0MB)
- Flash address: `0x0`
- MD5: `3202608109b018580e59dfb2d4c0e725`

## Installation

### Method 1: ESPConnect (Recommended)

**Browser-based flashing tool** — No software installation required.

1. Connect KB1 via USB-C
2. Open [ESPConnect](https://thelastoutpostworkshop.github.io/ESPConnect/)
3. Click Flash Tools → Download Flash Backup (save it!)
4. Load `KB1-firmware-v1.1.3.bin`
5. Check "Erase entire flash" and click "Flash Firmware"
6. Wait for completion (~30 seconds)

**Flash Settings:**
```
Flash address: 0x0
File: KB1-firmware-v1.1.3.bin
Erase flash: Yes (check "Erase entire flash before writing")
```

### Method 2: esptool.py (Advanced)

```bash
esptool.py --chip esp32s3 --port /dev/ttyACM0 \
  erase_flash

esptool.py --chip esp32s3 --port /dev/ttyACM0 \
  --baud 921600 write_flash \
  0x0 KB1-firmware-v1.1.3.bin
```

## Verification

After flashing:
1. Connect KB1 to BLE web app
2. Check firmware version displays `1.1.3` in device info
3. Test chord mode with different scales selected
4. Verify chords sound correct (e.g., C Major = C-E-G, not quantized to scale)

## Important Notes

⚠️ **Always flash the complete factory image** (`KB1-firmware-v1.1.3.bin`) at offset `0x0`. Never flash app-only binaries, as this will cause SHA-256 verification failures.

✅ **Complete firmware includes:**
- Bootloader (0x0)
- Partition table (0x8000)
- Application (0x10000)

## Compatibility

- **Web App:** Compatible with all KB1-config versions
- **Hardware:** Seeed XIAO ESP32-S3
- **BLE Protocol:** Unchanged from v1.1.1/v1.1.2
- **Configuration:** All settings preserved during upgrade

## Upgrade Paths

**From v1.1.1 or v1.1.2:**
1. Optional: Download backup via ESPConnect
2. Flash KB1-firmware-v1.1.3.bin
3. All settings automatically preserved

**From v1.1.0 or earlier:**
1. Download backup via ESPConnect (recommended)
2. Flash KB1-firmware-v1.1.3.bin
3. Reconfigure settings if needed

## Changes Summary

**Fixed:**
- Chord mode now uses chromatic notes (no scale quantization)
- Correct chord voicings across all scales

**Unchanged:**
- Scale mode behavior (quantization still applies)
- BLE protocol and characteristics
- All other keyboard, lever, touch, and slider functionality
- Power management and sleep behavior

## Known Issues

None at this time.

## Support

For issues or questions:
- GitHub Issues: [KB1 Firmware Repository]
- Documentation: See README.md and other release notes

---

**Release Date:** February 28, 2026  
**Build System:** PlatformIO + ESP-IDF  
**Tested On:** Seeed XIAO ESP32-S3
