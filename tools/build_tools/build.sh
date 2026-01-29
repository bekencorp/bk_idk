#!/bin/bash

set -e
set -u

ARMINO_TARGET="build"
ARMINO_DIR=$1
PROJECT_DIR=$2
BUILD_DIR=$3
BUILD_TARGET=$4

python_isabs_cmd="import os;print(os.path.isabs(\"$BUILD_DIR\"));"
if [ `python3 -c "$python_isabs_cmd"` == "False" ];then
	BUILD_DIR=${ARMINO_DIR}/${BUILD_DIR}
fi

SDK_BUILD_DIR=`dirname ${BUILD_DIR}`

ARMINO_TOOL=${ARMINO_DIR}/tools/build_tools/armino
BUILD_TARGET_PREFIX=${BUILD_TARGET:0:5}

need_build_properties_lib=0
need_build_soc=0
need_clean=0
need_doc=0

if [ "${BUILD_TARGET_PREFIX}" == "libbk" ]; then
	ARMINO_SOC=${BUILD_TARGET#lib}
	need_build_properties_lib=1
	BUILD_DIR=build/properties_libs/${ARMINO_SOC}
	PROJECT_DIR=projects/properties_libs
	echo "build armino ${ARMINO_SOC} properties libs only"
	export LIB_HASH="NULL"
elif [ "${BUILD_TARGET_PREFIX}" == "relbk" ]; then
	ARMINO_SOC=${BUILD_TARGET#rel}
	BUILD_DIR=build/${BUILD_DIR}/${ARMINO_SOC}
	need_build_properties_lib=0
	need_build_soc=1
	echo "build armino ${ARMINO_SOC} only"
	export LIB_HASH="NULL"
elif [ "${BUILD_TARGET_PREFIX}" == "clean" ]; then
	ARMINO_SOC=${BUILD_TARGET#clean}
	BUILD_DIR=build/${BUILD_DIR}/${ARMINO_SOC}
	need_clean=1
elif [ "${BUILD_TARGET_PREFIX}" == "docbk" ]; then
	ARMINO_SOC=${BUILD_TARGET#doc}
	BUILD_DIR=build/${BUILD_DIR}/${ARMINO_SOC}
	echo "build armino ${ARMINO_SOC} doc only"
	need_doc=1
elif [ "${APP_VERSION}" == "verify" ]; then
	ARMINO_SOC=${BUILD_TARGET}
	BUILD_DIR=${BUILD_DIR}/${ARMINO_SOC}
	need_build_soc=1
	need_build_properties_lib=0
	compute_hash_tool=${ARMINO_DIR}/tools/build_tools/compute_files_hash.py
	export LIB_HASH=$(python3 ${compute_hash_tool} ${ARMINO_DIR}/components/bk_libs/${ARMINO_SOC}/libs)
else
	ARMINO_SOC=${BUILD_TARGET}
	BUILD_DIR=${BUILD_DIR}/${ARMINO_SOC}
	need_build_soc=1
	has_properties_lib_src=$(${ARMINO_DIR}/tools/build_tools/detect_internal_lib_src.py)
	if [ ${has_properties_lib_src} == "1" ]; then
		echo "build armino properties lib first, then ${ARMINO_SOC}"
		need_build_properties_lib=1
		export LIB_HASH="NULL"
	else
		need_build_properties_lib=0
		compute_hash_tool=${ARMINO_DIR}/tools/build_tools/compute_files_hash.py
		export LIB_HASH=$(python3 ${compute_hash_tool} ${ARMINO_DIR}/components/bk_libs/${ARMINO_SOC}/libs)
	fi
fi

PROPERTIES_LIB_BUILD_DIR=${SDK_BUILD_DIR}/properties_libs/${ARMINO_SOC}
PROPERTIES_LIB_DIR=${PROPERTIES_PROJECT_DIR}
WRITE_PRODUCT_ID_SCRIPT="${ARMINO_DIR}/tools/build_tools/write_product_id.py"
PRODUCT_ID_FILE="${ARMINO_DIR}/properties/soc/${ARMINO_SOC}/spid.txt"
NORMAL_BOOTLOADER_BIN="${ARMINO_DIR}/components/bk_libs/${ARMINO_SOC}/bootloader/normal_bootloader/bootloader.bin"
AB_BOOTLOADER_BIN="${ARMINO_DIR}/components/bk_libs/${ARMINO_SOC}/bootloader/ab_bootloader/bootloader.bin"
SDKCONFIG_H="${PROPERTIES_LIB_BUILD_DIR}/sdkconfig"
# not to build properties repeatedly when verify.
if [ -n "${BK_JENKINS_ID:-}" ]; then
	if [ -d "$PROPERTIES_LIB_BUILD_DIR" ]; then
		need_build_properties_lib=0
	fi
fi

if [ "${need_clean}" == "1" ]; then
	echo "remove ${ARMINO_DIR}/components/bk_libs/${ARMINO_SOC}"
	rm -rf ${ARMINO_DIR}/components/bk_libs/${ARMINO_SOC}
	echo "remove ${BUILD_DIR}"
	rm -rf ${BUILD_DIR}
	echo "remove ${PROPERTIES_LIB_BUILD_DIR}"
	rm -rf ${PROPERTIES_LIB_BUILD_DIR}
	python ${ARMINO_DIR}/tools/build_tools/armino_doc.py clean ${ARMINO_SOC}
	exit 0
fi

if [ "${need_doc}" == "1" ]; then
	${ARMINO_TOOL} -B ${BUILD_DIR} -P ${PROJECT_DIR} doc ${ARMINO_SOC}
	exit 0
fi

if [ "${need_build_properties_lib}" == "1" ]; then
	echo "build properties lib for ${ARMINO_SOC}"
	rm -rf ${PROPERTIES_LIB_BUILD_DIR}/sdkconfig
	${ARMINO_TOOL} -B ${PROPERTIES_LIB_BUILD_DIR} -P ${PROPERTIES_LIB_DIR} set-target ${ARMINO_SOC}
	${ARMINO_TOOL} -B ${PROPERTIES_LIB_BUILD_DIR} -P ${PROPERTIES_LIB_DIR} ${ARMINO_TARGET}
	${ARMINO_DIR}/tools/build_tools/copy_internal_libs.sh ${ARMINO_SOC} ${ARMINO_DIR} ${PROPERTIES_LIB_BUILD_DIR} ${PROJECT}

	if [ -f "${PRODUCT_ID_FILE}" ] && [ -f "${WRITE_PRODUCT_ID_SCRIPT}" ]; then
		PRODUCT_STRING=$(cat "${PRODUCT_ID_FILE}")
		PRODUCT_STRING=$(echo "${PRODUCT_STRING}" | xargs)

		# Parse CONFIG_PRODUCT_STRING from sdkconfig
		if [ ! -f "${SDKCONFIG_H}" ]; then
			echo "Error: sdkconfig.h not found at ${SDKCONFIG_H}"
			exit 1
		fi

		CONFIG_PRODUCT_ID=$(grep "CONFIG_PRODUCT_ID" "${SDKCONFIG_H}" | cut -d '=' -f 2)

		if [ "${CONFIG_PRODUCT_ID}" != "y" ]; then
			PRODUCT_STRING="0"
		fi

		if [ -n "${PRODUCT_STRING}" ]; then
			if [ -f "${NORMAL_BOOTLOADER_BIN}" ]; then
				echo "Writing product ID to ${NORMAL_BOOTLOADER_BIN}..."
				python3 "${WRITE_PRODUCT_ID_SCRIPT}" "${NORMAL_BOOTLOADER_BIN}" "${PRODUCT_STRING}"
				if [ $? -eq 0 ]; then
					echo "Product ID write to normal bootloader completed successfully"
				else
					echo "Warning: Product ID write to normal bootloader failed, continuing build"
				fi
			fi
			
			if [ -f "${AB_BOOTLOADER_BIN}" ]; then
				echo "Writing product ID to ${AB_BOOTLOADER_BIN}..."
				python3 "${WRITE_PRODUCT_ID_SCRIPT}" "${AB_BOOTLOADER_BIN}" "${PRODUCT_STRING}"
				if [ $? -eq 0 ]; then
					echo "Product ID write to AB bootloader completed successfully"
				else
					echo "Warning: Product ID write to AB bootloader failed, continuing build"
				fi
			fi
		fi
	fi

fi

if [ "${need_build_soc}" == "1" ]; then
	echo "build ${ARMINO_SOC}"
	rm -rf ${BUILD_DIR}/sdkconfig
	${ARMINO_TOOL} -B ${BUILD_DIR} -P ${PROJECT_DIR} set-target ${ARMINO_SOC}
	${ARMINO_TOOL} -B ${BUILD_DIR} -P ${PROJECT_DIR} ${ARMINO_TARGET}
	${ARMINO_DIR}/tools/build_tools/armino_as_lib.sh ${ARMINO_SOC} ${ARMINO_DIR} ${BUILD_DIR} ${PROJECT}
fi

