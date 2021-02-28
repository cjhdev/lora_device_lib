/* Copyright (c) 2019-2021 Cameron Harper
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

#include <string.h>

#include "ldl_debug.h"
#include "ldl_radio.h"
#include "ldl_system.h"
#include "ldl_internal.h"

/* functions **********************************************************/

const struct ldl_radio_interface *LDL_Radio_getInterface(const struct ldl_radio *self)
{
    const struct ldl_radio_interface *retval;

    switch(self->type){
    default:
    case LDL_RADIO_NONE:
        retval = NULL;
        break;
#ifdef LDL_ENABLE_SX1272
    case LDL_RADIO_SX1272:
        retval = LDL_SX1272_getInterface();
        break;
#endif
#ifdef LDL_ENABLE_SX1276
    case LDL_RADIO_SX1276:
        retval = LDL_SX1276_getInterface();
        break;
#endif
#ifdef LDL_ENABLE_SX1261
    case LDL_RADIO_SX1261:
        retval = LDL_SX1261_getInterface();
        break;
#endif
#ifdef LDL_ENABLE_SX1262
    case LDL_RADIO_SX1262:
        retval = LDL_SX1262_getInterface();
        break;
#endif
#ifdef LDL_ENABLE_WL55
    case LDL_RADIO_WL55:
        retval = LDL_WL55_getInterface();
        break;
#endif
    }

    return retval;
}

void LDL_Radio_setEventCallback(struct ldl_radio *self, struct ldl_mac *ctx, ldl_radio_event_fn cb)
{
    LDL_PEDANTIC(self != NULL)

    self->cb = cb;
    self->cb_ctx = ctx;
}

void LDL_Radio_handleInterrupt(struct ldl_radio *self, uint8_t n)
{
    LDL_PEDANTIC(self != NULL)

    (void)n;

    if(self->cb != NULL){

        self->cb(self->cb_ctx);
    }
}

int16_t LDL_Radio_getMinSNR(enum ldl_spreading_factor sf)
{
    int16_t retval = 0;

    /* applicable to 1272 and 1276 */
    switch(sf){
    default:
    case LDL_SF_7:
        retval = -750;
        break;
    case LDL_SF_8:
        retval = -1000;
        break;
    case LDL_SF_9:
        retval = -1250;
        break;
    case LDL_SF_10:
        retval = -1500;
        break;
    case LDL_SF_11:
        retval = -1750;
        break;
    case LDL_SF_12:
        retval = -2000;
        break;
    }

    return retval;
}

uint32_t LDL_Radio_getAirTime(enum ldl_signal_bandwidth bw, enum ldl_spreading_factor sf, uint8_t size, bool crc)
{
    /* from 4.1.1.7 of sx1272 datasheet
     *
     * Ts (symbol period)
     * Rs (symbol rate)
     * PL (payload length)
     * SF (spreading factor
     * CRC (presence of trailing CRC)
     * IH (presence of implicit header)
     * DE (presence of data rate optimize)
     * CR (coding rate 1..4)
     *
     *
     * Ts = 1 / Rs
     * Tpreamble = ( Npreamble x 4.25 ) x Tsym
     *
     * Npayload = 8 + max( ceil[( 8PL - 4SF + 28 + 16CRC + 20IH ) / ( 4(SF - 2DE) )] x (CR + 4), 0 )
     *
     * Tpayload = Npayload x Ts
     *
     * Tpacket = Tpreamble + Tpayload
     *
     * */

    bool header;
    bool lowDataRateOptimize;
    uint32_t Tpacket;
    uint32_t Ts;
    uint32_t Tpreamble;
    uint32_t numerator;
    uint32_t denom;
    uint32_t Npayload;
    uint32_t Tpayload;

    /* optimise this mode according to the datasheet */
    lowDataRateOptimize = ((bw == LDL_BW_125) && ((sf == LDL_SF_11) || (sf == LDL_SF_12))) ? true : false;

    /* lorawan always uses a header */
    header = true;

    Ts = ((U32(1) << sf) * U32(1000000)) / LDL_Radio_bwToNumber(bw);
    Tpreamble = (Ts * U32(12)) +  (Ts / U32(4));

    numerator = (U32(8) * U32(size)) - (U32(4) * U32(sf)) + U32(28) + ( crc ? U32(16) : U32(0) ) - ( header ? U32(20) : U32(0) );
    denom = U32(4) * (U32(sf) - ( lowDataRateOptimize ? U32(2) : U32(0) ));

    Npayload = U32(8) + ((((numerator / denom) + (((numerator % denom) != 0U) ? U32(1) : U32(0))) * (U32(LDL_CR_5) + U32(4))));

    Tpayload = Npayload * Ts;

    Tpacket = Tpreamble + Tpayload;

    /* convert to us to ms and overestimate */
    return (Tpacket / U32(1000)) + U32(1);
}

uint32_t LDL_Radio_bwToNumber(enum ldl_signal_bandwidth bw)
{
    uint32_t retval;

    switch(bw){
    default:
    case LDL_BW_125:
        retval = U32(125000);
        break;
    case LDL_BW_250:
        retval = U32(250000);
        break;
    case LDL_BW_500:
        retval = U32(500000);
        break;
    }

    return retval;
}

