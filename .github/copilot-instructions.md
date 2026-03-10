# KB1 MIDI Controller - AI Assistant Instructions

> Custom instructions for AI assistants working on the PocketMidi KB1 MIDI controller project.

## Project Overview

The KB1 is a **handcrafted ESP32-S3 based MIDI controller** with dual workspaces:
- **`firmware/`** - C++ embedded firmware (PlatformIO/Arduino)
- **`KB1-config/`** - Vue.js/TypeScript web configuration app (BLE Web API)

**Communication:** BLE MIDI + custom BLE configuration protocol  
**Target Hardware:** Seeed XIAO ESP32-S3  
**Philosophy:** One-person artisanal operation

---

## Critical Project Terminology

### ❌ NEVER USE:
- **"Factory"** → Use **"complete"** or **"handcrafted"** instead
- "Production line", "mass manufacturing", or corporate terminology
- `build_factory.sh` → It's `build_complete.sh`

### ✅ ALWAYS USE:
- **"Complete firmware image"** (not "factory image")
- **"Handcrafted"**, **"curated"**, **"bespoke"**
- Language reflecting a one-person, artisanal operation

---

## Firmware Development (C++/ESP32)

### Build System - CRITICAL

**⚠️ NEVER DISTRIBUTE APP-ONLY BINARIES!** Always build complete images:

```bash
./build_complete.sh  # Creates KB1-firmware-vX.X.X.bin
```

**Why:** App-only `.bin` files cause SHA-256 verification failures and brick devices.

**Complete image includes:**
- Bootloader (0x0, ~14KB)
- Partition table (0x8000, ~3KB)
- Application (0x10000, ~978KB)

### ESP32-S3 Specific Requirements

- **Chip target:** Always `--chip esp32s3` in esptool commands
- **Board:** `seeed_xiao_esp32s3`
- **Flash mode:** DIO, 80MHz, 8MB
- **Bootloader at 0x0** (not 0x1000 like original ESP32!)

### Version Update Checklist

When bumping version (e.g., 1.2.5 → 1.2.6):

1. `firmware/src/objects/Constants.h` - Update `FIRMWARE_VERSION` defines
2. `firmware/build_complete.sh` - Update `FIRMWARE_VERSION` variable
3. `firmware/README.md` - Update "Latest Release" section
4. `KB1-config/package.json` - Update `version` field
5. Create `firmware/RELEASE_NOTES_vX.X.X.md`
6. Run `./build_complete.sh` to generate complete binary

### Code Conventions

- **Debug output:** Use `SERIAL_PRINT()` macro (auto-disabled in production)
- **Sleep timing:** Store in **seconds** (user-facing), convert to **milliseconds** internally
- **Variable naming:** `lightSleepIdleMs`, `deepSleepIdleMs` (not "timeout" for internal vars)
- **BLE callbacks:** Separate files (`*Callbacks.cpp`)

### Sleep System Architecture (v1.1.4+)

- **Light Sleep:** User-configurable (3-10 min, default 5 min)
- **Deep Sleep:** Auto-calculated as `lightSleep + 90s` (90s warning period is hardcoded)
- **BLE Timeout:** Keepalive prevents sleep (5-20 min, default 10 min)
- **Wake:** Only touch sensor wakes from deep sleep (BLE radio disabled during sleep)

**Known Bug History:** Pre-v1.1.4 firmware ignored `lightSleepTimeout` setting (used `deepSleepTimeout` instead).

### Serial Debugging

- Controlled by `SERIAL_PRINT_ENABLED` macro in `Constants.h`
- Startup delay: 600ms (v1.1.4+) - optimized from 5000ms
- Timeout-based wait: exits immediately when serial monitor connects

### File Structure

```
firmware/src/
├── main.cpp                 # Main loop, setup, sleep logic
├── bt/                      # BLE controllers and callbacks
├── controls/                # Input handlers (keyboard, levers, touch)
├── led/                     # LED controller and patterns
├── music/                   # Scale manager, chord generation
└── objects/
    ├── Constants.h          # VERSION, UUIDs, config macros
    ├── Globals.h            # Shared types and externs
    └── Settings.h           # Setting structures
```

---

## Web App Development (Vue.js/TypeScript)

### Theme System - Designer-Friendly

**CSS-ONLY architecture** for easy visual updates without code changes.

**Single source of truth:** `src/styles/themes/kb1.css`

```css
/* All visual assets referenced via CSS variables */
--lever-panel-bg: url('/src/assets/ui/lever-panel.svg');
--btn-primary-bg: linear-gradient(135deg, #3b82f6 0%, #2563eb 100%);
--kb1-spacing-md: 1rem;
```

**Designer workflow:**
1. Export assets from Figma → `src/assets/ui/`
2. Update paths in `kb1.css`
3. No component code changes needed!

### BLE Protocol

**Characteristic UUIDs:** See `FIRMWARE_PRESET_UUIDS.md` for complete list.

**Settings validation:** In `src/ble/kb1Protocol.ts`, not components  
**Data units:** Use **seconds** as base unit (matches firmware user-facing values)  
**Encoding/Decoding:** Typed functions in `kb1Protocol.ts`

### Code Conventions

- **Component pattern:** Use `v-model` for two-way binding
- **Validation:** Centralized in `kb1Protocol.ts`
- **Timing inputs:** Add "s" suffix labels for consistency
- **Auto-calculated values:** Use Vue `watch()` (e.g., deep sleep = light sleep + 90s)
- **Hint text:** Brief behavior explanation, not just field labels

### File Structure

```
KB1-config/src/
├── ble/
│   ├── bleClient.ts         # BLE connection management
│   └── kb1Protocol.ts       # Protocol encoding/validation
├── components/              # UI components (use theme variables)
├── composables/             # Shared state (useDeviceState, etc.)
├── pages/                   # Route pages
├── services/                # Business logic
├── state/                   # Global state management
└── styles/
    └── themes/
        └── kb1.css          # Theme variables (designer edits here)
```

### Data Flow

```
User Input → Component v-model → localSettings → handleApply()
  → useDeviceState.sendSettings() → kb1Protocol.encode()
  → bleClient.sendData() → Device Hardware
```

---

## BLE Architecture & Protocol

### Device Preset System (8 slots)

**Firmware UUIDs:**
- Save: `d3a7b321-0001-4000-8000-000000000009`
- Load: `d3a7b321-0001-4000-8000-00000000000a`
- List: `d3a7b321-0001-4000-8000-00000000000b`
- Delete: `d3a7b321-0001-4000-8000-00000000000c`

**Preset metadata format (per slot):**
```
Bytes 0-31:  Name (UTF-8, null-terminated, 32 bytes max)
Bytes 32-35: Timestamp (uint32_t, little-endian)
Byte 36:     isValid (1 = has data, 0 = empty)
Bytes 37-39: Padding (reserved)
```

### Preset Workflow

**Community Tab (Working Presets):**
- Unlimited browser localStorage presets
- Experiment and iterate
- Export/import JSON for sharing

**Archive Tab (Device Presets):**
- 8 slots in device flash memory
- Survive browser resets
- For finalized, go-to configurations

**Recommended flow:** Experiment in Working → Refine → Archive to device → Share with community

---

## Common Pitfalls & Known Issues

### Firmware

1. **Never flash app-only binaries** → Use `build_complete.sh`
2. **BLE radio disabled before sleep** → Cannot wake from deep sleep via BLE
3. **90-second warning period is hardcoded** → Not user-configurable by design
4. **Flash space:** ~76% full (1MB / 1.3MB) - consider optimization for new features
5. **Sleep bug (pre-v1.1.4):** Old firmware ignored `lightSleepTimeout` setting

### Web App

1. **Bluetooth must be enabled on hardware** → Cross-lever gesture (hold 3s)
2. **BLE keepalive required** → Web app must ping to prevent sleep
3. **Deep sleep timeout auto-calculated** → Don't offer user control
4. **Touch sensor is only wake source** from deep sleep

### Build Process

1. **Always test on real hardware** before release
2. **Backup flash before flashing** (ESPConnect provides this)
3. **Use ESPConnect web flasher** for user-friendly flashing experience
4. **Flash at offset 0x0** with "Erase entire flash" checked

---

## Testing Checklists

### Firmware Release
- [ ] Sleep timing verified (light sleep triggers at configured time)
- [ ] Deep sleep triggers 90s after light sleep
- [ ] BLE keepalive prevents sleep when app connected
- [ ] Touch sensor wakes from deep sleep
- [ ] Settings persist across power cycles
- [ ] Serial debugging works without delays
- [ ] Complete image built with `build_complete.sh`
- [ ] Tested on actual hardware (not just simulation)

### Web App Release
- [ ] BLE connection/disconnection works
- [ ] Settings load from device correctly
- [ ] Settings save to device and persist
- [ ] Auto-calculated values update (deep sleep, etc.)
- [ ] Community presets load correctly
- [ ] Device preset slots (8) work (save/load/delete)
- [ ] Evaluation mode works for demos
- [ ] Theme variables applied correctly
- [ ] Tested on Chrome/Edge (Web Bluetooth support required)

---

## Future Considerations

- **Flash space:** Approaching limits (~76% full) - optimize before adding features
- **Multiple themes:** Current architecture supports adding more theme files
- **90s warning period:** Would require `SleepControl.h` changes to make configurable
- **BLE keepalive grace period:** Default 10 min (changeable in `Constants.h`)

---

## Development Commands

### Firmware
```bash
# Build complete image (recommended)
./build_complete.sh

# Development build (PlatformIO)
pio run --environment seeed_xiao_esp32s3

# Upload to device
pio run --target upload --environment seeed_xiao_esp32s3

# Serial monitor
pio device monitor

# Clean build
pio run --target clean --environment seeed_xiao_esp32s3
pio run --target fullclean  # Re-download dependencies
```

### Web App
```bash
# Development server
npm run dev

# Build for production
npm run build

# Preview production build
npm run preview
```

---

## Resources & Documentation

- **Contributing Guide:** `/CONTRIBUTING.md`
- **Build Instructions:** `firmware/BUILD_INSTRUCTIONS.md`
- **Designer Workflow:** `KB1-config/DESIGNER_WORKFLOW.md`
- **Theme Implementation:** `KB1-config/THEME_IMPLEMENTATION.md`
- **Device Presets:** `KB1-config/DEVICE_PRESET_IMPLEMENTATION.md`
- **Firmware UUIDs:** `KB1-config/FIRMWARE_PRESET_UUIDS.md`
- **Release Notes:** `firmware/RELEASE_NOTES_v*.md`
- **Live Demo:** https://pocketmidi.github.io/KB1-config/

---

**Remember:** This is a handcrafted, artisanal project. Prioritize user experience, clear documentation, and maintainable code over rapid feature additions. Test thoroughly on real hardware before releases.
