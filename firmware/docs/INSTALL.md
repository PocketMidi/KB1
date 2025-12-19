# Installation & Upload Troubleshooting

If you encounter an error during build/upload like:

```
ModuleNotFoundError: No module named 'intelhex'
```

this means `esptool.py` (used by PlatformIO) needs the `intelhex` Python package.

On macOS you can fix this by installing `intelhex` both for your user Python and into PlatformIO's virtualenv (penv):

```bash
# install for your user Python
python3 -m pip install --user intelhex

# install into PlatformIO's penv (preferred if PlatformIO CLI fails to find the module)
~/.platformio/penv/bin/python -m pip install intelhex

# run PlatformIO from its penv if `platformio` isn't on your PATH
~/.platformio/penv/bin/platformio run --target upload --environment seeed_xiao_esp32s3
```

After these steps the upload/build should succeed. If problems persist, try reinstalling `tool-esptoolpy` via PlatformIO or consult PlatformIO documentation.