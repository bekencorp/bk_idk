export PROPERTIES_PROJECT_DIR := projects/properties_libs
ARMINO_DIR := $(CURDIR)
ARMINO_TOOL := $(ARMINO_DIR)/tools/build_tools/armino
ARMINO_TOOL_WRAPPER := $(ARMINO_DIR)/tools/build_tools/build.sh
# 1. soc_targets contains all supported SoCs
# 2. cmake_supported_targets contains all targets that can directly
#    passed to armino cmake build system
# 3. cmake_not_supported_targets contains all targets:
#    3.1> armino cmake doesn't support it, only implemented in this
#         Makefile
#    3.2> armino cmake supports it, but has different target name

soc_targets := $(shell find  middleware/soc/ -name "*.defconfig" -exec basename {} \; | cut -f1 -d ".")
properties_lib_targets := $(subst bk, libbk, $(soc_targets))
rel_targets := $(subst bk, relbk, $(soc_targets))
clean_targets := $(subst bk, cleanbk, $(soc_targets))
doc_targets := $(subst bk, docbk, $(soc_targets))
cmake_supported_targets := menuconfig doc
cmake_not_supported_targets = help clean
all_targets = cmake_not_supported_targets soc_targets cmake_supported_targets
export SOC_SUPPORTED_TARGETS := ${soc_targets}

export ARMINO_SOC := $(findstring $(MAKECMDGOALS), $(soc_targets))
export ARMINO_SOC_LIB := $(findstring $(MAKECMDGOALS), $(properties_lib_targets))
export CMD_TARGET := $(MAKECMDGOALS)
export BUILD_TARGET := $(findstring $(MAKECMDGOALS), $(cmake_supported_targets))

ifeq ("$(APP_VERSION)", "")
	export APP_VERSION := unknow
else
	export APP_VERSION := $(APP_VERSION)
endif


ifeq ("$(PROJECT)", "")
	ifneq ("$(INTERNAL)", "")
		export PROJECT := $(INTERNAL)
		export PROJECT_DIR := $(ARMINO_DIR)/properties/projects/$(PROJECT)
		export PROPERTIES_PROJECT_DIR := properties/projects/properties_libs
	else
		export PROJECT := app
	endif
else
	export PROJECT := $(PROJECT)
endif

ifeq ("$(PROJECT_LIBS)", "")
	export PROJECT_LIBS := $(PROJECT)
else
	export PROJECT_LIBS := $(PROJECT_LIBS)
endif

ifeq ("$(PROJECT_DIR)", "")
	export PROJECT_DIR := $(ARMINO_DIR)/projects/$(PROJECT)
else
	export PROJECT_DIR := $(PROJECT_DIR)
endif

# Check if PROJECT is a relative path and convert to absolute
ifeq ($(patsubst /%,,$(PROJECT)),$(PROJECT))
	export PROJECT := $(ARMINO_DIR)/projects/$(PROJECT)
endif

ifeq ("$(ARMINO_SOC)", "")
ifeq ("$(ARMINO_SOC_LIB)", "")
	ARMINO_SOC := bk7236
	ARMINO_TARGET := $(MAKECMDGOALS)
endif
else
	ARMINO_TARGET := build
endif

export ARMINO_SOC_NAME ?= $(ARMINO_SOC)
export PROJECT_NAME := $(notdir $(PROJECT_DIR))
ifdef BUILD_DIR
	export PROJECT_BUILD_DIR := $(abspath $(BUILD_DIR)/$(ARMINO_SOC_NAME)/$(PROJECT_NAME))
else
	export PROJECT_BUILD_DIR := $(CURDIR)/build/$(ARMINO_SOC_NAME)/$(PROJECT_NAME)
endif

ifeq ($(findstring Windows_NT,$(OS)), Windows_NT)
    WIN32 := 1
	export PYTHONPATH := $(ARMINO_DIR)/tools/env_tools/bk_py_libs;$(PYTHONPATH)
else
	WIN32 := 0
	export PYTHONPATH := $(ARMINO_DIR)/tools/env_tools/bk_py_libs:$(PYTHONPATH)
endif

# overwrite project config
-include $(ARMINO_DIR)/middleware/soc/$(ARMINO_SOC)/soc_config.mk
export COMPILER_TOOLCHAIN_PATH := $(COMPILER_TOOLCHAIN_PATH)
-include $(PROJECT_DIR)/pj_config.mk

.PHONY: all_targets

help:
	@echo ""
	@echo " make bkxxx - build soc bkxxx"
	@echo " make build - fast build last soc"
	@echo " make all - build all soc"
	@echo " make libbkxxx - build properties libs for soc bkxxx"
	@echo " make liball - build all properties libs for all soc"
	@echo " make menuconfig - confiure armino"
	@echo " make clean - clean build"
	@echo " make help - display this help info"
	@echo " make doc - generate all doc"
	@echo " make docbk72xx - generate doc for bk72xx"
	@echo ""

common:
	@echo "ARMINO_SOC is set to $(ARMINO_SOC)"
	@echo "ARMINO_TARGET is set to $(ARMINO_TARGET)"
	@echo "armino project path=$(PROJECT_DIR)"
	@echo "armino path=$(ARMINO_DIR)"
	@echo "armino build path=$(ARMINO_DIR)/$(PROJECT_BUILD_DIR)"
	@export ARMINO_PATH=$(ARMINO_DIR)
	@export APP_NAME=$(APP_NAME)

is_secure_build := $(shell python3 $(ARMINO_DIR)/tools/build_tools/parse_build_type.py ${PROJECT_DIR} ${ARMINO_SOC})
ifeq ($(is_secure_build), 1)
BUILD_TYPE := secure_build
else
BUILD_TYPE := nonsecure_build
endif

has_lib_src := $(shell python3 $(ARMINO_DIR)/tools/build_tools/detect_internal_lib_src.py)
ifeq ($(has_lib_src), 1)
ifneq ("$(APP_VERSION)", "verify")
$(properties_lib_targets): common
	@$(ARMINO_TOOL_WRAPPER) $(ARMINO_DIR) $(PROJECT_DIR) $(PROJECT_BUILD_DIR) $@
endif
endif

liball: $(properties_lib_targets)

export BUILD_TARGETS := ${ARMINO_SOC_NAME} ${PRE_BUILD_TARGET}

# verify enable multithread build.
ifdef BK_JENKINS_ID
BUILD_TOOLCHAIN_DIR := $(ARMINO_DIR)/tools/build_tools/toolchain/gcc-arm-none-eabi-10.3-2024.08
ifneq ("$(wildcard $(BUILD_TOOLCHAIN_DIR))", "")
MAKEFLAGS += -j$(shell echo $(BUILD_TARGETS) | wc -w)
else
endif
endif

all: $(soc_targets)

$(rel_targets):
	@$(ARMINO_TOOL_WRAPPER) $(ARMINO_DIR) $(PROJECT_DIR) $(PROJECT_NAME) $@

relall: $(rel_targets)

$(cmake_supported_targets): common
ifeq ($(filter $(BUILD_TARGET), menuconfig doc), $(BUILD_TARGET))
	@$(MAKE) gen_sys_persist_config
	@$(ARMINO_TOOL) -B $(PROJECT_BUILD_DIR) -P $(PROJECT_DIR) $@
else
	@$(ARMINO_TOOL_WRAPPER) $(ARMINO_DIR) $(PROJECT_DIR) $(PROJECT_BUILD_DIR) $@
endif

$(clean_targets):
	@$(ARMINO_TOOL_WRAPPER) $(ARMINO_DIR) $(PROJECT_DIR) $(PROJECT_NAME) $@

$(doc_targets):
	@$(ARMINO_TOOL_WRAPPER) $(ARMINO_DIR) $(PROJECT_DIR) $(PROJECT_NAME) $@

ifeq ($(findstring Windows_NT,$(OS)), Windows_NT)
export WIN32 := 1
PRINT_SUMMARY := 0
export PYTHONPATH := $(ARMINO_DIR)/tools/env_tools/bk_py_libs;$(PYTHONPATH)
else
export WIN32 := 0
export PYTHONPATH := $(ARMINO_DIR)/tools/env_tools/bk_py_libs:$(PYTHONPATH)
endif


AUTO_PARTITION_TABLE := $(PROJECT_DIR)/config/$(ARMINO_SOC_NAME)/auto_partitions.csv
export PARTITIONS_DIR := $(PROJECT_BUILD_DIR)/partitions
export SECURITY_PREBUILD_DIR := $(PROJECT_BUILD_DIR)/security
export SYS_PERSIST_CONFIG_DIR := $(PROJECT_BUILD_DIR)/sys_persist_config
auto_partition_script := $(ARMINO_DIR)/tools/build_tools/build_process/bk_build_auto_partition.py
auto_partition_out := $(PARTITIONS_DIR)/partitions.txt

security_prebuild_script := $(ARMINO_DIR)/tools/build_tools/build_process/bk_sdk/bk_prebuild.py

sys_persist_config_script := $(ARMINO_DIR)/tools/build_tools/sys_persist_config/sys_persist_config_tool.py
sys_persist_config_bin := $(SYS_PERSIST_CONFIG_DIR)/sys_persist_config.bin

$(auto_partition_out): $(auto_partition_script) $(AUTO_PARTITION_TABLE)
	@mkdir -p $(PARTITIONS_DIR)
	@python3 $(auto_partition_script)

print_partitions: $(auto_partition_out)
	@echo ===================== Partitions Table =====================
	@cat $(auto_partition_out)
	@echo ============================================================

gen_sys_persist_config: $(sys_persist_config_script)
	@mkdir -p $(SYS_PERSIST_CONFIG_DIR)
	@python3 $(sys_persist_config_script)  --soc $(ARMINO_SOC_NAME) --project_dir $(PROJECT_DIR) --armino_dir $(ARMINO_DIR) --out_dir $(SYS_PERSIST_CONFIG_DIR) csv2bin
	@python3 $(sys_persist_config_script)  --soc $(ARMINO_SOC_NAME) --project_dir $(PROJECT_DIR) --armino_dir $(ARMINO_DIR) --out_dir $(SYS_PERSIST_CONFIG_DIR) gen_code

security_prebuild: $(security_prebuild_script) gen_sys_persist_config
	@mkdir -p $(SECURITY_PREBUILD_DIR)
	@python3 $(security_prebuild_script) --soc $(ARMINO_SOC_NAME) --project_dir $(PROJECT_DIR) --armino_dir $(ARMINO_DIR) --build_dir $(SECURITY_PREBUILD_DIR) --secure $(is_secure_build)

nonsecure_prebuild: print_partitions security_prebuild

$(ARMINO_SOC_NAME)_ns_cp0: nonsecure_prebuild
	@$(ARMINO_TOOL_WRAPPER) $(ARMINO_DIR) $(PROJECT_DIR) $(PROJECT_BUILD_DIR) ${ARMINO_SOC_NAME}

pack_sys_persist_config_script := $(ARMINO_DIR)/tools/build_tools/build_files/pack_sys_persist_config.py
nonsecure_pack_sys_persist_config: $(pack_sys_persist_config_script) $(ARMINO_SOC_NAME)_ns_cp0
	@python3 $(pack_sys_persist_config_script) --project_build_dir $(PROJECT_BUILD_DIR) --armino_soc $(ARMINO_SOC_NAME) --sys_persist_config_dir $(SYS_PERSIST_CONFIG_DIR)

secure_pack_sys_persist_config: $(pack_sys_persist_config_script) $(ARMINO_SOC_NAME)_s_cp0
	@python3 $(pack_sys_persist_config_script) --project_build_dir $(PROJECT_BUILD_DIR) --armino_soc $(ARMINO_SOC_NAME) --sys_persist_config_dir $(SYS_PERSIST_CONFIG_DIR)

nonsecure_package_script := $(ARMINO_DIR)/tools/build_tools/build_process/bk_build_package.py
package_dir := $(PROJECT_BUILD_DIR)/package
package_json := $(PARTITIONS_DIR)/bk_package.json
build_summary := $(package_dir)/build_summary.txt
nonsecure_package: $(nonsecure_package_script) nonsecure_pack_sys_persist_config ${PRE_BUILD_TARGET}
	@mkdir -p $(package_dir)
	@python3 $(nonsecure_package_script) $(PROJECT_BUILD_DIR) $(package_json) $(build_summary)
ifneq ($(PRINT_SUMMARY), 0)
	@cat $(build_summary)
endif

$(ARMINO_SOC_NAME)_s_cp0: security_prebuild
	@$(ARMINO_TOOL_WRAPPER) $(ARMINO_DIR) $(PROJECT_DIR) $(PROJECT_BUILD_DIR) ${ARMINO_SOC_NAME}

secure_package_script := $(ARMINO_DIR)/tools/build_tools/secure_pack.py
secure_package: $(secure_package_script) secure_pack_sys_persist_config $(ARMINO_SOC_NAME)_s_cp0 ${PRE_BUILD_TARGET}
	@python3 $(secure_package_script) $(ARMINO_SOC_NAME) $(ARMINO_DIR) $(PROJECT_BUILD_DIR) $(PROJECT_DIR)

nonsecure_build: nonsecure_package
secure_build: secure_package

postbuild_process_script := $(ARMINO_DIR)/tools/build_tools/postbuild_process.sh
${ARMINO_SOC_NAME}: $(BUILD_TYPE)
	@$(postbuild_process_script) ${ARMINO_SOC_NAME} $(ARMINO_DIR) $(PROJECT_BUILD_DIR)

clean:
	-@$(ARMINO_TOOL_PART_TABLE) $(DEFAULT_CSV_FILE) $(PARTITIONS_ARGS) $(CLEAN_ALLFILE_INSEQ)
	@echo "rm -rf ./build"
	@python3 tools/build_tools/armino_doc.py clean
	@rm -rf ./build

