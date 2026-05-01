# KB1 Firmware v1.7.2 Release Notes

**Release Date:** May 1, 2026  
**Focus:** Battery system corrections and interactive power planning

---

## 🔋 Battery System Improvements

This release fixes critical battery calculation errors and adds an interactive power mode selector for runtime planning.

### Fixed Power Consumption Values (Critical Bug Fix)

**Issue:** Power consumption constants were backwards - disconnected mode (BLE off) incorrectly consumed MORE power than idle connected mode (BLE on).

**Corrected Values:**
- **Disconnected (BLE Off):** 50mA → **35mA** ✓ (Lowest - no radio active)
- **BLE Idle Connected:** 35mA → **50mA** ✓ (Radio on, connection maintained)
- **BLE Configuration:** 60mA (unchanged)
- **BLE Live Performance:** 95mA (unchanged)

**Impact:** 
- Disconnected mode now correctly shows **longest runtime** (~11h at 93%)
- BLE Idle shows **shorter runtime** than disconnected (~7h at 93%)
- Battery estimates now align with actual hardware behavior

---

## 🎯 Interactive Power Mode Selector (Web App)

### New Battery Planning Tool

The battery modal now includes an interactive mode selector that lets users **preview runtime** for different usage scenarios before committing to an activity.

**Features:**
- Four radio buttons representing each power mode
- Real-time runtime calculation based on current battery percentage
- Visual feedback with mode-specific colors:
  - Disconnected: Blue (#4d5f7e)
  - Idle: Tan (#7c694c)
  - Configuration: Purple (#8e5078)
  - Live: Red (#a1483d)
- Selection persisted to localStorage
- Defaults to "Configuration" mode (user is actively using config app)

**UX Improvements:**
- Removed "Last Synced" display (no longer relevant for planning tool)
- Info icon placed after "Estimated Runtime" label for UI stability
- Help modal reordered from lowest to highest power consumption
- Compact single-line layout for mode selector and runtime display

---

## 📝 Technical Changes

### Firmware
- `BATTERY_ACTIVE_DRAIN_MA`: 50mA → 35mA
- `BATTERY_BLE_IDLE_DRAIN_MA`: 35mA → 50mA
- Updated comments to reflect correct power hierarchy
- Battery runtime calculations now use corrected baseline

### Web App
- Added interactive power mode selector component
- Runtime calculation dynamically updates based on selected mode
- Removed obsolete "Last Synced" timestamp
- Updated help text to emphasize planning functionality
- Default mode matches user's current state (Configuration)

---

## 🐛 Bug Fixes

- **Battery Calculation:** Fixed inverted power consumption values for disconnected vs. idle modes
- **Runtime Accuracy:** All battery runtime estimates now use correct drain rates
- **UI Stability:** Info icon placement no longer shifts when mode name changes

---

## 📦 Installation

**Firmware Update:**
1. Use KB1 Flash Tool or esptool to flash `KB1-firmware-v1.7.2.bin`
2. NVS partition is preserved (battery calibration retained)

**Web App Update:**
- Updated automatically at https://kb1.pocketmidi.com/

---

## 🔧 Developer Notes

### Power Consumption Hierarchy (Corrected)
```
Disconnected:     35mA  (BLE off, longest runtime)
BLE Idle:         50mA  (BLE on, minimal activity)
BLE Config:       60mA  (BLE on, data transfer)
BLE Live:         95mA  (BLE on, max responsiveness)
```

### Battery Modal Component Changes
- Power mode selector integrated into `battery-details` container
- Mode selection uses localStorage key: `kb1-battery-power-mode`
- Runtime calculation: `(batteryPercent * 420mAh) / modeDrainMa = hours`
- Outline styling: `outline` + `outline-offset` for ring effect

---

## 📊 Testing Notes

- Battery runtime estimates validated against firmware constants
- Power mode selector persists user selection across sessions
- Modal UX tested for stability and clarity
- All calculations aligned between firmware and web app

---

**Previous Release:** v1.7.1 (April 20, 2026) - Hardware-based musical expression  
**Next Planned:** v1.8.0 - TBD
