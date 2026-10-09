# KB1 Firmware

Embedded firmware for the handcrafted PocketMidi KB1 MIDI controller, built with PlatformIO and Arduino for the Seeed XIAO ESP32-S3.

## Capabilities

- Scale, Chord (Block/Strum), and Arp keyboard modes.
- Configurable lever, press, and capacitive touch controls.
- Serial MIDI output, with BLE configuration and real-time CC control from the companion app.
- Eight device preset slots and persistent settings.
- Idle sleep, touch wake, and time-based battery tracking.

## User Documentation

Use these maintained guides instead of a duplicate settings reference here:

- [KB1 Studio User Guide](https://pocketmidi.github.io/KB1-studio/): Hardware setup, charging, LED signals, and Tracker MIDI settings.
- [Configurator User Guide](https://github.com/PocketMidi/KB1-config/blob/main/docs/USER_GUIDE.md): Current app labels and control behavior.
- [KB1 Configurator](https://pocketmidi.github.io/KB1-config/): Wireless settings and performance sliders.

Bluetooth must be enabled using the inward-lever hold gesture before pairing with the Configurator. Charging and calibration instructions are maintained in the guides above, not duplicated here.

## Development Build

Install [PlatformIO](https://platformio.org/) and run from this directory:

```bash
pio run --environment seeed_xiao_esp32s3
pio run --target upload --environment seeed_xiao_esp32s3
pio device monitor
```

PlatformIO uploads the appropriate components to their correct offsets. Edit [platformio.ini](platformio.ini) for build or port settings.

## Complete Release Images

**Never distribute the app-only binary as a complete image.** Flashing the app partition at offset `0x0` will not boot.

```bash
bash build_complete.sh
```

[build_complete.sh](build_complete.sh) builds and merges:

| Component | Offset |
|---|---|
| Bootloader | `0x0` |
| Partition table | `0x8000` |
| Application | `0x10000` |

The script targets ESP32-S3 with DIO, 80 MHz, and 8 MB flash. Output is `../kb1-studio/dist/firmware/KB1-firmware-vX.Y.Z.bin`.

For browser flashing, use [KB1 Studio's Flash Tools](https://pocketmidi.github.io/KB1-studio/) on a desktop browser with Web Serial (Chrome, Edge, or Opera). Normal updates back up and restore NVS, preserving calibration, settings, and presets. **Clear device data on update** intentionally skips preservation.

For release coordination and hardware validation, see [CONTRIBUTING.md](../CONTRIBUTING.md).

## Source Layout

- [src/main.cpp](src/main.cpp): Setup, tasks, battery tracking, and main loop.
- [src/bt/](src/bt/): BLE services and callbacks.
- [src/controls/](src/controls/): Keyboard, lever, press, touch, octave, and sleep behavior.
- [src/led/](src/led/): LED feedback.
- [src/music/](src/music/): Scales, chords, and patterns.
- [src/objects/](src/objects/): Constants, shared state, and settings structures.

BLE, touch, and all I2C work must remain on Core 1. Do not split the two GPIO expanders or input/LED I2C tasks across cores.

## Historical Notes

These are development snapshots, not current setup instructions or verified current performance claims:

- [I2C efficiency analysis](I2C_EFFICIENCY_ANALYSIS.md)
- [Bulk-read implementation proposal](I2C_BULK_READ_IMPLEMENTATION_PLAN.md)
- [Additional optimization proposals](ADDITIONAL_PERFORMANCE_OPPORTUNITIES.md)
- [Archived Arduino firmware](initial_arduino_code/README.md)

## License

See the repository [LICENSE](../LICENSE).
