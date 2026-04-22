# KB1 Firmware Development Guidelines

## Critical Constraints - MUST FOLLOW

### ESP32-S3 Architecture Rules

**NEVER move these off Core 1:**
- **BLE Stack** - ESP-IDF hardware limitation, causes disconnects on Core 0
- **Touch Sensor** - Hardware peripheral returns garbage on Core 0  
- **ALL I2C Tasks** - `readInputs` + `ledTask` MUST be same core
  - MCP23017 GPIO expanders (buttons, levers, LEDs)
  - Cross-core I2C causes: `Unfinished Repeated Start transaction!` crashes

**NEVER split I2C devices across cores:**
- `mcp_U1` and `mcp_U2` both require Core 1
- All keyboard, lever, octave button, and LED updates use I2C
- ESP32 I2C driver mutex cannot handle cross-core coordination

### Battery/Charging Boot Sequence

**CRITICAL: Charge controller behavior depends on boot sequence**
- **USB at boot** → Bypass/power mode → Device powered, NOT charging battery
- **Boot on battery → plug USB** → Charging mode → Device powered AND charging battery

**Firmware must track:** `usbConnectedAtBoot = isUsbPowered()` captured EARLY in setup(), BEFORE `loadBatteryState()`

**NEVER overwrite early USB detection** in fresh-boot block - this causes charging LEDs on bypass boot (bug fixed April 2026, DO NOT REINTRODUCE)

### Serial Output Philosophy

**Serial printing is expensive** (~5-10ms per call):
- Use compact format (single-character prefixes)
- Throttle high-frequency output (500ms minimum)  
- Auto-detect USB connection (don't block when disconnected)
- See `.github/skills/kb1-conventions/` for format reference

### LED Conventions

**ALWAYS follow:** Pink=Up/Forward/Increase, Blue=Down/Reverse/Decrease

Override default hardware LEDs when mode requires directional feedback (e.g., Pattern Selector).

### Firmware Release Process

**NEVER copy `.pio/build/firmware.bin` as release binary** - it's app partition only, causes boot loop at offset 0x0

**ALWAYS use:** `bash build_complete.sh` to produce merged binary (bootloader + partitions + firmware)

**Release checklist:**
1. Bump `FIRMWARE_VERSION` in `src/objects/Constants.h`
2. Update version in `build_complete.sh`  
3. Run `bash build_complete.sh` → outputs `KB1-firmware-vX.Y.Z.bin`
4. Copy to `kb1-flash/public/firmware/`
5. Update `kb1-flash/public/firmware/releases.json`
6. Bump `APP_VERSION` in `KB1-config/src/constants.ts`
7. Commit + push all three repos, tag firmware repo

## Safe Optimizations

### Always Allowed

- **Serial output:** Compact format, auto-detection, throttling
- **I2C speed:** 400kHz tested safe (currently optimal)
- **Task priorities:** Adjust within same core
- **Logic changes:** LED patterns, control algorithms, MIDI mapping

### Performance Priorities

1. **HIGH:** Bulk I2C read optimization (12× speedup available)
2. **MEDIUM:** LED bulk write optimization
3. **LOW:** I2C clock speed increase (diminishing returns)

## Code Review Checklist

Before submitting changes:

1. ✅ Checked that BLE stack stays on Core 1
2. ✅ Verified all I2C operations on same core
3. ✅ Confirmed touch sensor on Core 1
4. ✅ Used compact serial output format
5. ✅ Followed LED color conventions
6. ✅ Tested USB boot sequence behavior
7. ✅ Updated release documentation if protocol changed

## When In Doubt

- Check `.github/skills/kb1-hardware/` for schematic and hardware details
- Check `.github/skills/kb1-optimization/` for performance patterns  
- Check `.github/skills/kb1-conventions/` for design patterns and breaking changes
- Ask before violating documented constraints

## Last Updated

April 22, 2026 - Migrated from memory system to .github/skills/
