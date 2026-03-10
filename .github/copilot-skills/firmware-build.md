---
applyTo:
  - "firmware/**/*.{cpp,h,ini,sh}"
  - "firmware/BUILD_INSTRUCTIONS.md"
  - "firmware/build_complete.sh"
---

# KB1 Firmware Build Expert

Expert knowledge for building, flashing, and releasing KB1 ESP32-S3 firmware.

## PRIMARY RULE: Always Build Complete Images

**⚠️ CRITICAL:** Never suggest or create app-only binaries for distribution.

```bash
# ✅ CORRECT - Complete image with bootloader
./build_complete.sh

# ❌ WRONG - App-only build (will brick devices!)
pio run --environment seeed_xiao_esp32s3
```

## ESP32-S3 Memory Layout

```
Address    Component           Size      Description
-------    ---------           ----      -----------
0x0        Bootloader          ~14KB     Second stage bootloader
0x8000     Partition Table     ~3KB      Flash partition map
0x10000    Application         ~978KB    Main firmware code
```

**Key differences from original ESP32:**
- Bootloader at **0x0** (not 0x1000!)
- Must specify `--chip esp32s3` in esptool commands

## Build Process

### Automated Build (Recommended)

```bash
cd firmware
./build_complete.sh
```

**Output:** `KB1-firmware-vX.X.X.bin` (~1MB complete image)

**What it does:**
1. Cleans previous build artifacts
2. Builds firmware with PlatformIO
3. Merges bootloader + partitions + app
4. Verifies output file integrity
5. Shows file size and MD5 checksum

### Manual Build (Advanced)

```bash
# Step 1: Build firmware components
python3 -m platformio run --environment seeed_xiao_esp32s3

# Step 2: Locate generated files
# .pio/build/seeed_xiao_esp32s3/bootloader.bin  (~14KB)
# .pio/build/seeed_xiao_esp32s3/partitions.bin  (~3KB)
# .pio/build/seeed_xiao_esp32s3/firmware.bin    (~978KB)

# Step 3: Merge into complete image
python3 ~/.platformio/packages/tool-esptoolpy/esptool.py \
  --chip esp32s3 merge_bin \
  -o KB1-firmware-v1.2.5.bin \
  --flash_mode dio \
  --flash_freq 80m \
  --flash_size 8MB \
  0x0 .pio/build/seeed_xiao_esp32s3/bootloader.bin \
  0x8000 .pio/build/seeed_xiao_esp32s3/partitions.bin \
  0x10000 .pio/build/seeed_xiao_esp32s3/firmware.bin

# Step 4: Verify
ls -lh KB1-firmware-v1.2.5.bin
md5 KB1-firmware-v1.2.5.bin
```

## Flashing Methods

### Method 1: ESPConnect (Recommended for Users)

**URL:** https://thelastoutpostworkshop.github.io/ESPConnect/

**Steps:**
1. Connect KB1 via USB-C
2. Open ESPConnect in Chrome/Edge
3. Click "Flash Tools" → "Download Flash Backup" (save it!)
4. Load firmware .bin file
5. **Check "Erase entire flash"**
6. Click "Flash Firmware"
7. Wait for completion (~30 seconds)

**Advantages:**
- No software installation required
- Works on any platform (Windows/macOS/Linux)
- Browser-based, user-friendly
- Built-in flash backup

### Method 2: PlatformIO CLI (Development)

```bash
# Development upload (app only - for rapid iteration)
pio run --target upload --environment seeed_xiao_esp32s3

# Production upload (complete image)
pio run --target upload --environment seeed_xiao_esp32s3
```

**Port detection:**
- macOS: `/dev/cu.usbmodem*`
- Linux: `/dev/ttyUSB0` or `/dev/ttyACM0`
- Windows: `COM3` or higher

### Method 3: esptool.py (Advanced)

```bash
esptool.py --chip esp32s3 \
  --port /dev/cu.usbmodem14201 \
  --baud 921600 \
  --before default_reset \
  --after hard_reset \
  write_flash 0x0 KB1-firmware-v1.2.5.bin
```

**Baud rates:**
- 921600 (fastest, may fail on some systems)
- 460800 (reliable fallback)
- 115200 (slowest, most compatible)

## Version Management

### Updating Firmware Version

**Example:** Bumping from v1.2.5 to v1.2.6

1. **Update Constants.h:**
```cpp
// firmware/src/objects/Constants.h
#define FIRMWARE_VERSION_MAJOR 1
#define FIRMWARE_VERSION_MINOR 2
#define FIRMWARE_VERSION_PATCH 6
```

2. **Update build script:**
```bash
# firmware/build_complete.sh
FIRMWARE_VERSION="v1.2.6"
```

3. **Update main.cpp:**
```cpp
// firmware/src/main.cpp
SERIAL_PRINTLN("KB1 FIRMWARE v1.2.6 - WITH CHORD MODE");
```

4. **Update README:**
```markdown
# firmware/README.md
**Latest Release:** v1.2.6 — [Release Notes](RELEASE_NOTES_v1.2.6.md)
```

5. **Update web app version:**
```json
// KB1-config/package.json
{
  "version": "1.2.6"
}
```

6. **Create release notes:**
```bash
touch firmware/RELEASE_NOTES_v1.2.6.md
```

7. **Build and test:**
```bash
cd firmware
./build_complete.sh
# Flash to device and test!
```

## Common Build Issues

### Issue: "esptool not found"

**Solution:**
```bash
# Install PlatformIO packages
pio pkg install --platform espressif32

# Or find esptool manually
find ~/.platformio -name "esptool.py"
```

### Issue: "SHA-256 mismatch" on boot

**Cause:** Flashed app-only binary without bootloader

**Solution:** Flash complete image at offset 0x0 with erase

### Issue: "Serial port not found"

**macOS:**
```bash
# List USB devices
ls /dev/cu.*

# Give terminal access to USB
# System Preferences → Security & Privacy → Files and Folders
```

**Linux:**
```bash
# Add user to dialout group
sudo usermod -a -G dialout $USER
# Log out and back in

# Check port permissions
ls -l /dev/ttyUSB0
```

### Issue: "Upload failed" or timeout

**Solutions:**
1. Hold BOOT button while connecting USB
2. Try lower baud rate (460800 or 115200)
3. Use different USB cable (data cable, not charge-only)
4. Try different USB port (avoid hubs)

### Issue: Build fails with "region `iram0_0_seg' overflowed"

**Cause:** Code size exceeded available RAM

**Solutions:**
1. Enable compiler optimizations (`-Os`)
2. Move constants to PROGMEM/Flash
3. Reduce debug output
4. Review large data structures

## PlatformIO Configuration

### platformio.ini Key Settings

```ini
[env:seeed_xiao_esp32s3]
platform = espressif32
board = seeed_xiao_esp32s3
framework = arduino

; Flash settings
board_build.flash_mode = dio
board_build.flash_freq = 80m
board_build.flash_size = 8MB

; Partition scheme
board_build.partitions = default.csv

; Upload settings
upload_speed = 921600
monitor_speed = 115200

; Build flags
build_flags = 
    -DCORE_DEBUG_LEVEL=0
    -DARDUINO_USB_CDC_ON_BOOT=1
```

## Release Checklist

Before creating a GitHub release:

- [ ] Version bumped in all required files (see Version Management)
- [ ] Clean build completed (`./build_complete.sh`)
- [ ] Output file size reasonable (~1MB)
- [ ] MD5 checksum documented
- [ ] Tested on actual hardware (not just compile)
- [ ] Serial debugging verified
- [ ] BLE connection tested
- [ ] Settings persist across reboots
- [ ] Complete image (not app-only)
- [ ] Release notes created
- [ ] GitHub release created with attached .bin file

## Flash Memory Usage

**Current usage (v1.2.5):**
- **Flash:** ~76% full (1MB / 1.3MB available)
- **RAM:** ~16% usage (51KB / 327KB available)

**Monitoring:**
```bash
# Check sizes after build
pio run --target size --environment seeed_xiao_esp32s3

# Or use build script (shows size automatically)
./build_complete.sh
```

**Optimization tips:**
- Use `PROGMEM` for large constant arrays
- Minimize String usage (prefer C strings)
- Enable compiler optimization (`-Os`)
- Remove debug Serial.print() calls

## Troubleshooting Boot Issues

### Device won't boot after flash

1. **Check power supply:**
   - USB cable must provide sufficient current
   - Try different USB port or power adapter

2. **Erase flash and reflash:**
```bash
esptool.py --chip esp32s3 --port /dev/cu.usbmodem14201 erase_flash
esptool.py --chip esp32s3 --port /dev/cu.usbmodem14201 write_flash 0x0 KB1-firmware-v1.2.5.bin
```

3. **Check serial output:**
```bash
pio device monitor
# Look for boot errors, panic messages, or stack traces
```

4. **Restore factory bootloader:**
   - Use ESPConnect to flash original backup
   - Or flash fresh Arduino bootloader from Seeed

## Best Practices

1. **Always test on real hardware** before releasing
2. **Use `build_complete.sh`** for distribution builds
3. **Document MD5 checksums** for releases
4. **Back up flash** before flashing (ESPConnect)
5. **Keep build logs** for debugging
6. **Test at lower baud rates** if upload fails
7. **Monitor flash usage** before adding features
8. **Use descriptive version tags** (v1.2.5, not latest)

---

**Remember:** The ESP32-S3 bootloader is at 0x0, not 0x1000. Always use complete images for distribution!
