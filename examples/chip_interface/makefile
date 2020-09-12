DIR_ROOT := ../..

DIR_BUILD := build
DIR_BIN := bin

SRC := read_example.c write_example.c set_mode_example.c
OBJ := $(SRC:.c=.o)

VPATH += .
VPATH += $(DIR_ROOT)/src

INCLUDES += -I.
INCLUDES += -I$(DIR_ROOT)/include

CFLAGS += $(INCLUDES)

CLFAGS += -Wall

example: $(addprefix $(DIR_BUILD)/, $(OBJ))
#	$(CC) $(CFLAGS) $^ -o $@

$(DIR_BUILD)/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	@ echo cleaning up objects
	@ rm -f $(DIR_BUILD)/*

.PHONY: example
