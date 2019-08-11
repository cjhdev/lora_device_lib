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

#include "lora_region.h"
#include "lora_debug.h"
#include "lora_mac.h"

#ifdef LORA_ENABLE_AVR

    #include <avr/pgmspace.h>
    
#else

    #include <string.h>
    
    #define PROGMEM
    #define memcpy_P memcpy
    
#endif
    
/* static function prototypes *****************************************/

static bool upRateRange(enum lora_region region, uint8_t chIndex, uint8_t *minRate, uint8_t *maxRate);

/* functions **********************************************************/

void LDL_Region_convertRate(enum lora_region region, uint8_t rate, enum lora_spreading_factor *sf, enum lora_signal_bandwidth *bw, uint8_t *mtu)
{
    LORA_PEDANTIC(sf != NULL)
    LORA_PEDANTIC(bw != NULL)
    LORA_PEDANTIC(mtu != NULL)
    
    switch(region){ 
#if defined(LORA_ENABLE_EU_863_870) || defined(LORA_ENABLE_EU_433)                              
#   ifdef LORA_ENABLE_EU_863_870    
    case EU_863_870:
#   endif    
#   ifdef LORA_ENABLE_EU_433    
    case EU_433:
#   endif        
        switch(rate){
        case 0U:
            *sf = SF_12;
            *bw = BW_125;
            *mtu = 59U;
            break;
        case 1U:
            *sf = SF_11;
            *bw = BW_125;
            *mtu = 59U;
            break;
        case 2U:
            *sf = SF_10;
            *bw = BW_125;
            *mtu = 59U;
            break;
        case 3U:
            *sf = SF_9;
            *bw = BW_125;
            *mtu = 123U;
            break;
        case 4U:
            *sf = SF_8;
            *bw = BW_125;
            *mtu = 250U;
            break;
        case 5U:        
            *sf = SF_7;
            *bw = BW_125;
            *mtu = 250U;
            break;
        case 6U:
            *sf = SF_7;
            *bw = BW_250;
            *mtu = 250U;
            break;                        
        default:
            *sf = SF_7;
            *bw = BW_125;
            *mtu = 250U;
            LORA_ERROR("invalid rate")
            break;
        }
        break;
#endif        
#ifdef LORA_ENABLE_US_902_928
    case US_902_928:
    
        switch(rate){
        case 0U:
            *sf = SF_10;
            *bw = BW_125;
            *mtu = 19U;
            break;
        case 1U:
            *sf = SF_9;
            *bw = BW_125;
            *mtu = 61U;
            break;
        case 2U:
            *sf = SF_8;
            *bw = BW_125;
            *mtu = 133U;
            break;
        case 3U:
            *sf = SF_7;
            *bw = BW_125;
            *mtu = 250U;
            break;
        case 4U:
        case 12U:
            *sf = SF_8;
            *bw = BW_500;
            *mtu = 250U;
            break;
        case 8U:
            *sf = SF_12;
            *bw = BW_500;
            *mtu = 61U;
            break;
        case 9U:
            *sf = SF_11;
            *bw = BW_500;
            *mtu = 137U;
            break;
        case 10U:
            *sf = SF_10;
            *bw = BW_500;
            *mtu = 250U;
            break;
        case 11U:
            *sf = SF_9;
            *bw = BW_500;
            *mtu = 250U;
            break;        
        case 13U:
            *sf = SF_7;
            *bw = BW_500;
            *mtu = 250U;
            break;
        default:
            *sf = SF_7;
            *bw = BW_125;
            *mtu = 250U;
            LORA_ERROR("invalid rate")
            break;
        }    
        break;
#endif                
#ifdef LORA_ENABLE_AU_915_928
    case AU_915_928:
    
        switch(rate){
        case 0U:
            *sf = SF_12;
            *bw = BW_125;
            *mtu = 59U;
            break;
        case 1U:
            *sf = SF_11;
            *bw = BW_125;
            *mtu = 59U;
            break;
        case 2U:
            *sf = SF_10;
            *bw = BW_125;
            *mtu = 59U;
            break;
        case 3U:
            *sf = SF_9;
            *bw = BW_125;
            *mtu = 123U;
            break;
        case 4U:
            *sf = SF_8;
            *bw = BW_125;
            *mtu = 250U;
            break;
        case 5U:
            *sf = SF_7;
            *bw = BW_125;
            *mtu = 250U;
            break;
        case 6U:
        case 12U:
            *sf = SF_8;
            *bw = BW_500;
            *mtu = 250U;
            break;
        case 8U:
            *sf = SF_12;
            *bw = BW_500;
            *mtu = 61U;
            break;
        case 9U:
            *sf = SF_11;
            *bw = BW_500;
            *mtu = 137U;
            break;
        case 10U:
            *sf = SF_10;
            *bw = BW_500;
            *mtu = 250U;
            break;
        case 11U:
            *sf = SF_9;
            *bw = BW_500;
            *mtu = 250U;
            break;        
        case 13U:
            *sf = SF_7;
            *bw = BW_500;
            *mtu = 250U;
            break;        
        default:
            *sf = SF_7;
            *bw = BW_125;
            *mtu = 250U;
            LORA_ERROR("invalid rate")
            break;
        }    
        break;
#endif                
    default:
        break;
    }
}

bool LDL_Region_getBand(enum lora_region region, uint32_t freq, uint8_t *band)
{
    LORA_PEDANTIC(band != NULL)

    bool retval = false;

    switch(region){
#ifdef LORA_ENABLE_EU_863_870        
    case EU_863_870:
    
        retval = true;
    
        if((freq >= 863000000UL) && (freq <= 868000000UL)){
            
            *band = 0U;
        }
        else if((freq >= 868000000UL) && (freq <= 868600000UL)){
            
            *band = 1U;
        }
        else if((freq >= 868700000UL) && (freq <= 869200000UL)){
            
            *band = 2U;
        }
        else if((freq >= 869400000UL) && (freq <= 869650000UL)){
            
            *band = 3U;
        }
        else if((freq >= 869700000UL) && (freq < 870000000UL)){
            
            *band = 4U;
        }
        else{
            
            retval = false;
        }        
        break;
#endif    
    default:
        *band = 0U;
        retval = true;
        break;        
    }
    
    return retval;
}

bool LDL_Region_isDynamic(enum lora_region region)
{
    bool retval;
    
    switch(region){
    default:
        retval = true;
        break;        
#ifdef LORA_ENABLE_US_902_928
    case US_902_928:         
        retval = false;
        break;
#endif
#ifdef LORA_ENABLE_AU_915_928
    case AU_915_928:
        retval = false;                    
        break;
#endif        
    }
    
    return retval;
}

bool LDL_Region_getChannel(enum lora_region region, uint8_t chIndex, uint32_t *freq, uint8_t *minRate, uint8_t *maxRate)
{
    bool retval = false;
    
    switch(region){
    default:
        break;    
#ifdef LORA_ENABLE_US_902_928
    case US_902_928:         

        retval = true;
        
        if(chIndex < 64U){

            *freq = 902300000UL + ( 200000UL * chIndex);
            *minRate = 0U;
            *maxRate = 3U;
        }
        else if(chIndex < 72U){
            
            *freq = 903000000UL + ( 200000UL * (chIndex - 64U));
            *minRate = 4U;
            *maxRate = 4U;
        }
        else{
            
            retval = false;
        }
        break;
#endif
#ifdef LORA_ENABLE_AU_915_928
    case AU_915_928:
    
        retval = true;
        
        if(chIndex < 64U){

            *freq = 915200000UL + ( 200000UL * chIndex);
            *minRate = 0U;
            *maxRate = 5U;
        }
        else if(chIndex < 72U){
            
            *freq = 915900000UL + ( 200000UL * (chIndex - 64U));
            *minRate = 6U;
            *maxRate = 6U;
        }
        else{
            
            retval = false;
        }
        break;
#endif        
    }
    
    return retval;
}

uint8_t LDL_Region_numChannels(enum lora_region region)
{
    uint8_t retval;
    
    switch(region){
    default:
        retval = 16U;    
        break;
#if defined(LORA_ENABLE_US_902_928)  || defined(LORA_ENABLE_AU_915_928)              
#   ifdef LORA_ENABLE_US_902_928
    case US_902_928:
#   endif                 
#   ifdef LORA_ENABLE_AU_915_928
    case AU_915_928:
#   endif     
           
        retval = 72U;
        break;                
#endif        
    }
    
    return retval;    
}

void LDL_Region_getDefaultChannels(enum lora_region region, struct lora_mac *receiver, void (*handler)(struct lora_mac *reciever, uint8_t chIndex, uint32_t freq, uint8_t minRate, uint8_t maxRate))
{
    LORA_PEDANTIC(handler != NULL)
    
    uint8_t minRate;
    uint8_t maxRate;
    
    switch(region){
    default:
        break;    
#ifdef LORA_ENABLE_EU_863_870
    case EU_863_870:
    
        (void)upRateRange(region, 0U, &minRate, &maxRate);
        
        handler(receiver, 0U, 868100000UL, minRate, maxRate);
        handler(receiver, 1U, 868300000UL, minRate, maxRate);
        handler(receiver, 2U, 868500000UL, minRate, maxRate);        
        break;
#endif        
#ifdef LORA_ENABLE_EU_433    
    case EU_433:
    
        (void)upRateRange(region, 0U, &minRate, &maxRate);
        
        handler(receiver, 0U, 433175000UL, minRate, maxRate);
        handler(receiver, 1U, 433375000UL, minRate, maxRate);
        handler(receiver, 2U, 433575000UL, minRate, maxRate);        
        break;
#endif        
    }
}

uint16_t LDL_Region_getOffTimeFactor(enum lora_region region, uint8_t band)
{
    uint16_t retval = 0U;
    
    switch(region){
    default:
        break;    
#ifdef LORA_ENABLE_EU_863_870    
    case EU_863_870:
    
        switch(band){
        case 0U:
        case 1U:
        case 4U:
            retval = 100U;      // 1.0%
            break;
        case 2U:
            retval = 1000U;     // 0.1%
            break;
        case 3U:        
            retval = 10U;       // 10.0%
            break;                    
        default:
            break;
        }
        break;
#endif        
#ifdef LORA_ENABLE_EU_433    
    case EU_433:        
        retval = 100U;
        break;    
#endif
    }
    
    return retval;    
}

bool LDL_Region_validateRate(enum lora_region region, uint8_t chIndex, uint8_t minRate, uint8_t maxRate)
{
    bool retval = false;
    uint8_t min;
    uint8_t max;

    if(upRateRange(region, chIndex, &min, &max)){
        
        if((minRate >= min) && (maxRate <= max)){
            
            retval = true;
        }
    }    
    
    return retval;
}

bool LDL_Region_validateFreq(enum lora_region region, uint8_t chIndex, uint32_t freq)
{
    return true;
}

void LDL_Region_getRX1DataRate(enum lora_region region, uint8_t tx_rate, uint8_t rx1_offset, uint8_t *rx1_rate)
{
    LORA_PEDANTIC(rx1_rate != NULL)

    const uint8_t *ptr = NULL;
    uint16_t index = 0U;
    size_t size = 0U;
    
    switch(region){
    default:
#if defined(LORA_ENABLE_EU_863_870) || defined(LORA_ENABLE_EU_433)        
#   ifdef LORA_ENABLE_EU_863_870
    case EU_863_870:
#   endif
#   ifdef LORA_ENABLE_EU_433
    case EU_433:
#   endif    
    {
        static const uint8_t rates[] PROGMEM = {
            0U, 0U, 0U, 0U, 0U, 0U,
            1U, 0U, 0U, 0U, 0U, 0U,
            2U, 1U, 0U, 0U, 0U, 0U,
            3U, 2U, 1U, 0U, 0U, 0U,
            4U, 3U, 2U, 1U, 0U, 0U,
            5U, 4U, 3U, 2U, 1U, 0U,
            6U, 5U, 4U, 3U, 2U, 1U,
            7U, 6U, 5U, 4U, 3U, 2U,
        };

        index = (tx_rate * 6U) + rx1_offset;
        ptr = rates;
        size = sizeof(rates);
    }
        break;                   
#endif        
#ifdef LORA_ENABLE_US_902_928
    case US_902_928:        
    {
        static const uint8_t rates[] PROGMEM = {
            10U, 9U,  8U,  8U,
            11U, 10U, 9U,  8U,
            12U, 11U, 10U, 9U,
            13U, 12U, 11U, 10U,
            13U, 13U, 12U, 11U,
        };
        
        index = (tx_rate * 4U) + rx1_offset;
        ptr = rates;
        size = sizeof(rates);
    }
        break;
#endif        
#ifdef LORA_ENABLE_AU_915_928
    case AU_915_928:        
    {
        static const uint8_t rates[] PROGMEM = {
            8U,  8U,  8U,  8U,  8U,  8U,
            9U,  8U,  8U,  8U,  8U,  8U,
            10U, 9U,  8U,  8U,  8U,  8U,
            11U, 10U, 9U,  8U,  8U,  8U,
            12U, 11U, 10U, 9U,  8U,  8U,
            13U, 12U, 11U, 10U, 9U,  8U,
            13U, 13U, 12U, 11U, 10U, 9U,
        };
        
        index = (tx_rate * 6U) + rx1_offset;
        ptr = rates;
        size = sizeof(rates);
    }
        break;
#endif        
    }
    
    if(ptr != NULL){
    
        if(index < size){
        
            (void)memcpy_P(rx1_rate, &ptr[index], sizeof(*rx1_rate));
        }
        else{
                        
            *rx1_rate = tx_rate;
            LORA_ERROR("out of range error")
        }
    }        
}

void LDL_Region_getRX1Freq(enum lora_region region, uint32_t txFreq, uint8_t chIndex, uint32_t *freq)
{
    switch(region){
    default:
        *freq = txFreq;
        break;        
#if defined(LORA_ENABLE_US_902_928)  || defined(LORA_ENABLE_AU_915_928)              
#   ifdef LORA_ENABLE_US_902_928
    case US_902_928:
#   endif                 
#   ifdef LORA_ENABLE_AU_915_928
    case AU_915_928:
#   endif     
               
        *freq = 923300000UL + ((uint32_t)(chIndex % 8U) * 600000UL);       
        break;    
#endif        
    }
}

uint16_t LDL_Region_getMaxFCNTGap(enum lora_region region)
{
    return 16384U;        
}

uint8_t LDL_Region_getRX1Delay(enum lora_region region)
{
    return 1U;    
}

uint8_t LDL_Region_getJA1Delay(enum lora_region region)
{
    return 5U;        
}

uint8_t LDL_Region_getRX1Offset(enum lora_region region)
{
    return 0U;        
}

uint32_t LDL_Region_getRX2Freq(enum lora_region region)
{
    uint32_t retval;
    
    switch(region){
    default:
#ifdef LORA_ENABLE_EU_863_870    
    case EU_863_870:
        retval = 869525000UL;
        break;    
#endif
#ifdef LORA_ENABLE_EU_433        
    case EU_433:
        retval = 434665000UL;
        break;    
#endif
#ifdef LORA_ENABLE_US_902_928
    case US_902_928:         
        retval = 923300000UL;
        break;        
#endif
#ifdef LORA_ENABLE_AU_915_928
    case AU_915_928:        
        retval = 923300000UL;
        break;        
#endif        
    }
    
    return retval; 
}

uint8_t LDL_Region_getRX2Rate(enum lora_region region)
{
    uint8_t retval;
    
    switch(region){
    default:
#ifdef LORA_ENABLE_EU_863_870
    case EU_863_870:
        retval = 0U;
        break;
#endif
#ifdef LORA_ENABLE_EU_433
    case EU_433:
        retval = 0U;
        break;
#endif        
#ifdef LORA_ENABLE_US_902_928
    case US_902_928:     
        retval = 8U;
        break;
#endif
#ifdef LORA_ENABLE_AU_915_928
    case AU_915_928:
        retval = 8U;
        break;
#endif                
    }
    
    return retval; 
}

bool LDL_Region_getTXPower(enum lora_region region, uint8_t power, int16_t *dbm)
{
    bool retval = false;
    
    switch(region){ 
#ifdef LORA_ENABLE_EU_863_870               
    case EU_863_870:
        
        if(power <= 7U){
            
            *dbm = 1600 - (power * 200);
            retval = true;
        }
        else{
            
            *dbm = 1600 - (7U * 200);
        }
        break;
#endif
#ifdef LORA_ENABLE_EU_433        
    case EU_433:
    
        if(power <= 5U){
            
            *dbm = 1215 - (power * 200);
            retval = true;
        }
        else{
            
            *dbm = 1215 - (5U * 200);
        }
        break;
#endif            
#if defined(LORA_ENABLE_US_902_928)  || defined(LORA_ENABLE_AU_915_928)              
#   ifdef LORA_ENABLE_US_902_928
    case US_902_928:
#   endif                 
#   ifdef LORA_ENABLE_AU_915_928
    case AU_915_928:
#   endif     
        
        if(power <= 10U){
            
            *dbm = 3000 - (power * 200);
            retval = true;
        }
        else{
            
            *dbm = 3000 - (10U * 200);
        }
        break;
#endif    
    default:
        break;      
    }
    
    return retval; 
}

uint8_t LDL_Region_getJoinRate(enum lora_region region, uint16_t trial)
{
    uint8_t retval = 0U;
    
    switch(region){    
#if defined(LORA_ENABLE_EU_863_870) || defined(LORA_ENABLE_EU_433)                      
#   ifdef LORA_ENABLE_EU_863_870    
    case EU_863_870:
#   endif    
#   ifdef LORA_ENABLE_EU_433    
    case EU_433:
#   endif       
        retval = 5U - (trial % (6U - LORA_DEFAULT_RATE));
        break;
#endif        
#if defined(LORA_ENABLE_US_902_928)        
#   ifdef LORA_ENABLE_US_902_928
    case US_902_928:
#   endif           
        if((trial & 1U) > 0U){
            
            retval = 4U;
        }
        else{
            
            retval = 3U - ((trial >> 1U) % (4U - LORA_DEFAULT_RATE));
        }
        break;
#endif
#if defined(LORA_ENABLE_AU_915_928)              
#   ifdef LORA_ENABLE_AU_915_928
    case AU_915_928:
#   endif                
        if((trial & 1U) > 0U){
            
            retval = 0U;
        }
        else{
            
            retval = 4U - ((trial >> 1U) % (5U - LORA_DEFAULT_RATE));
        }
        break;
#endif        
    default:
        break;
    }
    
    return retval;
}

/* static functions ***************************************************/

static bool upRateRange(enum lora_region region, uint8_t chIndex, uint8_t *minRate, uint8_t *maxRate)
{
    bool retval = false;
    
    switch(region){    
#if defined(LORA_ENABLE_EU_863_870) || defined(LORA_ENABLE_EU_433)                      
#   ifdef LORA_ENABLE_EU_863_870    
    case EU_863_870:
#   endif    
#   ifdef LORA_ENABLE_EU_433    
    case EU_433:
#   endif       
    
        if(chIndex < 16U){
    
            *minRate = 0U;
            *maxRate = 5U;
            retval = true;
        }
        break;
#endif        
#ifdef LORA_ENABLE_US_902_928
    case US_902_928:
    
        if(chIndex <= 71U){
    
            if(chIndex <= 63U){
                
                *minRate = 0U;
                *maxRate = 3U;
            }
            else{ 
                
                *minRate = 4U;
                *maxRate = 4U;                
            }            
            
            retval = true;
        }
        break;        
#endif    
#ifdef LORA_ENABLE_AU_915_928
    case AU_915_928:
    
        if(chIndex <= 71U){
    
            if(chIndex <= 63U){
                
                *minRate = 0U;
                *maxRate = 5U;
            }
            else{ 
                
                *minRate = 6U;
                *maxRate = 6U;                
            }            
            
            retval = true;
        }
        break;        
#endif    
    default:
        *minRate = 0U;
        *maxRate = 0U;
        break;      
    }
    
    return retval;
}

#ifndef LORA_ENABLE_AVR
    #undef memcpy_P
#endif
