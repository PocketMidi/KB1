# KB1 Firmware v1.2.6 Release Notes

**Release Date:** March 11, 2026

Version 1.2.6 focuses on reliability improvements with critical bug fixes and safeguards for pattern control management and preset change tracking.

**Version Identification:**
- Firmware version string: `1.2.6`
- Version characteristic reports correctly via BLE

---

## What's New

### Safeguards & Reliability

#### Pattern Control Reset Safeguards
- **Web App Pattern Control Management**: Shape Selector (CC 201) and Swing (CC 202) reset logic now happens entirely in the web app before upload
- **Automatic Reset on SHAPE Close**: When the SHAPE panel closes, any control assigned to Shape Selector or Swing automatically resets to firmware defaults
- **UI/Firmware Sync**: Ensures web app UI always reflects actual control assignments after exiting pattern mode
- **Simplified Firmware**: Firmware now only manages arpeggiator playback state, preventing control conflicts

#### Preset Auto-Open Fix
- **SHAPE Panel Auto-Open**: Presets with active strum patterns now correctly auto-open the SHAPE panel on load
- **Consistent Behavior**: Panel auto-open works reliably whether site is freshly loaded or already running

---

## Bug Fixes

### Change Tracking System (Critical)
Fixed comprehensive Vue reactivity issues affecting preset modification detection:

#### Affected Components
- **Lever Settings**: STEPS adjustment, Unipolar/Bipolar toggle, profile selection, min/max values, duration ganging
- **Press Settings**: Profile selection (inc/pd/exp/log/lin), min/max values, reset values, duration ganging
- **Touch Settings**: Function mode selection, KB1 Expression parameter clamping, min/max values, threshold adjustment, category fallback
- **Scale Settings**: Natural/Compact keyboard mapping toggle
- **System Settings**: Auto deep sleep timeout sync

#### Impact
- **Save Button**: Now reliably enables after any setting adjustment
- **Preset Re-saving**: Users can now modify loaded presets and re-save them properly
- **Change Detection**: All 65+ direct mutations converted to object spread pattern for proper Vue reactivity

### Pattern Control Naming
- **Consistent Terminology**: CC 201 referred to as "Shape Selector", CC 202 as "Swing" throughout codebase for clarity

---

## Technical Details

### Firmware Changes
- Removed `PatternControlMemory` save/restore logic (~90 lines)
- Removed `previousStrumPattern` tracking
- Simplified `resetPatternControlsCallback` to only call `stopArpeggiator()` when exiting pattern mode
- Firmware no longer modifies control CC assignments

### Web App Changes
- Added watchers in `MobileScales.vue` and `MobileControls.vue` to detect SHAPE panel close
- Pattern control reset performs before settings upload
- Fixed Vue computed setter pattern across all settings components
- Added debug logging for change tracking (`console.log('✏️ Settings changed')`)

### Default Control Assignments (Post-Reset)
When SHAPE panel closes, controls reset to:
- Lever 1: CC 3 (default)
- Lever 1 Press: CC 24 (default)
- Lever 2: CC 128 (disabled)
- Lever 2 Press: CC 128 (disabled)
- Touch: CC 1 (default)

---

## Compatibility

- **Firmware**: v1.2.6
- **Web App**: Requires updated KB1-config web app (March 11, 2026 or later)
- **Hardware**: Seeed XIAO ESP32-S3
- **Bluetooth**: BLE 4.2+

---

## Upgrade Notes

### For Users
1. Flash firmware v1.2.6 to your KB1 device
2. Use the updated web app (clear browser cache if needed)
3. Existing presets remain compatible
4. Any presets with Shape Selector/Swing assigned to controls will auto-reset when SHAPE panel closes

### Breaking Changes
- None - all changes are backward compatible

---

## Known Issues

- None at this time

---

## Credits

Special thanks to the KB1 community for reporting the preset modification tracking issue and pattern control reset behavior.
