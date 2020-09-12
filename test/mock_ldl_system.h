#ifndef MOCK_LORA_SYSTEM_H
#define MOCK_LORA_SYSTEM_H

#include "ldl_system.h"
#include "ldl_mac.h"

struct mock_system_param {

    uint8_t battery_level;

    uint16_t upCounter;
    uint16_t downCounter;
};

void mock_lora_system_init(struct mock_system_param *self);

uint32_t LDL_System_ticks(void *app);

#endif
