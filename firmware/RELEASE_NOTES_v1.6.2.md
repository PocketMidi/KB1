# KB1 Firmware v1.6.2 Release Notes
**Release Date:** April 11, 2026

## 🐛 Bug Fixes

### Battery Set % Tool (Secret Menu)
- **Fixed:** Setting battery % via the dev tool (5-tap battery icon unlock) was silently rejected by the firmware
- **Root cause:** The battery control characteristic rejected any command longer than 1 byte, so the 2-byte `[0x02, percentage]` set-% command was discarded before being read
- **Fix:** Length check now accepts both 1-byte (reset) and 2-byte (set %) commands; the set command correctly recalculates all internal time trackers so the new % persists accurately across reboots

### Battery Auto-Sync via Keep-Alive (Battery Display)
- **Fixed:** Battery level was not updating automatically — only a manual "Sync Now" in the battery modal would refresh the display
- **Root cause:** The keep-alive BLE characteristic was missing `PROPERTY_NOTIFY`, so the web app could not subscribe to status notifications (`NotSupportedError: GATT Error: Not supported`)
- **Fix:** Keep-alive characteristic now supports notifications; firmware sends a 10-byte status packet (battery %, USB state) on every ping, so the web app receives automatic battery updates every 60 seconds

## ⬆️ Upgrade Notes
No settings or calibration data will be lost — full NVS preservation applies.
