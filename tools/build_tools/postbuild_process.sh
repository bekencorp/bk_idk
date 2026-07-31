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
if [ "${PROJECT_NAME:-}" = "ate_mini_code" ] || [ "${PROJECT:-}" = "ate_mini_code" ]; then
	BOOTLOADER_BIN="${ARMINO_DIR}/components/bk_libs/${ARMINO_SOC}/bootloader/ate_bootloader/bootloader.bin"
fi

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

SPID_OFFSET_IN_BOOT_DEFAULT=8
SPID_OFFSET_IN_BOOT="${SPID_OFFSET_IN_BOOT_DEFAULT}"
PROJECT_CONFIG_FILE=""
if [ -n "${PROJECT_DIR:-}" ]; then
    if [ "${PROJECT_DIR#/}" != "${PROJECT_DIR}" ]; then
        PROJECT_CONFIG_FILE="${PROJECT_DIR}/config/${ARMINO_SOC}/config"
    else
        PROJECT_CONFIG_FILE="${ARMINO_DIR}/${PROJECT_DIR}/config/${ARMINO_SOC}/config"
    fi
elif [ -n "${BUILD_DIR:-}" ]; then
    PROJECT_NAME_FROM_BUILD_DIR=$(basename "${BUILD_DIR}")
    PROJECT_CONFIG_FILE="${ARMINO_DIR}/projects/${PROJECT_NAME_FROM_BUILD_DIR}/config/${ARMINO_SOC}/config"
fi
if [ -n "${PROJECT_CONFIG_FILE}" ] && [ -f "${PROJECT_CONFIG_FILE}" ]; then
    CONFIG_SPID_OFFSET_RAW=$(grep '^CONFIG_SPID_OFFSET_IN_BOOT=' "${PROJECT_CONFIG_FILE}" | cut -d '=' -f 2 | tr -d '[:space:]' | tail -n 1)
    if [ -n "${CONFIG_SPID_OFFSET_RAW}" ]; then
        if PARSED_SPID_OFFSET=$(python3 -c 'import sys; print(int(sys.argv[1], 0))' "${CONFIG_SPID_OFFSET_RAW}" 2>/dev/null); then
            if [ "${PARSED_SPID_OFFSET}" = "8" ] || [ "${PARSED_SPID_OFFSET}" = "10" ]; then
                SPID_OFFSET_IN_BOOT="${PARSED_SPID_OFFSET}"
            else
                echo "Error: CONFIG_SPID_OFFSET_IN_BOOT='${CONFIG_SPID_OFFSET_RAW}' is unsupported, only 0x8 or 0xA is allowed"
                exit 1
            fi
        else
            echo "Error: Invalid CONFIG_SPID_OFFSET_IN_BOOT='${CONFIG_SPID_OFFSET_RAW}' in ${PROJECT_CONFIG_FILE}"
            exit 1
        fi
    fi
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

# Read FLASH_CRC_ENABLE from auto_partitions.csv (non-secure build, same as bk_build_package.py)
read_crc_mode_from_auto_partitions() {
	local csv="$1"
	local line val

	[ -f "${csv}" ] || return 1
	while IFS= read -r line || [ -n "${line}" ]; do
		line=$(echo "${line}" | sed 's/^[[:space:]]*//;s/[[:space:]]*$//')
		[ -z "${line}" ] && continue
		case "${line}" in
			\#*) continue ;;
		esac
		case "${line}" in
			*FLASH_CRC_ENABLE*)
				val=$(echo "${line}" | cut -d'=' -f2- | tr -d '[:space:]' | tr '[:lower:]' '[:upper:]')
				case "${val}" in
					FALSE|0) CRC_MODE="NO_CRC" ;;
					TRUE|1) CRC_MODE="CRC" ;;
				esac
				return 0
				;;
		esac
	done < "${csv}"
	return 1
}

# Copy all-app.bin and rename it with spid and timestamp
if [ -f "${ALL_APP_BIN}" ] && [ -n "${SPID}" ] && [ "${SPID}" != "0" ]; then
	SECURITY_CSV=""
	if [ -f "${BUILD_DIR}/package/security.csv" ]; then
		SECURITY_CSV="${BUILD_DIR}/package/security.csv"
	elif [ -n "${PROJECT_DIR:-}" ] && [ -f "${PROJECT_DIR}/config/${ARMINO_SOC}/security.csv" ]; then
		SECURITY_CSV="${PROJECT_DIR}/config/${ARMINO_SOC}/security.csv"
	fi

	AUTO_PARTITIONS_CSV=""
	if [ -n "${PROJECT_DIR:-}" ] && [ -f "${PROJECT_DIR}/config/${ARMINO_SOC}/auto_partitions.csv" ]; then
		AUTO_PARTITIONS_CSV="${PROJECT_DIR}/config/${ARMINO_SOC}/auto_partitions.csv"
	fi

	# Secure: flash_crc_en in security.csv.
	# Non-secure: FLASH_CRC_ENABLE in auto_partitions.csv.
	# Default: CRC.
	CRC_MODE="CRC"
	FLASH_CRC_VALUE=""
	if [ -n "${SECURITY_CSV}" ]; then
		FLASH_CRC_VALUE=$(grep '^flash_crc_en,' "${SECURITY_CSV}" 2>/dev/null | cut -d',' -f2 | tr -d '[:space:]' | tr '[:lower:]' '[:upper:]')
	fi
	if [ -n "${FLASH_CRC_VALUE}" ]; then
		if [ "${FLASH_CRC_VALUE}" = "FALSE" ]; then
			CRC_MODE="NO_CRC"
		fi
	elif [ -n "${AUTO_PARTITIONS_CSV}" ]; then
		read_crc_mode_from_auto_partitions "${AUTO_PARTITIONS_CSV}" || true
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
python3 -B "${WRITE_PRODUCT_ID_SCRIPT}" "${ALL_APP_BIN}" "${PRODUCT_STRING}" --spid-offset-in-boot "${SPID_OFFSET_IN_BOOT}"

if [ $? -eq 0 ]; then
	echo "Product ID write operation completed successfully"
else
	echo "Error: Product ID write operation failed"
	exit 1
fi
