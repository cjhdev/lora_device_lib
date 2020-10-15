DIR_ROOT := $(shell cd ../..;pwd)
DIR_BUILD := build
DIR_BIN := bin

CC := avr-gcc

VPATH += $(DIR_ROOT)/src
VPATH += .

INCLUDES += -I$(DIR_ROOT)/include
INCLUDES += -I.

LDL_SRC := $(notdir $(wildcard $(DIR_ROOT)/src/*.c))

SRC := $(LDL_SRC) main.c
OBJ := $(SRC:.c=.o)

MCU := atmega328p

PORT := /dev/ttyACM0

CFLAGS += -mmcu=$(MCU)
CFLAGS += -Os -Wall -std=gnu99 -g
CFLAGS += $(INCLUDES)

# 8bit types and packed alignment
CFLAGS += -funsigned-char
CFLAGS += -funsigned-bitfields
CFLAGS += -fpack-struct
CFLAGS += -fshort-enums

# trim unused
CFLAGS += -ffunction-sections
CFLAGS += -fdata-sections

# seems to increase flash
#CFLAGS += -ffreestanding

# i presume this may cause stack to grow more at expense of code
CFLAGS += -maccumulate-args

# i presume this will slow function calls
CFLAGS += -mcall-prologues

# optimise loops
CFLAGS += -fno-tree-scev-cprop

#CFLAGS += -DBAUD=115200U
CFLAGS += -DBAUD=38400UL

CFLAGS += -DLDL_TARGET_INCLUDE='"port.h"'

all: $(DIR_BIN)/avr.lst

$(DIR_BIN)/avr.elf: $(addprefix $(DIR_BUILD)/, $(OBJ))
	$(CC) $(CFLAGS) -Wl,-Map=$(basename $@).map,--cref,--relax  $^ -o $@


$(DIR_BIN)/avr.lst: $(DIR_BIN)/avr.elf
	avr-objdump -h -S $^ > $@

$(DIR_BIN)/avr.hex: $(DIR_BIN)/avr.elf
	avr-objcopy -j .text -j .data -O ihex $^ $@

$(DIR_BUILD)/%.o: %.c
	@ echo building $@
	@ $(CC) $(CFLAGS) -c $< -o $@

size: $(DIR_BIN)/avr.elf
	@ echo size of $^:
	@ avr-size --format=avr --mcu=$(MCU) $^

clean:
	@ echo cleaning up objects
	@ rm -f $(DIR_BUILD)/*

squeaky_clean: clean
	@ echo cleaning up images
	@ rm -f $(DIR_BIN)/*

.PHONY: size ldl-size clean squeaky_clean all $(DIR_BIN)/avr.elf
