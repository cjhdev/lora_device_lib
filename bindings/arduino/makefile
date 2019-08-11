# the folder containing Arduino software
DIR_ARDUINO=/opt/arduino/arduino-1.8.5

# ldl project root folder
DIR_ROOT=../..

SPACE :=
SPACE +=

ARDUINO := $(DIR_ARDUINO)/arduino-builder
HARDWARE := -hardware $(DIR_ARDUINO)/hardware
TOOLS += -tools $(DIR_ARDUINO)/hardware/tools/avr
TOOLS += -tools $(DIR_ARDUINO)/tools-builder

FQBN := -fqbn arduino:avr:uno

CORE_VERSION := $(shell cat $(DIR_ROOT)/version)

VERSION := $(CORE_VERSION)

VERSION += +$(shell git rev-parse HEAD)

ifdef BUILD_NUMBER
VERSION += +$(BUILD_NUMBER)
endif

VERSION := $(subst $(SPACE),,$(VERSION))

EXAMPLES := $(patsubst src/%,build/arduino_ldl/%, $(wildcard src/examples/*))

LIBS := -libraries output

package: output/arduino_ldl

output/arduino_ldl:
	@ echo packaging $@:
	rm -fr $@
	mkdir -p $@
	cp -fr src/* $@
	cp $(DIR_ROOT)/src/* $@
	cp $(DIR_ROOT)/include/* $@
	cp $(DIR_ROOT)/LICENSE $@
	echo $(VERSION) > $@/version
	sed -i '/#define LORA_PLATFORM_H/a #define LORA_TARGET_INCLUDE "platform.h"' $@/lora_platform.h
	sed -i '/version=/s/.*/version=$(VERSION)/' $@/library.properties
	@ echo

build: $(EXAMPLES)

build/arduino_ldl/examples/environment_sensor: output/arduino_ldl
	@ echo building $@:
	rm -rf $@
	mkdir -p $@
	$(ARDUINO) $(TOOLS) $(FQBN) -build-path $(abspath $@) $(HARDWARE) $(LIBS) $</$(patsubst build/arduino_ldl/%,%, $@)
	@ echo

build/arduino_ldl/examples/debug_node: output/arduino_ldl
	@ echo building $@:
	rm -rf $@
	mkdir -p $@
	$(ARDUINO) $(TOOLS) $(FQBN) -build-path $(abspath $@) $(HARDWARE) $(LIBS) $</$(patsubst build/arduino_ldl/%,%, $@)
	@ echo

clean:
	rm -fr output/*
	rm -fr build/*

.PHONY: package build clean output/arduino_ldl
