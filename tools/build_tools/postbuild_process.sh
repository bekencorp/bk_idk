#!/bin/bash

set -e
set -u

ARMINO_SOC=$1
ARMINO_DIR=$2
BUILD_DIR=$3

# Write product ID to all-app.bin
WRITE_PRODUCT_ID_SCRIPT="${ARMINO_DIR}/tools/build_tools/write_product_id.py"
ALL_APP_BIN="${BUILD_DIR}/package/all-app.bin"
PRODUCT_ID_FILE="${ARMINO_DIR}/middleware/boards/${ARMINO_SOC}/product_ids.txt"
SDKCONFIG_H="${BUILD_DIR}/${ARMINO_SOC}/config/sdkconfig.h"

# Check if PRODUCT_ID_FILE exists, exit if not
if [ ! -f "${PRODUCT_ID_FILE}" ]; then
	echo "product_ids.txt not found at ${PRODUCT_ID_FILE}"
	exit 0
fi

# Check if required files exist
if [ ! -f "${WRITE_PRODUCT_ID_SCRIPT}" ]; then
	exit 1
fi

if [ ! -f "${ALL_APP_BIN}" ]; then
	echo "Error: all-app.bin not found at ${ALL_APP_BIN}"
	exit 1
fi

# Parse CONFIG_PRODUCT_STRING from sdkconfig.h
if [ ! -f "${SDKCONFIG_H}" ]; then
	echo "Error: sdkconfig.h not found at ${SDKCONFIG_H}"
	exit 1
fi

# Extract CONFIG_PRODUCT_STRING value from sdkconfig.h
# Pattern: #define CONFIG_PRODUCT_STRING "value" or #define CONFIG_PRODUCT_STRING value
# Handle both quoted and unquoted values
CONFIG_PRODUCT_STRING=$(grep -E '^#define\s+CONFIG_PRODUCT_STRING' "${SDKCONFIG_H}" | sed -E 's/^#define\s+CONFIG_PRODUCT_STRING\s+//' | sed -E 's/^"([^"]*)"$/\1/' | head -n 1)

if [ -z "${CONFIG_PRODUCT_STRING}" ]; then
	echo "CONFIG_PRODUCT_STRING not found in ${SDKCONFIG_H}"
	exit 0
fi

# Trim whitespace
CONFIG_PRODUCT_STRING=$(echo "${CONFIG_PRODUCT_STRING}" | xargs)

echo "Found CONFIG_PRODUCT_STRING: ${CONFIG_PRODUCT_STRING}"
echo "Writing product ID to ${ALL_APP_BIN}..."

# Call write_product_id.py with CONFIG_PRODUCT_STRING as product_id parameter
python3 "${WRITE_PRODUCT_ID_SCRIPT}" "${ALL_APP_BIN}" "${CONFIG_PRODUCT_STRING}"

if [ $? -eq 0 ]; then
	echo "Product ID write operation completed successfully"
else
	echo "Error: Product ID write operation failed"
	exit 1
fi
