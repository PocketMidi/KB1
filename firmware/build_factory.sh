#!/bin/bash
# Factory Firmware Build Script for KB1 ESP32-S3
# This creates a complete factory image including bootloader, partitions, and app

set -e  # Exit on error

FIRMWARE_VERSION="v1.1.3"
BUILD_DIR=".pio/build/seeed_xiao_esp32s3"
OUTPUT_NAME="KB1-firmware-${FIRMWARE_VERSION}.bin"

echo "======================================"
echo "KB1 Factory Firmware Builder"
echo "Version: ${FIRMWARE_VERSION}"
echo "======================================"

# Clean and build firmware
echo "Cleaning previous build..."
python3 -m platformio run --target clean --environment seeed_xiao_esp32s3

echo "Building firmware..."
python3 -m platformio run --environment seeed_xiao_esp32s3

# Check if all required files exist
if [ ! -f "${BUILD_DIR}/bootloader.bin" ]; then
    echo "ERROR: bootloader.bin not found!"
    exit 1
fi

if [ ! -f "${BUILD_DIR}/partitions.bin" ]; then
    echo "ERROR: partitions.bin not found!"
    exit 1
fi

if [ ! -f "${BUILD_DIR}/firmware.bin" ]; then
    echo "ERROR: firmware.bin not found!"
    exit 1
fi

# Find esptool
ESPTOOL="${HOME}/.platformio/packages/tool-esptoolpy/esptool.py"

if [ ! -f "${ESPTOOL}" ]; then
    echo "ERROR: esptool.py not found at ${ESPTOOL}"
    exit 1
fi

# Create factory image with all components
echo "Creating factory image..."
python3 "${ESPTOOL}" --chip esp32s3 merge_bin \
    -o "${OUTPUT_NAME}" \
    --flash_mode dio \
    --flash_freq 80m \
    --flash_size 8MB \
    0x0 "${BUILD_DIR}/bootloader.bin" \
    0x8000 "${BUILD_DIR}/partitions.bin" \
    0x10000 "${BUILD_DIR}/firmware.bin"

echo ""
echo "======================================"
echo "✅ Factory firmware created successfully!"
echo "======================================"
echo "File: ${OUTPUT_NAME}"
ls -lh "${OUTPUT_NAME}"
echo ""
echo "MD5:"
md5 "${OUTPUT_NAME}" | awk '{print $4}'
echo ""
echo "Version info:"
strings "${OUTPUT_NAME}" | grep -E "(KB1 FIRMWARE|Build Date)" | head -2
echo ""
echo "⚠️  IMPORTANT: Flash complete image at offset 0x0"
echo "This includes bootloader + partitions + app"
echo "======================================"
