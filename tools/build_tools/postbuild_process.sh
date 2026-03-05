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
	# Detect CRC mode by checking for magic strings at specific offsets
	# Check for magic strings at 0x100 (no_crc) or 0x110 (crc)
	# Define magic strings array for easy maintenance
	MAGIC_STRINGS=("BEKEN" "BK.SB" "BK7236")

	CRC_MODE="NO_CRC"
	if [ -f "${ALL_APP_BIN}" ]; then
		# Read bytes at offset 0x100 and 0x110 using xxd to avoid null byte issues
		# xxd -p: plain hexdump, -l6: read 6 bytes, -s256: skip to offset 0x100 (0x100 = 256)
		MAGIC_100_HEX=$(xxd -p -l6 -s256 "${ALL_APP_BIN}" 2>/dev/null | tr -d '\n')
		MAGIC_110_HEX=$(xxd -p -l6 -s272 "${ALL_APP_BIN}" 2>/dev/null | tr -d '\n')

		# Convert hex to ASCII for comparison
		MAGIC_100_ASCII=$(echo "${MAGIC_100_HEX}" | xxd -r -p 2>/dev/null | tr -d '\0')
		MAGIC_110_ASCII=$(echo "${MAGIC_110_HEX}" | xxd -r -p 2>/dev/null | tr -d '\0')

		# Check for magic strings at 0x100 (no_crc mode)
		FOUND_AT_100=0
		if [ -n "${MAGIC_100_ASCII}" ]; then
			for magic in "${MAGIC_STRINGS[@]}"; do
				if echo -n "${MAGIC_100_ASCII}" | grep -qF "${magic}" 2>/dev/null; then
					CRC_MODE="NO_CRC"
					FOUND_AT_100=1
					break
				fi
			done
		fi

		# Check for magic strings at 0x110 (crc mode) - only if not found at 0x100
		if [ "${FOUND_AT_100}" -eq 0 ] && [ -n "${MAGIC_110_ASCII}" ]; then
			for magic in "${MAGIC_STRINGS[@]}"; do
				if echo -n "${MAGIC_110_ASCII}" | grep -qF "${magic}" 2>/dev/null; then
					CRC_MODE="CRC"
					break
				fi
			done
		fi
	fi

	TIMESTAMP=$(date +"%Y%m%d_%H%M%S")
	NEW_FIRMWARE_NAME="${SPID}_${CRC_MODE}_${TIMESTAMP}.bin"
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
