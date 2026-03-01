# Contributing to KB1

> **Purpose:** This guide documents project conventions, architecture decisions, and development workflows. Useful for contributors, maintainers, and AI assistants working on the codebase.

---

## Project Overview

**KB1 MIDI Controller** - A handcrafted ESP32-S3 based MIDI controller with:
- Dual workspace: `firmware/` (C++ embedded) and `KB1-config/` (Vue.js web app)
- BLE-based wireless configuration and MIDI output
- One-person artisanal operation - avoid mass-production terminology

### ⚠️ CRITICAL: ESP32-S3 Compatibility
**The combined .bin files MUST remain ESP32-S3 compatible!**
- Always use `--chip esp32s3` in esptool merge commands
- Target: `seeed_xiao_esp32s3` board
- Flash settings: DIO mode, 80MHz, 8MB flash size
- This is verified in `build_complete.sh` - don't modify without testing

---

## Terminology Preferences

### ❌ AVOID
- **"Factory"** - use "complete" or "handcrafted" instead
- "Production line" or mass-manufacturing terms
- Overly corporate language

### ✅ USE
- **"Complete firmware image"** (not "factory image")
- **"Handcrafted"**, **"curated"**, **"bespoke"**
- Language that reflects a one-person, artisanal operation
- `build_complete.sh` (not `build_factory.sh`)

---

## Architecture & Critical Info

### Sleep System (Fixed in v1.1.4)
- **Light Sleep:** User-configurable timeout (3-10 min, default 5 min)
- **Deep Sleep:** Auto-calculated as light sleep + 90s (hardcoded warning period)
- **BLE Timeout:** Keepalive prevents sleep (5-20 min, default 10 min)
- **Bug History:** Pre-v1.1.4 firmware ignored `lightSleepTimeout` setting (used `deepSleepTimeout` instead)

### Serial Debugging
- Controlled by `SERIAL_PRINT_ENABLED` macro in Constants.h
- Startup delay optimized to 600ms (v1.1.4+) - was 5000ms before
- Timeout-based wait: exits immediately when serial monitor connects

### Version Update Checklist
When bumping firmware version (e.g., 1.1.3 → 1.1.4):
1. `firmware/src/objects/Constants.h` - FIRMWARE_VERSION defines
2. `firmware/build_complete.sh` - FIRMWARE_VERSION variable
3. `firmware/README.md` - Latest Release section
4. `KB1-config/package.json` - version field
5. Create `firmware/RELEASE_NOTES_v1.x.x.md`
6. Run `./build_complete.sh` to generate complete binary

---

## File Organization

### Firmware Structure
```
firmware/
├── src/
│   ├── main.cpp                 # Main loop, setup, sleep logic
│   ├── bt/                      # BLE controllers and callbacks
│   ├── controls/                # Input handlers (keyboard, levers, touch)
│   ├── led/                     # LED controller
│   ├── music/                   # Scale manager
│   └── objects/
│       ├── Constants.h          # VERSION defines, UUIDs, config
│       ├── Globals.h            # Shared types and externs
│       └── Settings.h           # Setting structures
├── build_complete.sh            # Complete firmware builder
├── RELEASE_NOTES_v*.md          # Version-specific release notes
└── README.md                    # Project documentation
```

### Web App Structure
```
KB1-config/
├── src/
│   ├── ble/
│   │   ├── bleClient.ts         # BLE connection management
│   │   └── kb1Protocol.ts       # Settings validation, defaults
│   ├── components/
│   │   └── SystemSettings.vue   # Power management UI
│   └── ...
└── package.json                 # Web app version
```

---

## Code Conventions

### Firmware (C++)
- Use `SERIAL_PRINT()` macro for debug output (auto-disabled in production)
- Sleep timeouts stored in **seconds** (user-facing) but converted to **milliseconds** internally
- Variable naming: `lightSleepIdleMs`, `deepSleepIdleMs` (not "timeout" for internal vars)
- BLE characteristic callbacks in separate files (`*Callbacks.cpp`)

### Web App (Vue/TypeScript)
- Settings use **seconds** as base unit
- Add "s" suffix labels to timing inputs for consistency
- Validation in `kb1Protocol.ts` - don't duplicate in components
- Auto-calculated values (like deep sleep) should use `watch()` in Vue
- Hint text format: Brief explanation of behavior, not just field labels

---

## Common Tasks

### Building Complete Firmware
```bash
cd firmware
./build_complete.sh
# Output: KB1-firmware-v1.x.x.bin
```

### Flashing Device
```bash
# Method 1: PlatformIO (dev builds)
platformio run --target upload --environment seeed_xiao_esp32s3

# Method 2: ESPConnect web tool (complete images)
# https://thelastoutpostworkshop.github.io/ESPConnect/
# Flash at 0x0 with "Erase entire flash" checked
```

### Clean Build
```bash
# Light clean (rebuild sources)
platformio run --target clean --environment seeed_xiao_esp32s3

# Full clean (re-download all dependencies)
platformio run --target fullclean --environment seeed_xiao_esp32s3
```

---

## Known Issues & Gotchas

### BLE Keepalive
- Web app must ping to keep device awake
- BLE radio **disabled** before sleep - cannot wake from deep sleep via BLE
- Only **touch sensor** wakes from deep sleep

### Sleep Warning Period
- 90-second pulsing LED period is **hardcoded** in `SleepControl.h`
- Not user-configurable by design
- Deep sleep always = light sleep + 90s

### Memory Constraints
- Flash: ~76% full (1MB / 1.3MB) as of v1.1.4
- RAM: ~16% usage (51KB / 327KB)
- Keep BLE characteristics minimal

---

## Release History Context

### v1.1.4 (Mar 1, 2026)
- **Critical Fix:** Sleep timeout bug (ignored lightSleepTimeout since v1.0)
- **Optimization:** Startup reduced from 5s to 600ms
- **UX:** Removed deep sleep user control (auto-calculated)
- **Terminology:** Changed "BT CONNECTION" → "BLE TIMEOUT"
- **Naming:** Removed "factory" terminology, use "complete" instead

### v1.1.3 (Feb 28, 2026)
- Fixed chord mode scale quantization bug
- Chords now chromatic regardless of scale selection

### v1.1.2
- Preset system improvements

### v1.1.1
- Initial production release

---

## Development Notes

### Testing Checklist
- [ ] Sleep timing verification (light sleep triggers at configured time)
- [ ] Deep sleep triggers 90s after light sleep
- [ ] BLE keepalive prevents sleep when app connected
- [ ] Touch sensor wakes from deep sleep
- [ ] Settings persist across power cycles
- [ ] Serial debugging works without delays

### Future Considerations
- Flash space getting tight (~76% full) - consider optimization if adding features
- 90s warning period is hardcoded - would require `SleepControl.h` changes
- BLE keepalive grace period is 10 min default (changeable in Constants.h)

---

## Project Philosophy

This is a **one-person, handcrafted operation**. Language should reflect:
- Artisanal quality over mass production
- Thoughtful design over corporate speak
- Personal touch and care in development

---

**Last Updated:** March 1, 2026 (v1.1.4 release)
