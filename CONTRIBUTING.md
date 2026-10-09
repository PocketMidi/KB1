# Contributing to KB1

KB1 is a one-person, handcrafted ESP32-S3 MIDI controller project. Prioritize clear user experience, maintainable code, and verification on real hardware.

## Workspace

- [firmware/](firmware/): PlatformIO/Arduino C++ firmware in this repository.
- [KB1-config](https://github.com/PocketMidi/KB1-config): Vue/TypeScript Configurator, checked out alongside firmware.
- [KB1-studio](https://github.com/PocketMidi/KB1-studio): TypeScript user guides, Flash Tools, and Instrument Builder.

See each repository's README for its development commands. The Configurator and Studio have separate Git histories; check status in each repository before changing or committing files.

## Documentation Ownership

- The [root README](README.md) is the project overview and documentation index.
- The [firmware README](firmware/README.md) owns firmware build and complete-image instructions.
- The [Configurator User Guide](https://github.com/PocketMidi/KB1-config/blob/main/docs/USER_GUIDE.md) owns the written app-settings reference, aligned with the in-app USER GUIDE and **i** dialogs.
- [Studio's User Guide](https://pocketmidi.github.io/KB1-studio/) owns hardware setup, charging, LED signals, and Tracker configuration. Its tool guides cover flashing and instrument building.
- The [hardware documentation](hardware/README.md) owns design and assembly files.

Link to the owning guide instead of copying detailed parameter lists into multiple READMEs. Use the actual UI labels and displayed units, not internal enum names or raw firmware values. If guides disagree, verify against current app components and firmware before editing.

Preserve useful historical investigations with a prominent historical notice. Do not present old implementation proposals, source line numbers, or estimated gains as verified current behavior.

## Terminology

Use **complete firmware image**, **starter presets**, and language reflecting a handcrafted project. Avoid mass-manufacturing terminology. Distinguish starter presets from **Load Defaults**, which resets the editable configuration without replacing saved slots.

## Firmware Constraints

- Target **Seeed XIAO ESP32-S3**; esptool commands must use `--chip esp32s3`.
- Complete images contain bootloader at `0x0`, partition table at `0x8000`, and application at `0x10000`. Never publish an app-only image as a complete image.
- BLE, touch sensing, and **all I2C work must remain on Core 1**. Input reads and LED writes share the two MCP23017 expanders; do not split them across cores.
- Use `SERIAL_PRINT()` macros for debug output and throttle high-frequency logging.
- Directional feedback uses **pink for up/forward/increase**, **blue for down/reverse/decrease**.
- Capture USB-at-boot detection before loading persisted battery state; do not overwrite it with a fresh-boot default.
- Sleep settings are user-facing seconds; convert to milliseconds for runtime scheduling. Deep sleep follows the idle warning by 90 seconds, rather than exposing a separate user setting.

## Configurator Conventions

- Keep protocol encoding and validation in the BLE layer, not UI components.
- Use typed data, existing components, and `v-model` for settings.
- Keep visual styling in the theme system. See the [typography reference](https://github.com/PocketMidi/KB1-config/blob/main/TYPOGRAPHY_REFERENCE.md).
- Display the units and ranges appropriate to each selected parameter; do not equate every UI value with a raw MIDI byte.
- Verify sending, refresh-from-device, preset Apply, and NVS sync independently. Loading a browser preset is not the same as sending it to hardware.

## Release Coordination

1. Update firmware version defines in [Constants.h](firmware/src/objects/Constants.h) and the version in [build_complete.sh](firmware/build_complete.sh).
2. Run `bash build_complete.sh` from the firmware directory.
3. Add the complete image to Studio's published firmware assets and update `public/firmware/releases.json` with the exact byte size. The builder outputs to Studio's `dist/firmware/`; preserve release assets in `public/firmware/` for subsequent Vite builds.
4. Update the Configurator's `APP_VERSION` in `src/constants.ts` when coordinating a compatible release. Do not use the package version as a proxy for the displayed app version.
5. Update directly affected guides and the release manifest's notes.
6. Validate on actual hardware before publication. Coordinate commits across repositories and tag the firmware release.

## Validation

Use the smallest relevant build or type-check for code changes. Documentation-only changes need label, link, and consistency checks, not a firmware rebuild.

Before releasing firmware or changing protocol behavior, verify:

- [ ] Settings load, send, persist across restart, and refresh correctly.
- [ ] Preset Apply and device-slot NVS sync retain their distinct behavior.
- [ ] Light sleep uses the configured timeout; deep sleep follows after 90 seconds.
- [ ] BLE keepalive prevents sleep while configuring.
- [ ] Touch wakes from deep sleep.
- [ ] Battery boot followed by USB connection starts tracked charging.
- [ ] USB-at-boot bypass does not falsely signal tracked charging.
- [ ] Partial tracked charge sessions preserve calibration progress.
- [ ] Normal Studio updates preserve NVS; clear-data updates erase it only when explicitly selected.
- [ ] The released binary is a complete image, tested on ESP32-S3 hardware.
