# KB1 Firmware v1.6.0 Release Notes
**Release Date:** April 5, 2026

## 🎯 What's New

### BLE-Aware Battery Tracking
- **Dynamic drain rates** based on Bluetooth connection state
- **85mA drain** when BLE connected (active MIDI streaming)
- **50mA drain** when BLE disconnected (idle/standby mode)
- **More accurate runtime estimates** reflecting actual usage patterns
- **Time-based calculation** across all power states (active, light sleep, deep sleep)

**Impact:**
- Battery percentage updates reflect real-world usage more accurately
- Longer estimated runtime when not connected to BLE devices
- Proper accounting for BLE radio power consumption

### Keep-Alive Auto-Sync
- **60-second interval** automatic connection maintenance
- **Battery percentage sync** sent to web app every minute
- **Prevents BLE timeout** during extended sessions
- **Zero user interaction required** - works silently in background

**Technical Details:**
- Lightweight BLE write operation every 60s
- Keeps connection alive during long idle periods
- Web app receives live battery updates without manual refresh

### Battery Persistence Fix (Critical)
- **Manual battery % now persists** across reboots and firmware uploads
- **Calculated time storage** - converts percentage to equivalent consumed time
- **Prevents corruption** - main loop no longer overwrites manual calibration
- **LED breathing fix** - exits charging mode when manual % set via dev mode

**What Was Broken:**
- Setting battery % via dev mode would revert to 100% or "Uncalibrated" after reboot
- Time trackers reset to 0, causing battery calculation to return wrong value
- LED breathing continued after manual set (should exit charging mode)

**How It's Fixed:**
- Manual % converted to equivalent time: `(100 - %) × 420mAh ÷ 50mA × 3600s`
- Example: 74% = 26% consumed = 109.2mAh = 7,848,000ms stored in NVS
- Battery calculation returns same % from stored time on next boot
- Charging mode exits immediately when manual % set

**Testing Results:**
- ✅ 87% → Persisted through reboot
- ✅ 74% → Persisted through reboot
- ✅ 88% → Persisted through firmware upload and reboot
- ✅ NVS values loading correctly (batBleOffMs: 3628s)

### Dev Mode Battery Control
- **Secret menu unlock** - tap battery icon 5 times within 2 seconds
- **Manual % control** - drag slider to set any battery percentage (0-100%)
- **Mobile-optimized input** - ValueControl component with touch-friendly drag selector
- **Instant device sync** - sends BLE command 0x02 to firmware
- **Testing & calibration** - useful for development and edge-case testing

**Usage:**
1. Open Battery Modal
2. Tap battery icon 5 times quickly (< 2s)
3. "DEV MODE" section appears with slider
4. Drag to desired percentage
5. Automatically syncs to device via BLE

### Optimized Debug Output
- **Minimal serial footprint** for production firmware
- **Compact format** - essential troubleshooting info only
- **Battery save message** - "Bat saved: X%" (replaces verbose logs)
- **Performance win** - each serial print blocks ~5-10ms
- **Production-ready** - efficient and clean console output

## 🌐 Web App Changes (v1.6)

### System Settings Always Accessible
- **Removed connection requirement** for System Settings
- **Settings persist** regardless of BLE state
- **User preferences available** immediately on app load
- **Consistent with other system-level controls** (presets still require connection for device operations)

**What Changed:**
- System Settings accordion moved outside `disconnected-state` wrapper
- Subtitle updated to "Configurator Settings" (was "Power & Timeout Settings")
- Version badge shows "SYSTEM v1.6"

### ValueControl Component Integration
- **Touch-friendly drag control** for mobile-first design
- **Replaces number input** in dev mode battery control
- **Visual feedback** with gradient progress bar (purple/pink)
- **Haptic feedback** on supported devices
- **Smooth value changes** with touch/mouse drag

### UI Cleanup
- **Firmware update removed** from mobile web app (242 lines of CSS removed)
- **Streamlined SystemSettings.vue** - reduced from 830+ lines to ~280 lines
- **Better focus** on core configuration features
- **Improved performance** with less UI complexity

**Why Removed:**
- Web Serial API limited to Chrome (not Safari/Firefox)
- iOS has zero Web Serial support
- Android USB OTG unreliable across devices
- User frustration from browser/OS compatibility issues

### Version Synchronization
- **App version: 1.6** (constants.ts, package.json)
- **Firmware version: 1.6.0** (displays in System Settings)
- **Consistent branding** across firmware and web app

## 🔧 Technical Changes

### Firmware Architecture
- **CharacteristicCallbacks.cpp** (Lines 230-310): Command 0x02 improved
  - Calculates consumed mAh from percentage
  - Stores as equivalent time in `activeTimeBleDisconnectedMs`
  - Exits charging mode to stop LED breathing
  - Saves to NVS with correct persistence keys
- **main.cpp**: Optimized debug output, minimal serial prints
- **Battery calculation**: Uses time tracking for percentage (not voltage)
- **Keep-alive**: 60s BLE write in main loop

### Web App Architecture
- **BatteryModal.vue**: 5-click dev mode unlock, ValueControl integration
- **SystemSettings.vue**: Removed firmware update section and dependencies
- **MobileScales.vue**: System Settings accessibility change
- **constants.ts**: APP_VERSION = '1.6'
- **package.json**: version: "1.6.0"

### Dependency Preservation
- **esptool-js v0.4.2** retained in package.json (not used in mobile app)
- **useFirmwareUpdate.ts** preserved intact (not deleted)
- Available for porting to future desktop/native applications

## 📦 Binary Details

**Filename:** `KB1-firmware-v1.6.0.bin`  
**Size:** 1.0M (1,025,568 bytes)  
**Format:** Combined binary (bootloader + partitions + firmware)  
**Flash Layout:**
- 0x0: Bootloader
- 0x8000: Partition table
- 0x10000: Firmware application

**Flash Command:**
```bash
esptool.py --chip esp32s3 --port /dev/cu.usbmodem* write_flash \
  0x0 KB1-firmware-v1.6.0.bin
```

## 🐛 Bug Fixes

1. **Battery % persistence** - Manual calibration now survives reboots
2. **LED breathing on USB-only** - Exits charging mode when manual % set
3. **Battery calculation corruption** - Time-based approach prevents 100% revert
4. **NVS storage** - Correct keys used for battery state persistence

## 🔄 Migration Notes

### From v1.5.x
- **No breaking changes** - all settings and calibrations preserved
- **Battery calibration survives** firmware upload (if using dev mode)
- **Web app version** auto-detects firmware version on connection
- **Backward compatible** - works with older web app versions

### First-Time Setup
1. Flash using the **[KB1 Web Flash Tool](https://pocketmidi.github.io/KB1-flash/)** — no software needed (Chrome required), or flash manually via `esptool.py`
2. Open KB1 Configurator web app (auto-updates to v1.6)
3. Connect via Bluetooth
4. Battery tracking active immediately (calibrated or estimating)
5. Optional: Use dev mode to set initial battery % for testing

### KB1 Web Flash Tool

New in v1.6: the **[KB1 Web Flash Tool](https://pocketmidi.github.io/KB1-flash/)** makes firmware updates easy — select a version, click Flash, done. No drivers, no CLI tools required. The tool automatically backs up and restores your NVS settings (presets, calibration, configuration) so nothing is lost during an update.

## 📝 Known Limitations

- **Battery calibration** still requires 5-hour USB charge for most accurate tracking
- **Speaker compensation** must be manually reported (analog amp usage not auto-detected)
- **BLE connection** required for preset load/save/archive operations
- **Dev mode** visibility limited to battery modal (no global dev settings)

## 🔮 Future Roadmap

- Desktop firmware update tool with NVS preservation (v2.0+)
- Additional dev mode features (lever/touch calibration overrides)
- Real-time BLE connection quality metrics
- Enhanced battery analytics and history tracking

---

**Upgrade Path:** Flash KB1-firmware-v1.6.0.bin → Connect to web app → Enjoy improved battery tracking!
