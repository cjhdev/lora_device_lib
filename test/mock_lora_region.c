#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "cmocka.h"
#include "lora_region.h"

bool LDL_Region_getRate(const struct lora_region *self, uint8_t rate, struct lora_data_rate *setting)
{
    (void)memcpy(setting, mock_ptr_type(struct lora_data_rate *), sizeof(struct lora_data_rate));
    return mock_type(bool);
}

void LDL_Region_getDefaultChannels(const struct lora_region *self, void *receiver, void (*handler)(void *reciever, uint8_t chIndex, uint32_t freq, uint8_t minRate, uint8_t maxRate))
{
}

bool LDL_Region_validateFrequency(const struct lora_region *self, uint32_t frequency, uint8_t *band)
{
    *band = mock_type(uint8_t);
    return mock_type(bool);    
}

void LDL_Region_getDefaultSettings(const struct lora_region *self, struct lora_region_default *defaults)
{
    memset(defaults, 0, sizeof(*defaults));
}

uint8_t LDL_Region_numChannels(const struct lora_region *self)
{
    return mock_type(uint8_t);
}

uint8_t LDL_Region_getMACPayload(const struct lora_region *self, uint8_t rate)
{
    return mock_type(uint8_t);
}

uint8_t LDL_Region_getJA1Delay(const struct lora_region *self)
{
    return mock_type(uint8_t);
}

uint16_t LDL_Region_getOffTimeFactor(const struct lora_region *self, uint8_t band)
{
    return mock_type(uint16_t);
}

bool LDL_Region_validateRate(const struct lora_region *self, uint8_t chIndex, uint8_t minRate, uint8_t maxRate)
{
    return mock_type(bool);
}

bool LDL_Region_validateFreq(const struct lora_region *self, uint8_t chIndex, uint32_t freq)
{
    return mock_type(bool);
}

bool LDL_Region_getRX1DataRate(const struct lora_region *self, uint8_t tx_rate, uint8_t rx1_offset, uint8_t *rx1_rate)
{
    *rx1_rate = mock_type(uint8_t);
    return mock_type(bool);
}

bool LDL_Region_getRX1Freq(const struct lora_region *self, uint32_t txFreq, uint32_t *freq)
{
    *freq = mock_type(uint32_t);
    return mock_type(bool);
}

uint16_t LDL_Region_getMaxFCNTGap(const struct lora_region *self)
{
    return mock_type(uint16_t);
}

const struct lora_region *LDL_Region_getRegion(enum lora_region_id region)
{
    return mock_ptr_type(const struct lora_region *);
}

bool LDL_Region_isDynamic(const struct lora_region *self)
{
    return mock_type(bool);
}

bool LDL_Region_getBand(const struct lora_region *self, uint32_t freq, uint8_t *band)
{
    *band = mock_type(uint8_t);
    return mock_type(bool);
}

bool LDL_Region_getChannel(const struct lora_region *self, uint8_t chIndex, uint32_t *freq, uint8_t *minRate, uint8_t *maxRate)
{
    *freq = mock_type(uint32_t);
    *minRate = mock_type(uint8_t);
    *maxRate = mock_type(uint8_t);
    return mock_type(bool);
}
