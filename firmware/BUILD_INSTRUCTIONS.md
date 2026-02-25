# KB1 Firmware Build Instructions

## ⚠️ CRITICAL: Always Build Factory Images

**Never distribute app-only `.bin` files!** This will cause SHA-256 verification failures and brick devices.

## Quick Build (Recommended)

Use the automated build script:

```bash
./build_factory.sh
```

This creates a complete firmware image: `KB1-firmware-vX.X.X.bin`

## Manual Build Process

If you need to build manually:

### 1. Build the firmware components

```bash
python3 -m platformio run --environment seeed_xiao_esp32s3
```

This generates three files in `.pio/build/seeed_xiao_esp32s3/`:
- `bootloader.bin` (14KB)
- `partitions.bin` (3KB)
- `firmware.bin` (978KB)

### 2. Merge into factory image

```bash
python3 ~/.platformio/packages/tool-esptoolpy/esptool.py \
  --chip esp32s3 merge_bin \
  -o KB1-firmware-vX.X.X.bin \
  --flash_mode dio \
  --flash_freq 80m \
  --flash_size 8MB \
  0x0 .pio/build/seeed_xiao_esp32s3/bootloader.bin \
  0x8000 .pio/build/seeed_xiao_esp32s3/partitions.bin \
  0x10000 .pio/build/seeed_xiao_esp32s3/firmware.bin
```

### 3. Verify the firmware image

```bash
ls -lh KB1-firmware-vX.X.X.bin
strings KB1-firmware-vX.X.X.bin | grep "KB1 FIRMWARE"
md5 KB1-firmware-vX.X.X.bin
```

## ESP32-S3 Memory Layout

The Seeed XIAO ESP32-S3 uses this flash layout:

```
Address    Component           Size
-------    ---------           ----
0x0        Bootloader          ~14KB
0x8000     Partition Table     ~3KB
0x10000    Application         ~978KB
```

## Why Complete Images Are Required

**App-only firmware:**
- ❌ Causes SHA-256 verification failures
- ❌ Bootloader mismatch → boot loops
- ❌ Bricks devices

**Complete firmware (bootloader + partitions + app):**
- ✅ Includes all required components
- ✅ Correct SHA-256 digest embedded
- ✅ Works on blank or previously flashed chips
- ✅ Flash once at offset 0x0

## Flashing Instructions

### For Users (ESPConnect/Web Flasher)
```
Flash address: 0x0
File: KB1-firmware-vX.X.X.bin
```

### For Developers (PlatformIO)
```bash
platformio run --target upload --environment seeed_xiao_esp32s3
```

### For Developers (esptool)
```bash
esptool.py --chip esp32s3 --port /dev/ttyUSB0 \
  --baud 921600 --before default_reset --after hard_reset \
  write_flash 0x0 KB1-firmware-vX.X.X.bin
```

## Version Updates

When updating firmware version:

1. Edit `src/main.cpp` - update version string:
   ```cpp
   SERIAL_PRINTLN("KB1 FIRMWARE vX.X.X - WITH CHORD MODE");
   ```

2. Edit `build_factory.sh` - update version:
   ```bash
   FIRMWARE_VERSION="vX.X.X"
   ```

3. Run `./build_factory.sh` to create new factory image

4. Test on device before release!

## Release Checklist

- [ ] Version string updated in `main.cpp`
- [ ] Version updated in `build_factory.sh`
- [ ] Clean build completed
- [ ] Complete image created (includes bootloader)
- [ ] Tested on actual hardware
- [ ] MD5 checksum documented
- [ ] GitHub release created with complete image
- [ ] Flash instructions specify offset 0x0

---

**Remember:** The ESP32-S3's bootloader is at 0x0, not 0x1000 like original ESP32!
