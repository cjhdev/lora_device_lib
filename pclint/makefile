DIR_ROOT := ..
DIR_ENVIRONMENT := environment
DIR_SETTINGS := settings
DIR_REPORT := report

PCLINT_LIB := /usr/local/lib/pclint
PCLINT_BIN := /usr/local/bin/pclint

CC := gcc
GXX := g++

SRC := $(wildcard $(DIR_ROOT)/src/*.c)

DEFINES += -DLDL_ENABLE_US_902_928
DEFINES += -DLDL_ENABLE_AU_915_928
DEFINES += -DLDL_ENABLE_EU_863_870
DEFINES += -DLDL_ENABLE_EU_433

DEFINES += -DLDL_ENABLE_SX1272
DEFINES += -DLDL_ENABLE_SX1276


INCLUDES += -I$(DIR_SETTINGS)
INCLUDES += -I$(DIR_ENVIRONMENT)
INCLUDES += -I$(DIR_ROOT)/include

# keep going after errors
RULES += '-zero'

# use latest misra rules
RULES += au-misra3.lnt

# toolchain config
RULES += co-gcc.lnt

# n pass algorithm
RULES += '-passes(2)'

# no line breaks, 4 character indent
RULES += '-width(0,4)'

# suppress noise around use of stdbool
RULES += '-esym(970,_Bool)'

# errors are contained to files they appear in
RULES += '-restore_at_end'

# our headers are not library headers
RULES += '-libdir($(DIR_ROOT)/include)'

# unit checkout since this is a library
RULES += '-u'

# verbose
RULES += '-vfo'

.PHONY: install mandatory required advisory $(DIR_REPORT)/report.txt 

all: mandatory required advisory

# run co-gcc.mak to gather toolchain settings in the environment folder
install: 
	cd $(DIR_ENVIRONMENT) && make -f co-gcc.mak clean
	cd $(DIR_ENVIRONMENT) && make -f co-gcc.mak GCC_BIN=$(CC) GXX_BIN=$(GXX) CFLAGS=$(CFLAGS) CXXFLAGS=$(CXXFLAGS) CPPFLAGS=$(CPPFLAGS) COMMON_FLAGS=$(COMMON_FLAGS)

$(DIR_REPORT)/report.txt: $(SRC)
	$(PCLINT_BIN) '-os($@)' ++b $(DEFINES) $(INCLUDES) $(RULES) $^

mandatory: $(DIR_REPORT)/report.txt
	@ echo "$@: `cat $^ | grep -c $@` instances" 
	@ cat $^ | grep -s $@ || true

required: $(DIR_REPORT)/report.txt
	@ echo "$@: `cat $^ | grep -c $@` instances" 
	@ cat $^ | grep $@ || true

advisory: $(DIR_REPORT)/report.txt
	@ echo "$@: `cat $^ | grep -c $@` instances" 
	@ cat $^ | grep $@ || true

clean:
	rm -f $(DIR_REPORT)/report.txt
