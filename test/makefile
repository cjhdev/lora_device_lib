DIR_ROOT := ..
DIR_CMOCKA := $(DIR_ROOT)/vendor/cmocka
DIR_BUILD := build
DIR_BIN := bin

CC := gcc

VPATH += $(DIR_ROOT)/src
VPATH += $(DIR_CMOCKA)/src

INCLUDES += -I$(DIR_ROOT)/include
INCLUDES += -I$(DIR_CMOCKA)/include
INCLUDES += -I.

CMOCKA_DEFINES += -DHAVE_STRINGS_H
CMOCKA_DEFINES += -DHAVE_SIGNAL_H
CMOCKA_DEFINES += -DHAVE_INTTYPES_H
CMOCKA_DEFINES += -DHAVE_MALLOC_H

DEBUG_DEFINES += -D'LDL_TARGET_INCLUDE="debug_include.h"'
DEBUG_DEFINES += -DLDL_ENABLE_RADIO_TEST

DEBUG_DEFINES += -DLDL_ENABLE_US_902_928
DEBUG_DEFINES += -DLDL_ENABLE_AU_915_928
DEBUG_DEFINES += -DLDL_ENABLE_EU_863_870
DEBUG_DEFINES += -DLDL_ENABLE_EU_433

CFLAGS := -O0 -Wall -ggdb -fprofile-arcs -ftest-coverage $(INCLUDES) $(CMOCKA_DEFINES) $(DEBUG_DEFINES)
LDFLAGS := -fprofile-arcs -ggdb

SRC := $(notdir $(wildcard $(DIR_ROOT)/src/*.c))
SRC_CMOCKA := $(notdir $(wildcard $(DIR_CMOCKA)/src/*.c))

OBJ := $(SRC:.c=.o)
OBJ_CMOCKA := $(SRC_CMOCKA:.c=.o)

TESTS += tc_aes 
TESTS += tc_cmac
TESTS += tc_frame
TESTS += tc_mac_commands
TESTS += tc_input
TESTS += tc_timer
TESTS += tc_frame_with_encryption


LINE := ================================================================

.PHONY: clean all coverage line

all: $(addprefix $(DIR_BIN)/, $(TESTS))

line:
	@ echo $(LINE)

build_and_run: 
	@ FAIL=0; \
	for tc in $(addprefix $(DIR_BIN)/, $(TESTS)); do \
		echo $(LINE); \
		echo ""; \
		echo "$$tc:"; \
		echo ""; \
		make clean $$tc \
		&& echo "" \
		&& if ! ./$$tc; then FAIL=1; fi; \
		echo ""; \
	done; \
	exit $$FAIL

$(DIR_BUILD)/%.o: %.c
	@ echo building $@
	@ $(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(DIR_BUILD)/*

sqeaky_clean: clean
	rm -f $(DIR_BIN)/*

mccabe: 
	pmccabe -vt $(addprefix $(DIR_ROOT)/src/, $(SRC))

check: CC := clang
check: CFLAGS += --analyze -Xanalyzer -analyzer-output=text
check: $(addprefix $(DIR_BUILD)/, $(OBJ))

$(DIR_BIN)/tc_aes: $(addprefix $(DIR_BUILD)/, tc_aes.o ldl_aes.o $(OBJ_CMOCKA))
	@ echo linking $@
	@ $(CC) $(LDFLAGS) $^ -o $@

$(DIR_BIN)/tc_cmac: $(addprefix $(DIR_BUILD)/, tc_cmac.o ldl_cmac.o ldl_aes.o $(OBJ_CMOCKA))
	@ echo linking $@
	@ $(CC) $(LDFLAGS) $^ -o $@

$(DIR_BIN)/tc_frame: $(addprefix $(DIR_BUILD)/, tc_frame.o ldl_frame.o ldl_stream.o $(OBJ_CMOCKA))
	@ echo linking $@
	@ $(CC) $(LDFLAGS) $^ -o $@

$(DIR_BIN)/tc_frame_with_encryption: CFLAGS += -DLDL_ENABLE_SX1272
$(DIR_BIN)/tc_frame_with_encryption: CFLAGS += -DLDL_ENABLE_SX1276
$(DIR_BIN)/tc_frame_with_encryption: $(addprefix $(DIR_BUILD)/, $(OBJ) tc_frame_with_encryption.o mock_ldl_system.o mock_ldl_chip.o $(OBJ_CMOCKA))
	@ echo linking $@
	@ $(CC) $(LDFLAGS) $^ -o $@

$(DIR_BIN)/tc_input: CFLAGS += -DLDL_ENABLE_SX1272
$(DIR_BIN)/tc_input: CFLAGS += -DLDL_ENABLE_SX1276
$(DIR_BIN)/tc_input: $(addprefix $(DIR_BUILD)/, $(OBJ) tc_input.o mock_ldl_system.o mock_ldl_chip.o $(OBJ_CMOCKA))
	@ echo linking $@
	@ $(CC) $(LDFLAGS) $^ -o $@

$(DIR_BIN)/tc_timer: CFLAGS += -DLDL_ENABLE_SX1272
$(DIR_BIN)/tc_timer: CFLAGS += -DLDL_ENABLE_SX1276
$(DIR_BIN)/tc_timer: $(addprefix $(DIR_BUILD)/, $(OBJ) tc_timer.o mock_ldl_system.o mock_ldl_chip.o $(OBJ_CMOCKA))
	@ echo linking $@
	@ $(CC) $(LDFLAGS) $^ -o $@

$(DIR_BIN)/tc_mac_commands: $(addprefix $(DIR_BUILD)/, tc_mac_commands.o ldl_mac_commands.o ldl_stream.o $(OBJ_CMOCKA))
	@ echo linking $@
	@ $(CC) $(LDFLAGS) $^ -o $@
