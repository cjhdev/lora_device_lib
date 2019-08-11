DIR_ROOT := ../
SPACE :=
SPACE +=

CORE_VERSION := $(shell cat $(DIR_ROOT)/version)

VERSION := $(CORE_VERSION)

VERSION += +$(shell git rev-parse HEAD)

ifdef BUILD_NUMBER
VERSION += +$(BUILD_NUMBER)
endif

VERSION := $(subst $(SPACE),,$(VERSION))

all: clean
	LDL_VERSION=$(VERSION) doxygen
	rm html/index.html
	ln --symbolic group__ldl.html html/index.html
	rm -f html/*.js

clean: 
	rm -rf html/*

deploy: all
	rm -rf deploy
	mkdir deploy
	git clone git@github.com:cjhdev/lora_device_lib_api.git deploy
	rm -r deploy/docs/*
	cp -r html/* deploy/docs/
	git -C deploy add -A
	git -C deploy commit -m "update"
	git -C deploy push origin master

.PHONY: all clean deploy
