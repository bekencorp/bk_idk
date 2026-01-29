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

PRODUCT_BYTES=$(dd if="${BOOTLOADER_BIN}" bs=1 skip=$((0x108)) count=16 2>/dev/null | xxd -p -c 16)
# Convert hexadecimal data back to ASCII string (only take first 15 bytes, ignore CRC byte)
PRODUCT_STRING=$(echo "${PRODUCT_BYTES:0:30}" | sed 's/../& /g' | xxd -r -p | tr -d '\0')

if [ -z "${PRODUCT_STRING}" ]; then
    echo "Product ID is empty in ${BOOTLOADER_BIN}"
    exit 0
fi

# Trim whitespace
PRODUCT_STRING=$(echo "${PRODUCT_STRING}" | xargs)
# Convert PRODUCT_STRING to SPID, remove trailing null characters and spaces, keep only valid characters
SPID=$(echo -n "${PRODUCT_STRING}" | tr -d '\0' | sed 's/[[:space:]]*$//' | xargs)
echo "Writing product ID to ${ALL_APP_BIN}..."

# Copy all-app.bin and rename it with spid and timestamp
if [ -f "${ALL_APP_BIN}" ] && [ -n "${SPID}" ] && [ "${SPID}" != "0" ]; then
	TIMESTAMP=$(date +"%Y%m%d_%H%M%S")
	NEW_FIRMWARE_NAME="${SPID}_${TIMESTAMP}.bin"
	NEW_FIRMWARE_PATH="${BUILD_DIR}/${NEW_FIRMWARE_NAME}"
	cp "${ALL_APP_BIN}" "${NEW_FIRMWARE_PATH}"
	if [ $? -eq 0 ]; then
		echo "Firmware copied and renamed to: ${NEW_FIRMWARE_NAME}"
	else
		echo "Warning: Failed to copy firmware to ${NEW_FIRMWARE_NAME}"
	fi
fi

# Call write_product_id.py with PRODUCT_STRING as product_id parameter
python3 "${WRITE_PRODUCT_ID_SCRIPT}" "${ALL_APP_BIN}" "${PRODUCT_STRING}"

if [ $? -eq 0 ]; then
	echo "Product ID write operation completed successfully"
else
	echo "Error: Product ID write operation failed"
	exit 1
fi
