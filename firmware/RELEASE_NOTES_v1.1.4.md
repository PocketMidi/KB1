# KB1 Firmware v1.1.4 Release Notes

## Overview

Version 1.1.4 is a critical bugfix and optimization release addressing a sleep timing issue present since v1.0, improving startup performance, and enhancing power management UX.

## Firmware Changes

**Version Identification:**
- Firmware version string: `1.1.4`
- Build date: Mar 1 2026
- Full backward compatibility with v1.1.3, v1.1.2, and v1.1.1

### Critical Bug Fix: Light Sleep Timeout Ignored

**Issue:** Since v1.0, the firmware incorrectly used `deepSleepTimeout` instead of `lightSleepTimeout` when loading settings, causing the light sleep timeout setting to be completely ignored. Only the deep sleep timeout value was ever used.

**Impact:** Users who configured different light/deep sleep timeouts would not see the expected behavior. The device would always use the deep sleep timeout value for entering sleep mode.

**Fix:** 
- Corrected `loadSettings()` in [main.cpp](src/main.cpp#L333) to use `systemSettings.lightSleepTimeout`
- Renamed internal variable from `deepSleepIdleMs` to `lightSleepIdleMs` for clarity
- Now properly respects the user-configured light sleep timeout (3-10 min range)

**Affected Code:** [main.cpp](src/main.cpp)

### Performance Optimization: Faster Startup

**Issue:** Firmware had a fixed 5-second startup delay (2s initial + 3s serial wait) that blocked device availability.

**Fix:** Optimized serial debugging wait logic:
- Reduced initial delay from 2000ms to 100ms
- Changed serial wait from fixed 3s to timeout-based 500ms
- **Result:** Startup now 600ms worst case (was 5000ms)
- Serial debugging fully preserved - exits immediately when serial monitor connects

**Benefits:**
- 88% faster boot time
- Better user experience when powering on or waking from deep sleep
- No loss of debug capability for development

**Affected Code:** [main.cpp](src/main.cpp) `setup()`

### Power Management Improvements

**Timing Defaults Updated:**
- Light sleep timeout: 240s (4 min) → **300s (5 min)**
- Deep sleep timeout: Auto-calculated as light sleep + 90s (no change)
- BLE timeout: 600s (10 min) - unchanged

**Ranges:**
- Light sleep: 3-10 minutes (180-600s)
- BLE timeout: 5-20 minutes (300-1200s)
- Deep sleep: Auto-calculated, not user-configurable

**Rationale:** More conservative defaults reduce unintended sleep during active sessions while maintaining power efficiency.

## Web App Changes (v1.1.4)

### System Settings UX Enhancements

**Nomenclature Improvements:**
- Changed "BT CONNECTION" to "BLE TIMEOUT" for clarity
- Added explanatory hint: "App keepalive pings reset the timeout - device stays awake while connected"

**UI Consistency:**
- Added "s" suffix unit labels to all timing inputs (matching lever settings pattern)
- Improved visual consistency across settings panels

**Simplified Sleep Configuration:**
- Removed user-facing deep sleep control (was confusing)
- Deep sleep now auto-calculated as light sleep + 90s
- Clear hint text explains the 90-second warning behavior
- Validation ensures proper auto-calculation on load

**Updated Constraints:**
- Removed old BLE/deep sleep coupling constraint
- Simplified validation logic
- Better error messages for invalid ranges

**Affected Files:**
- [SystemSettings.vue](../KB1-config/src/components/SystemSettings.vue)
- [kb1Protocol.ts](../KB1-config/src/ble/kb1Protocol.ts)

## Technical Details

### Sleep Behavior Flow

The corrected sleep behavior now works as follows:

1. **Idle Detection:** Device monitors for user interaction
2. **Light Sleep Entry:** After configured timeout (default 5 min), LEDs begin pulsing
3. **Warning Period:** Fixed 90-second warning with pulsing LEDs
4. **Deep Sleep Entry:** After warning period completes (light sleep + 90s)
5. **Wake:** Touch sensor wakes from deep sleep; any interaction prevents sleep entry

### BLE Keepalive Protocol

- Web app sends periodic keepalive pings when connected
- Each ping resets both light sleep and BLE timeout timers
- BLE timeout configurable 5-20 min (default 10 min)
- Device stays awake during active configuration sessions
- BLE radio disabled before entering sleep modes

### Variable Naming Cleanup

Internal variable renamed for code clarity:
- `deepSleepIdleMs` → `lightSleepIdleMs`
- Better reflects actual usage in sleep logic
- Reduces future maintenance confusion

## Downloads

**Complete Firmware Image:**
- File: `KB1-firmware-v1.1.4.bin` (1.0MB)
- Flash address: `0x0`
- MD5: `34216a3ed60f8984fed75682c8d1ad15`

## Installation

### Method 1: ESPConnect (Recommended)

**Browser-based flashing tool** — No software installation required.

1. Connect KB1 via USB-C
2. Open [ESPConnect](https://thelastoutpostworkshop.github.io/ESPConnect/)
3. Click Flash Tools → Download Flash Backup (save it!)
4. Load `KB1-firmware-v1.1.4.bin`
5. Check "Erase entire flash" and click "Flash Firmware"
6. Wait for completion (~30 seconds)

### Method 2: PlatformIO (Developers)

```bash
# Clone repository
git clone https://github.com/yourusername/KB1-firmware.git
cd KB1-firmware

# Checkout v1.1.4
git checkout v1.1.4

# Clean build (recommended)
platformio run --target clean --environment seeed_xiao_esp32s3

# Build and upload
platformio run --target upload --environment seeed_xiao_esp32s3

# Monitor serial output
platformio device monitor --baud 115200
```

### Method 3: Build Complete Image

For creating a complete firmware image with bootloader + partitions + app:

```bash
./build_complete.sh
```

This creates `KB1-firmware-v1.1.4.bin` ready to flash at offset `0x0`.

## Upgrade Notes

### From v1.1.3

- **Recommended:** This version fixes a critical bug present since day one
- No breaking changes - all settings preserved
- Light sleep timeout will now work as configured
- Startup will be noticeably faster

### From v1.1.2 or earlier

- Same as v1.1.3 upgrade, plus bugfixes and optimizations
- Chord mode fix included (from v1.1.3)

### Web App Compatibility

- Web app automatically updated to v1.1.4
- Compatible with all v1.1.x firmware versions
- Deep sleep control removed from UI (auto-calculated)
- BLE timeout renamed from "BT CONNECTION"

## Testing Recommendations

After upgrading to v1.1.4:

1. **Verify Sleep Timing:**
   - Configure light sleep to 5 minutes
   - Wait without interaction
   - Confirm LEDs pulse at 5 minutes (not earlier/later)
   - Confirm deep sleep at 6.5 minutes (5 min + 90s)

2. **Test Startup Speed:**
   - Power cycle the device
   - Time from power-on to LED sequence completion
   - Should be under 1 second (vs ~5s in previous versions)

3. **Check BLE Keepalive:**
   - Connect web app
   - Leave connected beyond light sleep timeout
   - Device should remain awake while app connected

4. **Validate Settings Persistence:**
   - Configure custom light sleep timeout
   - Power cycle device
   - Verify setting persists and is respected

## Known Issues

None at this time.

## Migration Path

This is a drop-in replacement for v1.1.3. Simply flash the new firmware.

**Settings Migration:**
- All existing settings automatically preserved
- No manual reconfiguration needed
- Deep sleep values will auto-calculate on next save

## Credits

Developed and maintained by the PocketMidi team.

## License

GNU General Public License v3.0 - See [LICENSE](LICENSE) for details.

---

**Previous Releases:** [v1.1.3](RELEASE_NOTES_v1.1.3.md) | [v1.1.2](RELEASE_NOTES_v1.1.2.md) | [v1.1.1](RELEASE_NOTES_v1.1.1.md)
