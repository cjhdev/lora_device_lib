DIR_ROOT := ../..

DIR_BUILD := build
DIR_BIN := bin

SRC := $(notdir $(wildcard $(DIR_ROOT)/src/*.c)) example.c
OBJ := $(SRC:.c=.o)

VPATH += .
VPATH += $(DIR_ROOT)/src

INCLUDES += -I.
INCLUDES += -I$(DIR_ROOT)/include

CFLAGS += $(INCLUDES)

CFLAGS += -DLDL_ENABLE_SX1272
CFLAGS += -DLDL_ENABLE_SX1276
CFLAGS += -DLDL_ENABLE_EU_863_870

CLFAGS += -Wall

example: $(addprefix $(DIR_BUILD)/, $(OBJ))
#	$(CC) $(CFLAGS) $^ -o $@

$(DIR_BUILD)/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	@ echo cleaning up objects
	@ rm -f $(DIR_BUILD)/*

.PHONY: example
