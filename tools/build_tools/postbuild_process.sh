#!/bin/bash

set -e
set -u

ARMINO_SOC=$1
ARMINO_DIR=$2
BUILD_DIR=$3

# Write product ID to all-app.bin
WRITE_PRODUCT_ID_SCRIPT="${ARMINO_DIR}/tools/build_tools/write_product_id.py"
ALL_APP_BIN="${BUILD_DIR}/package/all-app.bin"
# Read product_id from bootloader.bin file (read 16 bytes from 0x118 offset)
BOOTLOADER_BIN="${ARMINO_DIR}/components/bk_libs/${ARMINO_SOC}/bootloader/normal_bootloader/bootloader.bin"

# Check if required files exist
if [ ! -f "${WRITE_PRODUCT_ID_SCRIPT}" ]; then
	exit 1
fi

if [ ! -f "${ALL_APP_BIN}" ]; then
	echo "Error: all-app.bin not found at ${ALL_APP_BIN}"
	exit 1
fi

# Check if bootloader.bin exists
if [ ! -f "${BOOTLOADER_BIN}" ]; then
    echo "Error: bootloader.bin not found at ${BOOTLOADER_BIN}"
    exit 1
fi

PRODUCT_BYTES=$(dd if="${BOOTLOADER_BIN}" bs=1 skip=$((0x118)) count=16 2>/dev/null | xxd -p -c 16)
# Convert hexadecimal data back to ASCII string (only take first 15 bytes, ignore CRC byte)
PRODUCT_STRING=$(echo "${PRODUCT_BYTES:0:30}" | sed 's/../& /g' | xxd -r -p | tr -d '\0')

if [ -z "${PRODUCT_STRING}" ]; then
    echo "Product ID is empty in ${BOOTLOADER_BIN}"
    exit 0
fi

# Trim whitespace
PRODUCT_STRING=$(echo "${PRODUCT_STRING}" | xargs)

echo "Found PRODUCT_STRING: ${PRODUCT_STRING}"
echo "Writing product ID to ${ALL_APP_BIN}..."

# Call write_product_id.py with PRODUCT_STRING as product_id parameter
python3 "${WRITE_PRODUCT_ID_SCRIPT}" "${ALL_APP_BIN}" "${PRODUCT_STRING}"

if [ $? -eq 0 ]; then
	echo "Product ID write operation completed successfully"
else
	echo "Error: Product ID write operation failed"
	exit 1
fi
