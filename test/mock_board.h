#ifndef MOCK_BOARD_H
#define MOCK_BOARD_H

#include <stdint.h>
#include <stdbool.h>

#include "lora_board.h"

struct mock_chip {
    
    bool select;
    bool reset;     
};

struct user_data {
    
    struct mock_chip chip;
    struct lora_radio radio;    
    struct lora_board board;
};

void board_select(void *receiver, bool state);
void board_reset(void *receiver, bool state);
void board_write(void *receiver, uint8_t data);
uint8_t board_read(void *receiver);
    
#endif
