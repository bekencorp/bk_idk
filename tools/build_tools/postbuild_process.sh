#!/bin/bash

set -e
set -u

ARMINO_SOC=$1
ARMINO_DIR=$2
BUILD_DIR=$3

# Write product ID to all-app.bin
WRITE_PRODUCT_ID_SCRIPT="${ARMINO_DIR}/tools/build_tools/write_product_id.py"
ALL_APP_BIN="${BUILD_DIR}/package/all-app.bin"
PRODUCT_ID_FILE="${ARMINO_DIR}/middleware/boards/${ARMINO_SOC}/product_id.txt"

if [ -f "${WRITE_PRODUCT_ID_SCRIPT}" ] && [ -f "${ALL_APP_BIN}" ]; then
	if [ -f "${PRODUCT_ID_FILE}" ]; then
		# Read product ID from product_id.txt for the specific SOC
		echo "Writing product ID to ${ALL_APP_BIN} (from ${PRODUCT_ID_FILE})..."
		python3 "${WRITE_PRODUCT_ID_SCRIPT}" "${ALL_APP_BIN}" --product-id-file "${PRODUCT_ID_FILE}"
	else
		# Fallback: if product_id.txt doesn't exist, skip
		echo "Warning: product_id.txt not found at ${PRODUCT_ID_FILE}"
		echo "Skipping product ID write operation"
	fi
	
	if [ $? -eq 0 ]; then
		echo "Product ID write operation completed"
	else
		echo "Warning: Product ID write operation returned error"
	fi
else
	if [ ! -f "${WRITE_PRODUCT_ID_SCRIPT}" ]; then
		echo "Warning: write_product_id.py not found at ${WRITE_PRODUCT_ID_SCRIPT}"
	fi
	if [ ! -f "${ALL_APP_BIN}" ]; then
		echo "Warning: all-app.bin not found at ${ALL_APP_BIN}"
	fi
fi
