DIR_ROOT := ../
SPACE :=
SPACE +=

SPHINXOPTS    ?=
SPHINXBUILD   ?= sphinx-build
SOURCEDIR     = .
BUILDDIR      = _build

VERSION := $(shell cat $(DIR_ROOT)/version)
UMLS := $(notdir $(wildcard *.uml))

all: clean
	LDL_VERSION=$(VERSION) doxygen
	rm -f html/*.js

uml: $(UMLS:.uml=.png)

%.png: %.uml
	plantuml $<

clean:
	rm -rf html/*
	rm -rf _build/*

deploy: all
	rm -rf deploy
	mkdir deploy
	git clone git@github.com:cjhdev/lora_device_lib_api.git deploy
	rm -r deploy/docs/*
	cp -r html/* deploy/docs/
	git -C deploy add -A
	git -C deploy commit -m "update"
	git -C deploy push origin master

help:
	$(SPHINXBUILD) -M help "$(SOURCEDIR)" "$(BUILDDIR)" $(SPHINXOPTS) $(O)

%: clean Makefile
	LDL_VERSION=$(VERSION) $(SPHINXBUILD) -M $@ "$(SOURCEDIR)" "$(BUILDDIR)" $(SPHINXOPTS) $(O)

.PHONY: all clean deploy help Makefile
