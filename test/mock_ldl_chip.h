#ifndef MOCK_LORA_CHIP_H
#define MOCK_LORA_CHIP_H

#include <stdint.h>
#include <stdbool.h>

struct mock_lora_chip {
    
    bool select;
    bool reset;     
};

struct user_data {
    
    struct mock_lora_chip chip;
    struct ldl_radio radio;    
};
    
#endif
