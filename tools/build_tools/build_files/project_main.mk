PROJECT_PATH := $(CURDIR)

PROJECT_DIR := $(shell echo "$(PROJECT_PATH)" | sed 's/.*projects\///')
TARGET := $(MAKECMDGOALS)

ifeq ($(findstring Windows_NT,$(OS)), Windows_NT)
PROJECT_PATH := $(subst \,/,$(PROJECT_PATH))
SDK_DIR := $(subst \,/,$(SDK_DIR))
endif

ifeq ("$(TARGET)", "clean")
SOC_TARGET := dummy
else
SOC_TARGET := $(TARGET)
$(info "Project: $(PROJECT_DIR)")
$(info "SDK_DIR: $(SDK_DIR)")
$(info "TARGET: $(TARGET)")
endif

ifeq ("$(SOC_TARGET)", "")
$(error "please input soc target")
endif

PROJECT_NAME := $(notdir $(PROJECT_DIR))
PROJECT_BUILD_DIR := $(PROJECT_PATH)/build/$(PROJECT_NAME)

.PHONY: clean

$(SOC_TARGET):
	make $(SOC_TARGET) PROJECT=$(PROJECT_DIR) BUILD_DIR=$(PROJECT_BUILD_DIR) -C $(SDK_DIR)

clean:
	@echo "rm -rf ./build"
	@rm -rf ./build
