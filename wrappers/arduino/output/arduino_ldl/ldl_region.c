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

#include "ldl_region.h"
#include "ldl_debug.h"
#include "ldl_mac.h"

#ifdef LDL_ENABLE_AVR

    #include <avr/pgmspace.h>
    
#else

    #include <string.h>
    
    #define PROGMEM
    #define memcpy_P memcpy
    
#endif

#include <stddef.h>
    
/* static function prototypes *****************************************/

static bool upRateRange(enum ldl_region region, uint8_t chIndex, uint8_t *minRate, uint8_t *maxRate);

/* functions **********************************************************/

void LDL_Region_convertRate(enum ldl_region region, uint8_t rate, enum ldl_spreading_factor *sf, enum ldl_signal_bandwidth *bw, uint8_t *mtu)
{
    LDL_PEDANTIC(sf != NULL)
    LDL_PEDANTIC(bw != NULL)
    LDL_PEDANTIC(mtu != NULL)
    
    switch(region){ 
#if defined(LDL_ENABLE_EU_863_870) || defined(LDL_ENABLE_EU_433)                              
#   ifdef LDL_ENABLE_EU_863_870    
    case LDL_EU_863_870:
#   endif    
#   ifdef LDL_ENABLE_EU_433    
    case LDL_EU_433:
#   endif        
        switch(rate){
        case 0U:
            *sf = LDL_SF_12;
            *bw = LDL_BW_125;
            *mtu = 59U;
            break;
        case 1U:
            *sf = LDL_SF_11;
            *bw = LDL_BW_125;
            *mtu = 59U;
            break;
        case 2U:
            *sf = LDL_SF_10;
            *bw = LDL_BW_125;
            *mtu = 59U;
            break;
        case 3U:
            *sf = LDL_SF_9;
            *bw = LDL_BW_125;
            *mtu = 123U;
            break;
        case 4U:
            *sf = LDL_SF_8;
            *bw = LDL_BW_125;
            *mtu = 250U;
            break;
        case 5U:        
            *sf = LDL_SF_7;
            *bw = LDL_BW_125;
            *mtu = 250U;
            break;
        case 6U:
            *sf = LDL_SF_7;
            *bw = LDL_BW_250;
            *mtu = 250U;
            break;                        
        default:
            *sf = LDL_SF_7;
            *bw = LDL_BW_125;
            *mtu = 250U;
            LDL_INFO(NULL,"invalid rate")
            break;
        }
        break;
#endif        
#ifdef LDL_ENABLE_US_902_928
    case LDL_US_902_928:
    
        switch(rate){
        case 0U:
            *sf = LDL_SF_10;
            *bw = LDL_BW_125;
            *mtu = 19U;
            break;
        case 1U:
            *sf = LDL_SF_9;
            *bw = LDL_BW_125;
            *mtu = 61U;
            break;
        case 2U:
            *sf = LDL_SF_8;
            *bw = LDL_BW_125;
            *mtu = 133U;
            break;
        case 3U:
            *sf = LDL_SF_7;
            *bw = LDL_BW_125;
            *mtu = 250U;
            break;
        case 4U:
        case 12U:
            *sf = LDL_SF_8;
            *bw = LDL_BW_500;
            *mtu = 250U;
            break;
        case 8U:
            *sf = LDL_SF_12;
            *bw = LDL_BW_500;
            *mtu = 61U;
            break;
        case 9U:
            *sf = LDL_SF_11;
            *bw = LDL_BW_500;
            *mtu = 137U;
            break;
        case 10U:
            *sf = LDL_SF_10;
            *bw = LDL_BW_500;
            *mtu = 250U;
            break;
        case 11U:
            *sf = LDL_SF_9;
            *bw = LDL_BW_500;
            *mtu = 250U;
            break;        
        case 13U:
            *sf = LDL_SF_7;
            *bw = LDL_BW_500;
            *mtu = 250U;
            break;
        default:
            *sf = LDL_SF_7;
            *bw = LDL_BW_125;
            *mtu = 250U;
            LDL_INFO(NULL,"invalid rate")
            break;
        }    
        break;
#endif                
#ifdef LDL_ENABLE_AU_915_928
    case LDL_AU_915_928:
    
        switch(rate){
        case 0U:
            *sf = LDL_SF_12;
            *bw = LDL_BW_125;
            *mtu = 59U;
            break;
        case 1U:
            *sf = LDL_SF_11;
            *bw = LDL_BW_125;
            *mtu = 59U;
            break;
        case 2U:
            *sf = LDL_SF_10;
            *bw = LDL_BW_125;
            *mtu = 59U;
            break;
        case 3U:
            *sf = LDL_SF_9;
            *bw = LDL_BW_125;
            *mtu = 123U;
            break;
        case 4U:
            *sf = LDL_SF_8;
            *bw = LDL_BW_125;
            *mtu = 250U;
            break;
        case 5U:
            *sf = LDL_SF_7;
            *bw = LDL_BW_125;
            *mtu = 250U;
            break;
        case 6U:
        case 12U:
            *sf = LDL_SF_8;
            *bw = LDL_BW_500;
            *mtu = 250U;
            break;
        case 8U:
            *sf = LDL_SF_12;
            *bw = LDL_BW_500;
            *mtu = 61U;
            break;
        case 9U:
            *sf = LDL_SF_11;
            *bw = LDL_BW_500;
            *mtu = 137U;
            break;
        case 10U:
            *sf = LDL_SF_10;
            *bw = LDL_BW_500;
            *mtu = 250U;
            break;
        case 11U:
            *sf = LDL_SF_9;
            *bw = LDL_BW_500;
            *mtu = 250U;
            break;        
        case 13U:
            *sf = LDL_SF_7;
            *bw = LDL_BW_500;
            *mtu = 250U;
            break;        
        default:
            *sf = LDL_SF_7;
            *bw = LDL_BW_125;
            *mtu = 250U;
            LDL_INFO(NULL,"invalid rate")
            break;
        }    
        break;
#endif                
    default:
        break;
    }
}

bool LDL_Region_getBand(enum ldl_region region, uint32_t freq, uint8_t *band)
{
    LDL_PEDANTIC(band != NULL)

    bool retval = false;

    switch(region){
#ifdef LDL_ENABLE_EU_863_870        
    case LDL_EU_863_870:
    
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

bool LDL_Region_isDynamic(enum ldl_region region)
{
    bool retval;
    
    switch(region){
    default:
        retval = true;
        break;        
#ifdef LDL_ENABLE_US_902_928
    case LDL_US_902_928:         
        retval = false;
        break;
#endif
#ifdef LDL_ENABLE_AU_915_928
    case LDL_AU_915_928:
        retval = false;                    
        break;
#endif        
    }
    
    return retval;
}

bool LDL_Region_getChannel(enum ldl_region region, uint8_t chIndex, uint32_t *freq, uint8_t *minRate, uint8_t *maxRate)
{
    bool retval = false;
    
    (void)chIndex;
    (void)freq;
    (void)minRate;
    (void)maxRate;
    
    switch(region){
    default:
        break;    
#ifdef LDL_ENABLE_US_902_928
    case LDL_US_902_928:         

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
#ifdef LDL_ENABLE_AU_915_928
    case LDL_AU_915_928:
    
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

uint8_t LDL_Region_numChannels(enum ldl_region region)
{
    uint8_t retval;
    
    switch(region){
    default:
        retval = 16U;    
        break;
#if defined(LDL_ENABLE_US_902_928)  || defined(LDL_ENABLE_AU_915_928)              
#   ifdef LDL_ENABLE_US_902_928
    case LDL_US_902_928:
#   endif                 
#   ifdef LDL_ENABLE_AU_915_928
    case LDL_AU_915_928:
#   endif     
           
        retval = 72U;
        break;                
#endif        
    }
    
    return retval;    
}

void LDL_Region_getDefaultChannels(enum ldl_region region, struct ldl_mac *mac)
{
    LDL_PEDANTIC(mac != NULL)
    
    uint8_t minRate;
    uint8_t maxRate;
    
    switch(region){
    default:
        break;    
#ifdef LDL_ENABLE_EU_863_870
    case LDL_EU_863_870:
    
        (void)upRateRange(region, 0U, &minRate, &maxRate);
        
        LDL_MAC_addChannel(mac, 0U, 868100000UL, minRate, maxRate);
        LDL_MAC_addChannel(mac, 1U, 868300000UL, minRate, maxRate);
        LDL_MAC_addChannel(mac, 2U, 868500000UL, minRate, maxRate);        
        break;
#endif        
#ifdef LDL_ENABLE_EU_433    
    case LDL_EU_433:
    
        (void)upRateRange(region, 0U, &minRate, &maxRate);
        
        LDL_MAC_addChannel(mac, 0U, 433175000UL, minRate, maxRate);
        LDL_MAC_addChannel(mac, 1U, 433375000UL, minRate, maxRate);
        LDL_MAC_addChannel(mac, 2U, 433575000UL, minRate, maxRate);        
        break;
#endif        
    }
}

#if defined(LDL_ENABLE_EU_863_870) || defined(LDL_ENABLE_EU_433)         
static uint8_t unpackCFListFreq(const uint8_t *cfList, uint32_t *freq)
{
    *freq = cfList[2];
    *freq <<= 8;
    *freq |= cfList[1];
    *freq <<= 8;
    *freq |= cfList[0];
    
    *freq *= 100UL;
    
    return 3U;
}
#endif

#if defined(LDL_ENABLE_US_902_928) || defined(LDL_ENABLE_AU_915_928)
static uint8_t unpackCFListMask(const uint8_t *cfList, uint16_t *mask)
{
    *mask = cfList[1];
    *mask <<= 8;
    *mask |= cfList[0];
    
    return 2U;    
}
#endif

void LDL_Region_processCFList(enum ldl_region region, struct ldl_mac *mac, const uint8_t *cfList, uint8_t cfListLen)
{
    if(cfListLen == 16U){
    
        switch(region){
        default:
            break;

#if defined(LDL_ENABLE_EU_863_870) || defined(LDL_ENABLE_EU_433)

#   ifdef LDL_ENABLE_EU_863_870        
        case LDL_EU_863_870:
#   endif
#   ifdef LDL_ENABLE_EU_433
        case LDL_EU_433:
#   endif        
           /* 0 means frequency list */
           if(cfList[15] == 0U){
               
                uint8_t minRate;
                uint8_t maxRate;
                uint32_t freq;
                uint8_t i;
                uint8_t pos;
                
                for(i=3U,pos=0U; i < 8U; i++){
                
                     pos += unpackCFListFreq(&cfList[pos], &freq);
                     
                     (void)upRateRange(region, i, &minRate, &maxRate);                 
                     
                     (void)LDL_MAC_addChannel(mac, i, freq, minRate, maxRate);
                }            
            }        
            break;
#endif  

#if defined(LDL_ENABLE_US_902_928) || defined(LDL_ENABLE_AU_915_928)
      
#   ifdef LDL_ENABLE_US_902_928
        case LDL_US_902_928:
            break;
#   endif
#   ifdef LDL_ENABLE_AU_915_928
        case LDL_AU_915_928:        
#   endif
            /* 1 means mask list */
           if(cfList[15] == 1U){
                
                uint16_t mask;
                uint8_t i;
                uint8_t b;
                uint8_t pos;
                
                for(i=0U,pos=0U; i < 5U; i++){
                
                    pos += unpackCFListMask(&cfList[pos], &mask);
                     
                     for(b=0U; b < 16U; b++){
                                        
                        if((mask & (1 << b)) > 0U){ 
                        
                            (void)LDL_MAC_unmaskChannel(mac, (i * 16U) + b);
                        }
                    }                 
                }            
            }        
            break;
#endif        
        }
    }    
}

uint32_t LDL_Region_getOffTimeFactor(enum ldl_region region, uint8_t band)
{
    uint32_t retval = 0UL;
    
    switch(region){
    default:
        break;    
#ifdef LDL_ENABLE_EU_863_870    
    case LDL_EU_863_870:
    
        switch(band){
        case 0U:
        case 1U:
        case 4U:
            retval = 100UL;      // 1.0%
            break;
        case 2U:
            retval = 1000UL;     // 0.1%
            break;
        case 3U:        
            retval = 10UL;       // 10.0%
            break;                    
        default:
            break;
        }
        break;
#endif        
#ifdef LDL_ENABLE_EU_433    
    case LDL_EU_433:        
        retval = 100UL;
        break;    
#endif
    }
    
    return retval;    
}

bool LDL_Region_validateRate(enum ldl_region region, uint8_t chIndex, uint8_t minRate, uint8_t maxRate)
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

bool LDL_Region_validateFreq(enum ldl_region region, uint8_t chIndex, uint32_t freq)
{
    (void)region;
    (void)chIndex;
    (void)freq;
    
    return true;
}

void LDL_Region_getRX1DataRate(enum ldl_region region, uint8_t tx_rate, uint8_t rx1_offset, uint8_t *rx1_rate)
{
    LDL_PEDANTIC(rx1_rate != NULL)

    const uint8_t *ptr = NULL;
    uint16_t i = 0U;
    size_t size = 0U;
    
    switch(region){
    default:
#if defined(LDL_ENABLE_EU_863_870) || defined(LDL_ENABLE_EU_433)        
#   ifdef LDL_ENABLE_EU_863_870
    case LDL_EU_863_870:
#   endif
#   ifdef LDL_ENABLE_EU_433
    case LDL_EU_433:
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

        i = (tx_rate * 6U) + rx1_offset;
        ptr = rates;
        size = sizeof(rates);
    }
        break;                   
#endif        
#ifdef LDL_ENABLE_US_902_928
    case LDL_US_902_928:        
    {
        static const uint8_t rates[] PROGMEM = {
            10U, 9U,  8U,  8U,
            11U, 10U, 9U,  8U,
            12U, 11U, 10U, 9U,
            13U, 12U, 11U, 10U,
            13U, 13U, 12U, 11U,
        };
        
        i = (tx_rate * 4U) + rx1_offset;
        ptr = rates;
        size = sizeof(rates);
    }
        break;
#endif        
#ifdef LDL_ENABLE_AU_915_928
    case LDL_AU_915_928:        
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
        
        i = (tx_rate * 6U) + rx1_offset;
        ptr = rates;
        size = sizeof(rates);
    }
        break;
#endif        
    }
    
    if(ptr != NULL){
    
        if(i < size){
        
            (void)memcpy_P(rx1_rate, &ptr[i], sizeof(*rx1_rate));
        }
        else{
                        
            *rx1_rate = tx_rate;
            LDL_INFO(NULL,"out of range error")
        }
    }        
}

void LDL_Region_getRX1Freq(enum ldl_region region, uint32_t txFreq, uint8_t chIndex, uint32_t *freq)
{
    (void)chIndex;
    
    switch(region){
    default:
        *freq = txFreq;
        break;        
#if defined(LDL_ENABLE_US_902_928)  || defined(LDL_ENABLE_AU_915_928)              
#   ifdef LDL_ENABLE_US_902_928
    case LDL_US_902_928:
#   endif                 
#   ifdef LDL_ENABLE_AU_915_928
    case LDL_AU_915_928:
#   endif     
               
        *freq = 923300000UL + ((uint32_t)(chIndex % 8U) * 600000UL);       
        break;    
#endif        
    }
}

uint8_t LDL_Region_getRX1Delay(enum ldl_region region)
{
    (void)region;
    
    return 1U;    
}

uint8_t LDL_Region_getJA1Delay(enum ldl_region region)
{
    (void)region;
    
    return 5U;        
}

uint8_t LDL_Region_getRX1Offset(enum ldl_region region)
{
    (void)region;
    
    return 0U;        
}

uint32_t LDL_Region_getRX2Freq(enum ldl_region region)
{
    uint32_t retval;
    
    switch(region){
    default:
#ifdef LDL_ENABLE_EU_863_870    
    case LDL_EU_863_870:
        retval = 869525000UL;
        break;    
#endif
#ifdef LDL_ENABLE_EU_433        
    case LDL_EU_433:
        retval = 434665000UL;
        break;    
#endif
#ifdef LDL_ENABLE_US_902_928
    case LDL_US_902_928:         
        retval = 923300000UL;
        break;        
#endif
#ifdef LDL_ENABLE_AU_915_928
    case LDL_AU_915_928:        
        retval = 923300000UL;
        break;        
#endif        
    }
    
    return retval; 
}

uint8_t LDL_Region_getRX2Rate(enum ldl_region region)
{
    uint8_t retval;
    
    switch(region){
    default:
#ifdef LDL_ENABLE_EU_863_870
    case LDL_EU_863_870:
        retval = 0U;
        break;
#endif
#ifdef LDL_ENABLE_EU_433
    case LDL_EU_433:
        retval = 0U;
        break;
#endif        
#ifdef LDL_ENABLE_US_902_928
    case LDL_US_902_928:     
        retval = 8U;
        break;
#endif
#ifdef LDL_ENABLE_AU_915_928
    case LDL_AU_915_928:
        retval = 8U;
        break;
#endif                
    }
    
    return retval; 
}

bool LDL_Region_validateTXPower(enum ldl_region region, uint8_t power)
{
    bool retval = false;
    
    switch(region){ 
#ifdef LDL_ENABLE_EU_863_870               
    case LDL_EU_863_870:
        
        if(power <= 7U){
            
            retval = true;            
        }
        break;
#endif
#ifdef LDL_ENABLE_EU_433        
    case LDL_EU_433:
    
        if(power <= 5U){
            
            retval = true;
        }
        break;
#endif            
#if defined(LDL_ENABLE_US_902_928)  || defined(LDL_ENABLE_AU_915_928)              
#   ifdef LDL_ENABLE_US_902_928
    case LDL_US_902_928:
#   endif                 
#   ifdef LDL_ENABLE_AU_915_928
    case LDL_AU_915_928:
#   endif     
        
        if(power <= 10U){
            
            retval = true;
        }
        break;
#endif    
    default:
        break;      
    }
    
    return retval;
}

int16_t LDL_Region_getTXPower(enum ldl_region region, uint8_t power)
{
    int16_t retval = 0;
    
    switch(region){ 
#ifdef LDL_ENABLE_EU_863_870               
    case LDL_EU_863_870:
        
        if(power <= 7U){
            
            retval = 1600 - (power * 200);
        }
        else{
            
            retval = 1600 - (7U * 200);
        }
        break;
#endif
#ifdef LDL_ENABLE_EU_433        
    case LDL_EU_433:
    
        if(power <= 5U){
            
            retval = 1215 - (power * 200);
        }
        else{
            
            retval = 1215 - (5U * 200);
        }
        break;
#endif            
#if defined(LDL_ENABLE_US_902_928)  || defined(LDL_ENABLE_AU_915_928)              
#   ifdef LDL_ENABLE_US_902_928
    case LDL_US_902_928:
#   endif                 
#   ifdef LDL_ENABLE_AU_915_928
    case LDL_AU_915_928:
#   endif     
        
        if(power <= 10U){
            
            retval = 3000 - (power * 200);
        }
        else{
            
            retval = 3000 - (10U * 200);
        }
        break;
#endif    
    default:
        break;      
    }
    
    return retval;
}

uint8_t LDL_Region_getJoinRate(enum ldl_region region, uint32_t trial)
{
    uint8_t retval = 0U;
    
    switch(region){    
#if defined(LDL_ENABLE_EU_863_870) || defined(LDL_ENABLE_EU_433)                      
#   ifdef LDL_ENABLE_EU_863_870    
    case LDL_EU_863_870:
#   endif    
#   ifdef LDL_ENABLE_EU_433    
    case LDL_EU_433:
#   endif       
        retval = 5U - (trial % (6U - LDL_DEFAULT_RATE));
        break;
#endif        
#if defined(LDL_ENABLE_US_902_928)        
#   ifdef LDL_ENABLE_US_902_928
    case LDL_US_902_928:
#   endif           
        if((trial & 1U) > 0U){
            
            retval = 4U;
        }
        else{
            
            retval = 3U - ((trial >> 1U) % (4U - LDL_DEFAULT_RATE));
        }
        break;
#endif
#if defined(LDL_ENABLE_AU_915_928)              
#   ifdef LDL_ENABLE_AU_915_928
    case LDL_AU_915_928:
#   endif                
        if((trial & 1U) > 0U){
            
            retval = 0U;
        }
        else{
            
            retval = 4U - ((trial >> 1U) % (5U - LDL_DEFAULT_RATE));
        }
        break;
#endif        
    default:
        break;
    }
    
    return retval;
}

uint32_t LDL_Region_getMaxDCycleOffLimit(enum ldl_region region)
{
    (void)region;
    
    /* I've only checked this for ETSI:
     * 
     * duty-cycle is evaluated over one hour therefore we can effectively
     * limit ourselves by never accumulating more than one hour of 
     * off-time.
     * 
     * */
    return (60UL*60UL*1000UL);
}

/* static functions ***************************************************/

static bool upRateRange(enum ldl_region region, uint8_t chIndex, uint8_t *minRate, uint8_t *maxRate)
{
    bool retval = false;
    
    switch(region){    
#if defined(LDL_ENABLE_EU_863_870) || defined(LDL_ENABLE_EU_433)                      
#   ifdef LDL_ENABLE_EU_863_870    
    case LDL_EU_863_870:
#   endif    
#   ifdef LDL_ENABLE_EU_433    
    case LDL_EU_433:
#   endif       
    
        if(chIndex < 16U){
    
            *minRate = 0U;
            *maxRate = 5U;
            retval = true;
        }
        break;
#endif        
#ifdef LDL_ENABLE_US_902_928
    case LDL_US_902_928:
    
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
#ifdef LDL_ENABLE_AU_915_928
    case LDL_AU_915_928:
    
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



#ifndef LDL_ENABLE_AVR
    #undef memcpy_P
#endif
