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

DEBUG_DEFINES += -D'LORA_TARGET_INCLUDE="debug_include.h"'
DEBUG_DEFINES += -DLORA_ENABLE_RADIO_TEST

DEBUG_DEFINES += -DLORA_ENABLE_US_902_928
DEBUG_DEFINES += -DLORA_ENABLE_AU_915_928
DEBUG_DEFINES += -DLORA_ENABLE_EU_863_870
DEBUG_DEFINES += -DLORA_ENABLE_EU_433

CFLAGS := -O0 -Wall -g -fprofile-arcs -ftest-coverage $(INCLUDES) $(CMOCKA_DEFINES) $(DEBUG_DEFINES)
LDFLAGS := -fprofile-arcs -g

SRC := $(notdir $(wildcard $(DIR_ROOT)/src/*.c))
SRC_CMOCKA := $(notdir $(wildcard $(DIR_CMOCKA)/src/*.c))

OBJ := $(SRC:.c=.o)
OBJ_CMOCKA := $(SRC_CMOCKA:.c=.o)

TESTS := $(basename $(wildcard tc_*.c))

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

check: CC := clang
check: CFLAGS += --analyze -Xanalyzer -analyzer-output=text
check: $(addprefix $(DIR_BUILD)/, $(OBJ))

$(DIR_BIN)/tc_aes: $(addprefix $(DIR_BUILD)/, tc_aes.o lora_aes.o $(OBJ_CMOCKA))
	@ echo linking $@
	@ $(CC) $(LDFLAGS) $^ -o $@

$(DIR_BIN)/tc_cmac: $(addprefix $(DIR_BUILD)/, tc_cmac.o lora_cmac.o lora_aes.o $(OBJ_CMOCKA))
	@ echo linking $@
	@ $(CC) $(LDFLAGS) $^ -o $@

$(DIR_BIN)/tc_frame: $(addprefix $(DIR_BUILD)/, tc_frame.o lora_frame.o mock_lora_cmac.o mock_lora_aes.o $(OBJ_CMOCKA))
	@ echo linking $@
	@ $(CC) $(LDFLAGS) $^ -o $@

$(DIR_BIN)/tc_frame_with_encryption: $(addprefix $(DIR_BUILD)/, tc_frame_with_encryption.o lora_frame.o lora_cmac.o lora_aes.o $(OBJ_CMOCKA))
	@ echo linking $@
	@ $(CC) $(LDFLAGS) $^ -o $@

$(DIR_BIN)/tc_event: $(addprefix $(DIR_BUILD)/, tc_event.o lora_event.o mock_system_time.o mock_lora_system.o $(OBJ_CMOCKA))
	@ echo linking $@
	@ $(CC) $(LDFLAGS) $^ -o $@

$(DIR_BIN)/tc_mac_commands: $(addprefix $(DIR_BUILD)/, tc_mac_commands.o lora_mac_commands.o lora_stream.o $(OBJ_CMOCKA))
	@ echo linking $@
	@ $(CC) $(LDFLAGS) $^ -o $@

$(DIR_BIN)/tc_region: $(addprefix $(DIR_BUILD)/, tc_region.o lora_region.o $(OBJ_CMOCKA))
	@ echo linking $@
	@ $(CC) $(LDFLAGS) $^ -o $@

$(DIR_BIN)/tc_integration_sx1272: CFLAGS += -DLORA_ENABLE_SX1272
$(DIR_BIN)/tc_integration_sx1272: CFLAGS += -DLORA_ENABLE_SX1276
$(DIR_BIN)/tc_integration_sx1272: $(addprefix $(DIR_BUILD)/, common_integration.o mock_lora_system.o mock_system_time.o $(OBJ) $(OBJ_CMOCKA))
	@ echo linking $@
	@ $(CC) $(LDFLAGS) $^ -o $@

$(DIR_BIN)/tc_integration_sx1276: CFLAGS += -DLORA_ENABLE_SX1272
$(DIR_BIN)/tc_integration_sx1276: CFLAGS += -DLORA_ENABLE_SX1276
$(DIR_BIN)/tc_integration_sx1276: $(addprefix $(DIR_BUILD)/, common_integration.o mock_lora_system.o mock_system_time.o $(OBJ) $(OBJ_CMOCKA))
	@ echo linking $@
	@ $(CC) $(LDFLAGS) $^ -o $@
