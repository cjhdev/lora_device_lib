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
TESTS += tc_timer
TESTS += tc_frame_with_encryption
TESTS += tc_only_sx1272
TESTS += tc_only_sx1276
TESTS += tc_only_sx1261
TESTS += tc_only_sx1262
TESTS += tc_only_wl55
TESTS += tc_only_us902
TESTS += tc_only_au915


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

check: CFLAGS += -DLDL_ENABLE_SX1272
check: CFLAGS += -DLDL_ENABLE_SX1276
check: CFLAGS += -DLDL_ENABLE_SX1261
check: CFLAGS += -DLDL_ENABLE_SX1262
check: CFLAGS += -DLDL_ENABLE_US_902_928
check: CFLAGS += -DLDL_ENABLE_AU_915_928
check: CFLAGS += -DLDL_ENABLE_EU_863_870
check: CC := clang
check: CFLAGS += --analyze -Xanalyzer -analyzer-output=text
check: $(addprefix $(DIR_BUILD)/, $(OBJ))

# AES sanity check
$(DIR_BIN)/tc_aes: $(addprefix $(DIR_BUILD)/, tc_aes.o ldl_aes.o $(OBJ_CMOCKA))
	@ echo linking $@
	@ $(CC) $(LDFLAGS) $^ -o $@

# AES CMAC sanity check
$(DIR_BIN)/tc_cmac: $(addprefix $(DIR_BUILD)/, tc_cmac.o ldl_cmac.o ldl_aes.o $(OBJ_CMOCKA))
	@ echo linking $@
	@ $(CC) $(LDFLAGS) $^ -o $@

# check frame codec
$(DIR_BIN)/tc_frame: $(addprefix $(DIR_BUILD)/, tc_frame.o ldl_frame.o ldl_stream.o $(OBJ_CMOCKA))
	@ echo linking $@
	@ $(CC) $(LDFLAGS) $^ -o $@

# frame encryption and authentication sanity check
$(DIR_BIN)/tc_frame_with_encryption: $(addprefix $(DIR_BUILD)/, ldl_frame.o ldl_stream.o ldl_sm.o ldl_aes.o ldl_cmac.o ldl_ctr.o ldl_ops.o tc_frame_with_encryption.o mock_ldl_system.o $(OBJ_CMOCKA))
	@ echo linking $@
	@ $(CC) $(LDFLAGS) $^ -o $@

# check mac_command codec
$(DIR_BIN)/tc_mac_commands: CFLAGS += -DLDL_ENABLE_CLASS_B
$(DIR_BIN)/tc_mac_commands: CFLAGS += -DLDL_L2_VERSION=LDL_L2_VERSION_1_1
$(DIR_BIN)/tc_mac_commands: $(addprefix $(DIR_BUILD)/, tc_mac_commands.o ldl_mac_commands.o ldl_stream.o $(OBJ_CMOCKA))
	@ echo linking $@
	@ $(CC) $(LDFLAGS) $^ -o $@

# check only sx1272
$(DIR_BIN)/tc_only_sx1272: CFLAGS += -DLDL_ENABLE_EU_863_870
$(DIR_BIN)/tc_only_sx1272: CFLAGS += -DLDL_ENABLE_SX1272
$(DIR_BIN)/tc_only_sx1272: $(addprefix $(DIR_BUILD)/, $(OBJ) tc_dummy.o mock_ldl_system.o $(OBJ_CMOCKA))
	@ echo linking $@
	@ $(CC) $(LDFLAGS) $^ -o $@

# check only sx1276
$(DIR_BIN)/tc_only_sx1276: CFLAGS += -DLDL_ENABLE_EU_863_870
$(DIR_BIN)/tc_only_sx1276: CFLAGS += -DLDL_ENABLE_SX1276
$(DIR_BIN)/tc_only_sx1276: $(addprefix $(DIR_BUILD)/, $(OBJ) tc_dummy.o mock_ldl_system.o $(OBJ_CMOCKA))
	@ echo linking $@
	@ $(CC) $(LDFLAGS) $^ -o $@

# check only sx1261
$(DIR_BIN)/tc_only_sx1261: CFLAGS += -DLDL_ENABLE_EU_863_870
$(DIR_BIN)/tc_only_sx1261: CFLAGS += -DLDL_ENABLE_SX1261
$(DIR_BIN)/tc_only_sx1261: $(addprefix $(DIR_BUILD)/, $(OBJ) tc_dummy.o mock_ldl_system.o $(OBJ_CMOCKA))
	@ echo linking $@
	@ $(CC) $(LDFLAGS) $^ -o $@

# check only sx1262
$(DIR_BIN)/tc_only_sx1262: CFLAGS += -DLDL_ENABLE_EU_863_870
$(DIR_BIN)/tc_only_sx1262: CFLAGS += -DLDL_ENABLE_SX1262
$(DIR_BIN)/tc_only_sx1262: $(addprefix $(DIR_BUILD)/, $(OBJ) tc_dummy.o mock_ldl_system.o $(OBJ_CMOCKA))
	@ echo linking $@
	@ $(CC) $(LDFLAGS) $^ -o $@

# check only wl55
$(DIR_BIN)/tc_only_wl55: CFLAGS += -DLDL_ENABLE_EU_863_870
$(DIR_BIN)/tc_only_wl55: CFLAGS += -DLDL_ENABLE_WL55
$(DIR_BIN)/tc_only_wl55: $(addprefix $(DIR_BUILD)/, $(OBJ) tc_dummy.o mock_ldl_system.o $(OBJ_CMOCKA))
	@ echo linking $@
	@ $(CC) $(LDFLAGS) $^ -o $@

# check only us
$(DIR_BIN)/tc_only_us902: CFLAGS += -DLDL_ENABLE_US_902_928
$(DIR_BIN)/tc_only_us902: CFLAGS += -DLDL_ENABLE_SX1261
$(DIR_BIN)/tc_only_us902: $(addprefix $(DIR_BUILD)/, $(OBJ) tc_dummy.o mock_ldl_system.o $(OBJ_CMOCKA))
	@ echo linking $@
	@ $(CC) $(LDFLAGS) $^ -o $@

# check only au
$(DIR_BIN)/tc_only_au915: CFLAGS += -DLDL_ENABLE_AU_915_928
$(DIR_BIN)/tc_only_au915: CFLAGS += -DLDL_ENABLE_SX1261
$(DIR_BIN)/tc_only_au915: $(addprefix $(DIR_BUILD)/, $(OBJ) tc_dummy.o mock_ldl_system.o $(OBJ_CMOCKA))
	@ echo linking $@
	@ $(CC) $(LDFLAGS) $^ -o $@

