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

EXAMPLES := $(patsubst src/examples/%,staging/%, $(wildcard src/examples/*))
BUILD_EXAMPLES := 

# very awkward way to replace app_eui for quick testing
ifneq ("$(wildcard $(DIR_ROOT)/secrets/sketch/app_eui)","")
APP_EUI := $(shell cat $(DIR_ROOT)/secrets/sketch/app_eui)
else
APP_EUI := {0x00U,0x00U,0x00U,0x00U,0x00U,0x00U,0x00U,0x00U}
endif

# very awkward way to replace dev_eui for quick testing
ifneq ("$(wildcard $(DIR_ROOT)/secrets/sketch/dev_eui)","")
DEV_EUI := $(shell cat $(DIR_ROOT)/secrets/sketch/dev_eui)
else
DEV_EUI := {0x00U,0x00U,0x00U,0x00U,0x00U,0x00U,0x00U,0x00U}
endif

# very awkward way to replace app_key for quick testing
ifneq ("$(wildcard $(DIR_ROOT)/secrets/sketch/app_key)","")
APP_KEY := $(shell cat $(DIR_ROOT)/secrets/sketch/app_key)
else
APP_KEY := {0x2bU,0x7eU,0x15U,0x16U,0x28U,0xaeU,0xd2U,0xa6U,0xabU,0xf7U,0x15U,0x88U,0x09U,0xcfU,0x4fU,0x3cU}
endif

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
	sed -i '/#define LDL_PLATFORM_H/a #define LDL_TARGET_INCLUDE "platform.h"' $@/ldl_platform.h
	sed -i '/version=/s/.*/version=$(VERSION)/' $@/library.properties
	@ echo

staging/%: output/arduino_ldl
	rm -rf $@
	mkdir -p $@
	cp -r $(patsubst staging/%,$</examples/%, $@)/* $@
	mkdir -p $@/src
	cp -r $< $@/src
	rm -rf $@/src/arduino_ldl/examples
	sed -i "s/{0x00U,0x00U,0x00U,0x00U,0x00U,0x00U,0x00U,0x00U}/$(APP_EUI)/g" $@/$(notdir $@).ino
	sed -i "s/{0x00U,0x00U,0x00U,0x00U,0x00U,0x00U,0x00U,0x01U}/$(DEV_EUI)/g" $@/$(notdir $@).ino
	sed -i "s/{0x2bU,0x7eU,0x15U,0x16U,0x28U,0xaeU,0xd2U,0xa6U,0xabU,0xf7U,0x15U,0x88U,0x09U,0xcfU,0x4fU,0x3cU}/$(APP_KEY)/g" $@/$(notdir $@).ino
	sed -i "s/<arduino_ldl.h>/\"src\/arduino_ldl\/arduino_ldl.h\"/g" $@/$(notdir $@).ino
	- cp $@/platform.h $@/src/arduino_ldl/

stage: $(EXAMPLES)

build: $(patsubst staging/%,build/%, $(EXAMPLES))

build/%: staging/%
	@ echo building $@:
	rm -rf $@
	mkdir -p $@
	$(ARDUINO) $(TOOLS) $(FQBN) -build-path $(abspath $@) -warnings all $(HARDWARE) $</$(notdir $@).ino
	@ echo

clean:
	rm -fr output/*
	rm -fr build/*
	rm -fr staging/*

# turns out the arduino ide is so useless you have to use avrdude yourself if you want to upload a hex file
# 
# avrdude -v -V -p atmega328p -c arduino -P $/dev/ttyACM1 -b 115200 -D -U flash:w:$(THE_HEX_FILE):i
#
# sigh

.PHONY: package build clean output/arduino_ldl stage build
