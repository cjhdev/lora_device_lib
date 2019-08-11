/* Copyright (c) 2019 Cameron Harper
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * */

#ifndef LORA_REGION_H
#define LORA_REGION_H

/** @file */

#ifdef __cplusplus
extern "C" {
#endif

#include "lora_platform.h"
#include "lora_radio_defs.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
    
/** LoRaWAN Region
 * 
 * @ingroup ldl_mac
 * 
 *  */
enum lora_region {
#ifdef LORA_ENABLE_EU_863_870
    EU_863_870,
#endif
#ifdef LORA_ENABLE_US_902_928
    US_902_928,
#endif
#ifdef LORA_ENABLE_AU_915_928
    AU_915_928,
#endif
#ifdef LORA_ENABLE_EU_433        
    EU_433,
#endif    
};

struct lora_mac;

void LDL_Region_convertRate(enum lora_region region, uint8_t rate, enum lora_spreading_factor *sf, enum lora_signal_bandwidth *bw, uint8_t *mtu);
void LDL_Region_getRX1DataRate(enum lora_region region, uint8_t tx_rate, uint8_t rx1_offset, uint8_t *rx1_rate);
void LDL_Region_getRX1Freq(enum lora_region region, uint32_t txFreq, uint8_t chIndex, uint32_t *freq);
bool LDL_Region_getTXPower(enum lora_region region, uint8_t power, int16_t *dbm);
bool LDL_Region_validateFreq(enum lora_region region, uint8_t chIndex, uint32_t freq);
bool LDL_Region_getBand(enum lora_region region, uint32_t freq, uint8_t *band);
bool LDL_Region_getChannel(enum lora_region region, uint8_t chIndex, uint32_t *freq, uint8_t *minRate, uint8_t *maxRate);
bool LDL_Region_validateRate(enum lora_region region, uint8_t chIndex, uint8_t minRate, uint8_t maxRate);
bool LDL_Region_isDynamic(enum lora_region region);
uint8_t LDL_Region_numChannels(enum lora_region region);
uint8_t LDL_Region_getJA1Delay(enum lora_region region);
uint16_t LDL_Region_getOffTimeFactor(enum lora_region region, uint8_t band);
uint16_t LDL_Region_getMaxFCNTGap(enum lora_region region);
uint8_t LDL_Region_getRX1Delay(enum lora_region region);
uint8_t LDL_Region_getRX1Offset(enum lora_region region);
uint32_t LDL_Region_getRX2Freq(enum lora_region region);
uint8_t LDL_Region_getRX2Rate(enum lora_region region);
uint8_t LDL_Region_getJoinRate(enum lora_region region, uint16_t trial);
void LDL_Region_getDefaultChannels(enum lora_region region, struct lora_mac *receiver, void (*handler)(struct lora_mac *reciever, uint8_t chIndex, uint32_t freq, uint8_t minRate, uint8_t maxRate));

#ifdef __cplusplus
}
#endif

#endif
