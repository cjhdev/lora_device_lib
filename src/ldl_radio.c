/* Copyright (c) 2019-2020 Cameron Harper
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
#include "ldl_radio_registers.h"
#include "ldl_internal.h"

#if defined(LDL_ENABLE_SX1272) || defined(LDL_ENABLE_SX1276)

const struct ldl_radio_interface LDL_Radio_interface = {

    .set_mode = LDL_Radio_setMode,
    .read_entropy = LDL_Radio_readEntropy,
    .read_buffer = LDL_Radio_readBuffer,
    .transmit = LDL_Radio_transmit,
    .receive = LDL_Radio_receive,
    .get_xtal_delay = LDL_Radio_getXTALDelay,
    .receive_entropy = LDL_Radio_receiveEntropy
};

struct modem_config {

    enum ldl_signal_bandwidth bw;
    enum ldl_spreading_factor sf;
    bool crc;
    uint16_t timeout;
};

/* static function prototypes *****************************************/

static enum ldl_radio_event interruptToEvent(struct ldl_radio *self, uint8_t n);
static uint8_t readFIFO(struct ldl_radio *self, uint8_t *data, uint8_t max);
static void setFreq(struct ldl_radio *self, uint32_t freq);
static void setPower(struct ldl_radio *self, int16_t dbm);
static uint8_t readReg(struct ldl_radio *self, enum ldl_radio_sx1272_sx1276_register reg);
static void writeReg(struct ldl_radio *self, enum ldl_radio_sx1272_sx1276_register reg, uint8_t data);
static void burstRead(struct ldl_radio *self, enum ldl_radio_sx1272_sx1276_register reg, uint8_t *data, uint8_t len);
static void burstWrite(struct ldl_radio *self, enum ldl_radio_sx1272_sx1276_register reg, const uint8_t *data, uint8_t len);
static void setOpRXSingle(struct ldl_radio *self);
static void setOpTX(struct ldl_radio *self);
static void setOpRXContinuous(struct ldl_radio *self);
static void setOpStandby(struct ldl_radio *self);
static void setOpSleep(struct ldl_radio *self);
static void setXTALReg(struct ldl_radio *self, enum ldl_radio_sx1272_sx1276_register reg);
static void setXTAL(struct ldl_radio *self);
static void setModemConfig1(struct ldl_radio *self, const struct modem_config *config);
static void setModemConfig2(struct ldl_radio *self, const struct modem_config *config);
static void setModemConfig3(struct ldl_radio *self, const struct modem_config *config);

#ifdef LDL_ENABLE_RADIO_DEBUG
static void rxDiagnostics(struct ldl_radio *self);
#endif

/* functions **********************************************************/

void LDL_Radio_init(struct ldl_radio *self, const struct ldl_radio_init_arg *arg)
{
    LDL_PEDANTIC(self != NULL)
    LDL_PEDANTIC(arg != NULL)

    LDL_PEDANTIC(arg->chip_read != NULL)
    LDL_PEDANTIC(arg->chip_write != NULL)
    LDL_PEDANTIC(arg->chip_set_mode != NULL)

    (void)memset(self, 0, sizeof(*self));

    self->chip = arg->chip;
    self->pa = arg->pa;
    self->type = arg->type;
    self->chip_read = arg->chip_read;
    self->chip_write = arg->chip_write;
    self->chip_set_mode = arg->chip_set_mode;
    self->tx_gain = arg->tx_gain;
    self->xtal = arg->xtal;
    self->xtal_delay = arg->xtal_delay;

    LDL_DEBUG("pa=%s type=%s tx_gain=%d xtal=%s xtal_delay=%u",
        LDL_Radio_paToString(self->pa),
        LDL_Radio_typeToString(self->type),
        self->tx_gain,
        LDL_Radio_xtalToString(self->xtal),
        self->xtal_delay
    )
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

    if(self->cb != NULL){

        self->cb(self->cb_ctx, interruptToEvent(self, n));
    }
}

void LDL_Radio_setMode(struct ldl_radio *self, enum ldl_radio_mode mode)
{
    LDL_PEDANTIC(self != NULL)

#ifdef LDL_ENABLE_RADIO_DEBUG
    LDL_Radio_debugLogReset(self);
#endif

    switch(mode){
    default:
    case LDL_RADIO_MODE_RESET:

        if(self->mode == LDL_RADIO_MODE_STANDBY){

#ifdef LDL_ENABLE_RADIO_DEBUG
            rxDiagnostics(self);
#endif
        }

        self->chip_set_mode(self->chip, LDL_CHIP_MODE_RESET);
        break;

    case LDL_RADIO_MODE_SLEEP:

        switch(self->mode){
        default:
        /* goto sleep from reset */
        case LDL_RADIO_MODE_RESET:

            self->chip_set_mode(self->chip, LDL_CHIP_MODE_SLEEP);
            /* do not write to registers coming out of reset! */
            break;

        /* goto sleep from sleep */
        case LDL_RADIO_MODE_SLEEP:
            setOpSleep(self);
            break;

        /* goto sleep from standby */
        case LDL_RADIO_MODE_STANDBY:

#ifdef LDL_ENABLE_RADIO_DEBUG
            rxDiagnostics(self);
#endif
            setOpStandby(self);
            writeReg(self, RegIrqFlags, 0xffU);
            writeReg(self, RegIrqFlagsMask, 0xffU);
            self->chip_set_mode(self->chip, LDL_CHIP_MODE_SLEEP);
            setOpSleep(self);
            break;
        }
        break;

    case LDL_RADIO_MODE_STANDBY:

        switch(self->mode){
        default:
        case LDL_RADIO_MODE_RESET:
            /* invalid transition */
            LDL_ASSERT(false)
            break;

        /* goto standby from sleep */
        case LDL_RADIO_MODE_SLEEP:

            self->chip_set_mode(self->chip, LDL_CHIP_MODE_STANDBY);
            setOpSleep(self);
            setXTAL(self);
            setOpStandby(self);
            break;

        /* goto standby from standby */
        case LDL_RADIO_MODE_STANDBY:

#ifdef LDL_ENABLE_RADIO_DEBUG
            rxDiagnostics(self);
#endif
            self->chip_set_mode(self->chip, LDL_CHIP_MODE_STANDBY);
            setOpStandby(self);
            writeReg(self, RegIrqFlags, 0xffU);
            writeReg(self, RegIrqFlagsMask, 0xffU);
            break;
        }
        break;
    }

    LDL_DEBUG("new_mode=%s cur_mode=%s", LDL_Radio_modeToString(mode), LDL_Radio_modeToString(self->mode))

#ifdef LDL_ENABLE_RADIO_DEBUG
    LDL_Radio_debugLogFlush(self, __FUNCTION__);
#endif

    self->mode = mode;
}

void LDL_Radio_transmit(struct ldl_radio *self, const struct ldl_radio_tx_setting *settings, const void *data, uint8_t len)
{
    LDL_PEDANTIC(self != NULL)
    LDL_PEDANTIC(settings != NULL)
    LDL_PEDANTIC((data != NULL) || (len == 0U))
    LDL_PEDANTIC(settings->freq != 0U)
    LDL_PEDANTIC(self->mode == LDL_RADIO_MODE_STANDBY)

    struct modem_config config = {
        .sf = settings->sf,
        .bw = settings->bw,
        .timeout = 0U,
        .crc = true
    };

#ifdef LDL_ENABLE_RADIO_DEBUG
    LDL_Radio_debugLogReset(self);

    rxDiagnostics(self);
#endif

    self->dio_mapping1 = 0x40U;

    setPower(self, settings->dbm);                      // configure PA and accessory IO

    setModemConfig1(self, &config);
    setModemConfig2(self, &config);
    setModemConfig3(self, &config);

    writeReg(self, RegSyncWord, 0x34U);                  // set sync word
    writeReg(self, RegInvertIQ, 0x27U);                 // non-invert IQ
    writeReg(self, RegDioMapping1, self->dio_mapping1); // DIO0 (TX_COMPLETE) DIO1 (RX_DONE)
    writeReg(self, RegIrqFlags, 0xffU);                  // clear interrupts
    writeReg(self, RegIrqFlagsMask, 0xf7U);             // unmask TX_DONE interrupt
    writeReg(self, RegFifoTxBaseAddr, 0U);              // set tx base
    writeReg(self, RegFifoAddrPtr, 0U);                 // set address pointer
    writeReg(self, LoraRegPayloadLength, len);          // bytes to transmit (note. datasheet doesn't say we have to but in practice we do)
    burstWrite(self, RegFifo, data, len);               // write buffer

    setFreq(self, settings->freq);                      // set carrier frequency

    /* read everything back for debug */
#ifdef LDL_ENABLE_RADIO_DEBUG
    (void)readReg(self, RegModemConfig1);
    (void)readReg(self, RegModemConfig2);
#ifdef LDL_ENABLE_SX1276
    if(self->type == LDL_RADIO_SX1276){
        (void)readReg(self, RegModemConfig3);
    }
#endif
    (void)readReg(self, RegSyncWord);
    (void)readReg(self, RegInvertIQ);
    (void)readReg(self, RegDioMapping1);
    (void)readReg(self, RegIrqFlags);
    (void)readReg(self, RegIrqFlagsMask);
    (void)readReg(self, RegFifoTxBaseAddr);
    (void)readReg(self, RegFifoAddrPtr);
    (void)readReg(self, LoraRegPayloadLength);
    (void)readReg(self, RegFrfMsb);
    (void)readReg(self, RegFrfMid);
    (void)readReg(self, RegFrfLsb);
#endif

    setOpTX(self);                                      // TX

#ifdef LDL_ENABLE_RADIO_DEBUG
    LDL_Radio_debugLogFlush(self, __FUNCTION__);
#endif
}

void LDL_Radio_receive(struct ldl_radio *self, const struct ldl_radio_rx_setting *settings)
{
    LDL_PEDANTIC(self != NULL)
    LDL_PEDANTIC(settings != NULL)
    LDL_PEDANTIC(settings->freq != 0U)
    LDL_PEDANTIC(self->mode == LDL_RADIO_MODE_STANDBY)

    uint16_t timeout;

    if(settings->timeout > 0x3ffU){

        timeout = 0x3ffU;
    }
    else if(settings->timeout < 8U){

        timeout = 8U;
    }
    else{

        timeout = settings->timeout;
    }

    struct modem_config config = {
        .sf = settings->sf,
        .bw = settings->bw,
        .timeout = timeout,
        .crc = false
    };

#ifdef LDL_ENABLE_RADIO_DEBUG
    LDL_Radio_debugLogReset(self);

    rxDiagnostics(self);
#endif

    self->dio_mapping1 = 0U;

    self->chip_set_mode(self->chip, LDL_CHIP_MODE_RX);                  // configure accessory IO

    /* modem config */
    setModemConfig1(self, &config);
    setModemConfig2(self, &config);
    setModemConfig3(self, &config);

    writeReg(self, RegSymbTimeoutLsb, (uint8_t)timeout);    // set symbol timeout
    writeReg(self, RegSyncWord, 0x34);                      // set sync word
    writeReg(self, RegLna, 0x23U);                          // LNA gain to max, LNA boost enable
    writeReg(self, RegPayloadMaxLength, settings->max);     // max payload
    writeReg(self, RegInvertIQ, U8(0x40U + 0x27U));             // invert IQ
    writeReg(self, RegDioMapping1, self->dio_mapping1);     // DIO0 (RX_TIMEOUT) DIO1 (RX_DONE)
    writeReg(self, RegIrqFlags, 0xffU);                      // clear all interrupts
    writeReg(self, RegIrqFlagsMask, 0x3fU);                 // unmask RX_TIMEOUT and RX_DONE interrupt
    writeReg(self, RegFifoAddrPtr, 0U);

    setFreq(self, settings->freq);                          // set carrier frequency

    /* read everything back for debug */
#ifdef LDL_ENABLE_RADIO_DEBUG
    (void)readReg(self, RegModemConfig1);
    (void)readReg(self, RegModemConfig2);
#ifdef LDL_ENABLE_SX1276
    if(self->type == LDL_RADIO_SX1276){
        (void)readReg(self, RegModemConfig3);
    }
#endif
    (void)readReg(self, RegSymbTimeoutLsb);
    (void)readReg(self, RegSyncWord);
    (void)readReg(self, RegLna);
    (void)readReg(self, RegPayloadMaxLength);
    (void)readReg(self, RegInvertIQ);
    (void)readReg(self, RegDioMapping1);
    (void)readReg(self, RegIrqFlags);
    (void)readReg(self, RegIrqFlagsMask);
    (void)readReg(self, RegFifoAddrPtr);
    (void)readReg(self, RegFrfMsb);
    (void)readReg(self, RegFrfMid);
    (void)readReg(self, RegFrfLsb);
#endif

    setOpRXSingle(self);                                    // single RX

#ifdef LDL_ENABLE_RADIO_DEBUG
    LDL_Radio_debugLogFlush(self, __FUNCTION__);
#endif
}

uint8_t LDL_Radio_readBuffer(struct ldl_radio *self, struct ldl_radio_packet_metadata *meta, void *data, uint8_t max)
{
    LDL_PEDANTIC(self != NULL)
    LDL_PEDANTIC((data != NULL) || (max == 0U))
    LDL_PEDANTIC(self->mode == LDL_RADIO_MODE_STANDBY)

    uint8_t retval;

#ifdef LDL_ENABLE_RADIO_DEBUG
    LDL_Radio_debugLogReset(self);

    rxDiagnostics(self);
#endif

    retval = readFIFO(self, data, max);

    meta->rssi = S16(readReg(self, RegPktRssiValue)) - S16(157);
    meta->snr = S16(readReg(self, RegPktSnrValue)) * S16(100) / S16(4);

#ifdef LDL_ENABLE_RADIO_DEBUG
    LDL_Radio_debugLogFlush(self, __FUNCTION__);
#endif

    return retval;
}

void LDL_Radio_receiveEntropy(struct ldl_radio *self)
{
    LDL_PEDANTIC(self != NULL)
    LDL_PEDANTIC(self->mode == LDL_RADIO_MODE_STANDBY)

#ifdef LDL_ENABLE_RADIO_DEBUG
    LDL_Radio_debugLogReset(self);

    rxDiagnostics(self);
#endif

    self->chip_set_mode(self->chip, LDL_CHIP_MODE_RX);  // configure accessory IO
    writeReg(self, RegIrqFlags, 0xffU);                  // clear all interrupts
    writeReg(self, RegIrqFlagsMask, 0xffU);             // mask all interrupts

    /* application note instructions */
    switch(self->type){
    default:
    case LDL_RADIO_NONE:
        /* do nothing */
        break;
#ifdef LDL_ENABLE_SX1272
    case LDL_RADIO_SX1272:
        writeReg(self, RegModemConfig1, 0x0aU);
        writeReg(self, RegModemConfig2, 0x74U);
        break;
#endif
#ifdef LDL_ENABLE_SX1276
    case LDL_RADIO_SX1276:
        writeReg(self, RegModemConfig1, 0x72U);
        writeReg(self, RegModemConfig2, 0x70U);
        break;
#endif
    }

    setOpRXContinuous(self);                                        // continuous RX

#ifdef LDL_ENABLE_RADIO_DEBUG
    LDL_Radio_debugLogFlush(self, __FUNCTION__);
#endif
}

uint32_t LDL_Radio_readEntropy(struct ldl_radio *self)
{
    size_t i;
    uint32_t retval = 0U;

    LDL_PEDANTIC(self != NULL)
    LDL_PEDANTIC(self->mode == LDL_RADIO_MODE_STANDBY)

#ifdef LDL_ENABLE_RADIO_DEBUG
    LDL_Radio_debugLogReset(self);

    rxDiagnostics(self);
#endif

    /* sample wideband RSSI */
    for(i=0U; i < (sizeof(retval) << 3); i++){

        retval <<= 1;
        retval |= (U32(readReg(self, RegRssiWideband)) & 1U);
    }

    setOpStandby(self);                                     // standby
    self->chip_set_mode(self->chip, LDL_CHIP_MODE_STANDBY); // configure accessory IO
    writeReg(self, RegIrqFlags, 0xffU);                      // clear all interrupts
    writeReg(self, RegIrqFlagsMask, 0xffU);                 // mask all interrupts

#ifdef LDL_ENABLE_RADIO_DEBUG
    LDL_Radio_debugLogFlush(self, __FUNCTION__);
#endif

    return retval;
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

    Npayload = U32(8) + ((((numerator / denom) + (((numerator % denom) != 0U) ? U32(1) : U32(0))) * (U32(CR_5) + U32(4))));

    Tpayload = Npayload * Ts;

    Tpacket = Tpreamble + Tpayload;

    /* convert to us to ms and overestimate */
    return (Tpacket / U32(1000)) + U32(1);
}

uint8_t LDL_Radio_getXTALDelay(struct ldl_radio *self)
{
    LDL_PEDANTIC(self != NULL)

    return self->xtal_delay;
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

const char *LDL_Radio_typeToString(enum ldl_radio_type type)
{
    const char *retval;

    switch(type){
    default:
        retval = "undefined";
        break;
    case LDL_RADIO_NONE:
        retval = "none";
        break;
#ifdef LDL_ENABLE_SX1272
    case LDL_RADIO_SX1272:
        retval = "SX1272";
        break;
#endif
#ifdef LDL_ENABLE_SX1276
    case LDL_RADIO_SX1276:
        retval = "SX1276";
        break;
#endif
    }

    return retval;
}

const char *LDL_Radio_modeToString(enum ldl_radio_mode mode)
{
    const char *retval;

    switch(mode){
    default:
        retval = "undefined";
        break;
    case LDL_RADIO_MODE_RESET:
        retval = "RESET";
        break;
    case LDL_RADIO_MODE_SLEEP:
        retval = "SLEEP";
        break;
    case LDL_RADIO_MODE_STANDBY:
        retval = "STANDBY";
        break;
    }

    return retval;
}

const char *LDL_Radio_xtalToString(enum ldl_radio_xtal xtal)
{
    const char *retval;

    switch(xtal){
    default:
        retval = "undefined";
        break;
    case LDL_RADIO_XTAL_CRYSTAL:
        retval = "CRYSTAL";
        break;
    case LDL_RADIO_XTAL_TCXO:
        retval = "TCXO";
        break;
    }

    return retval;
}

const char *LDL_Radio_paToString(enum ldl_radio_pa pa)
{
    const char *retval;

    switch(pa){
    default:
        retval = "undefined";
        break;
    case LDL_RADIO_PA_AUTO:
        retval = "AUTO";
        break;
    case LDL_RADIO_PA_BOOST:
        retval = "BOOST";
        break;
    case LDL_RADIO_PA_RFO:
        retval = "RFO";
        break;
    }

    return retval;
}

/* static functions ***************************************************/

static enum ldl_radio_event interruptToEvent(struct ldl_radio *self, uint8_t n)
{
    LDL_PEDANTIC(self != NULL)

    enum ldl_radio_event retval = LDL_RADIO_EVENT_NONE;

    switch(n){
    case 0U:

        switch(self->dio_mapping1){
        case 0U:
            retval = LDL_RADIO_EVENT_RX_READY;
            break;
        case 0x40U:
            retval = LDL_RADIO_EVENT_TX_COMPLETE;
            break;
        default:
            /* do nothing */
            break;
        }
        break;

    case 1U:

        switch(self->dio_mapping1){
        case 0U:
            retval = LDL_RADIO_EVENT_RX_TIMEOUT;
            break;
        default:
            /* do nothing */
            break;
        }
        break;

    default:
        /* do nothing */
        break;
    }

    return retval;
}

static void setOpSleep(struct ldl_radio *self)
{
    writeReg(self, RegOpMode, 0x80U);
    writeReg(self, RegOpMode, 0x80U);
}

static void setOpStandby(struct ldl_radio *self)
{
    writeReg(self, RegOpMode, 0x81U);   // standby
}

static void setOpTX(struct ldl_radio *self)
{
    writeReg(self, RegOpMode, 0x83U);   // tx
}

static void setOpRXContinuous(struct ldl_radio *self)
{
    writeReg(self, RegOpMode, 0x85U);   // rx continuous
}

static void setOpRXSingle(struct ldl_radio *self)
{
    writeReg(self, RegOpMode, 0x86U);   // rx single
}

static void setPower(struct ldl_radio *self, int16_t dbm)
{
    /* Todo:
     *
     * - review current limiting
     *
     * */
    enum ldl_radio_pa pa = self->pa;

    /* compensate for transmitter */
    dbm += self->tx_gain;

    dbm /= S16(100);

    switch(self->type){
    default:
    case LDL_RADIO_NONE:
        /* do nothing */
        break;

#ifdef LDL_ENABLE_SX1272
    case LDL_RADIO_SX1272:
    {
        uint8_t paConfig;
        uint8_t paDac;

        if(pa == LDL_RADIO_PA_AUTO){

            pa = (dbm <= 14) ? LDL_RADIO_PA_RFO : LDL_RADIO_PA_BOOST;
        }

        switch(pa){
        default:
        case LDL_RADIO_PA_RFO:

            self->chip_set_mode(self->chip, LDL_CHIP_MODE_TX_RFO);

            paDac = 0x84U;

            /* Pout = -1 + (0..15) */
            if(dbm >= 14){

                paConfig = 0x0fU;
            }
            else if(dbm >= -1){

                paConfig = (U8(dbm) + 1U) & 0xfU;
            }
            else{

                paConfig = 0U;
            }

            writeReg(self, RegPaConfig, paConfig);
            writeReg(self, SX1272RegPaDac, paDac);
            break;

        case LDL_RADIO_PA_BOOST:

            self->chip_set_mode(self->chip, LDL_CHIP_MODE_TX_BOOST);

            paConfig = 0x80U;

            /* fixed 20dbm */
            if(dbm >= 20){

                paDac = 0x87U;
                paConfig |= 0x0fU;
            }
            /* 2 dbm to 17dbm */
            else{

                paDac = 0x84U;

                if(dbm > 17){

                    paConfig |= 0xfU;
                }
                else if(dbm < 2){

                    paConfig |= 0U;
                }
                else{

                    paConfig |= (U8(dbm) - 2U);
                }
            }

            writeReg(self, RegPaConfig, paConfig);
            writeReg(self, SX1272RegPaDac, paDac);
            break;
        }
#ifdef LDL_ENABLE_RADIO_DEBUG
        (void)readReg(self, RegPaConfig);
        (void)readReg(self, SX1272RegPaDac);
#endif
    }
        break;
#endif
#ifdef LDL_ENABLE_SX1276
    case LDL_RADIO_SX1276:
    {
        uint8_t paConfig;
        uint8_t paDac;

        if(pa == LDL_RADIO_PA_AUTO){

            pa = (dbm <= 14) ? LDL_RADIO_PA_RFO : LDL_RADIO_PA_BOOST;
        }

        switch(pa){
        default:
        /* -4dbm to 14dbm */
        case LDL_RADIO_PA_RFO:

            self->chip_set_mode(self->chip, LDL_CHIP_MODE_TX_RFO);

            paDac = 0x84U;

            /* PaConfig = PaSelect(1) | MaxPower(3) | OutputPower(4)
             *
             * Pmax = 10.8 + (0.6 * MaxPower)
             *
             * Pout = Pmax - (15 - OutputPower)
             *
             *
             * */
            if(dbm >= 14){

                /* Pmax = 10.8 + 0.6 * 5 = 13.8 dBm */
                paConfig = 0x5fU;
            }
            else if(dbm == 13){

                /* Pmax = (10.8 + 0.6 * 5) = 13.8 dBm
                 *
                 * Pout = 13.8 - (15 - 14) = 12.8 dBm
                 *
                 * */
                paConfig = 0x5eU;
            }
            else if(dbm >= -3){

                /* Pmax = (10.8 + 0.6 * 2) = 12 dBm
                 *
                 * Pout = 12 - (15 - OutputPower) = -3 .. 12 dBm
                 *
                 * */
                paConfig = 0x20U | (U8(dbm) + 3U);
            }
            else{

                /* Pmax = 10.8 dBm
                 *
                 * Pout = 11.4 - 15 = -3.6
                 *
                 * */
                paConfig = 0x10U;
            }

            writeReg(self, RegPaConfig, paConfig);
            writeReg(self, SX1276RegPaDac, paDac);
            break;

        /* 2dBm to 17dBm
         * or
         * 20dBm fixed */
        case LDL_RADIO_PA_BOOST:

            self->chip_set_mode(self->chip, LDL_CHIP_MODE_TX_BOOST);

            /* PaConfig = PaSelect(1) | MaxPower(3) | OutputPower(4) */
            paConfig = U8(0x80) | U8(0x40);

            /* fixed 20dbm */
            if(dbm >= 20){

                paDac = 0x87U;
                paConfig |= 0xfU;
            }
            /* 2 dbm to 17dbm */
            else{

                paDac = 0x84U;

                if(dbm > 17){

                    paConfig |= 0xfU;
                }
                else if(dbm < 2){

                    paConfig |= 0x40U;
                }
                else{

                    paConfig |= (U8(dbm) - 2U);
                }
            }

            writeReg(self, RegPaConfig, paConfig);
            writeReg(self, SX1276RegPaDac, paDac);
            break;
        }

#ifdef LDL_ENABLE_RADIO_DEBUG
        (void)readReg(self, RegPaConfig);
        (void)readReg(self, SX1276RegPaDac);
#endif
    }
        break;
#endif
    }
}

static void setModemConfig1(struct ldl_radio *self, const struct modem_config *config)
{
    switch(self->type){
    default:
    case LDL_RADIO_NONE:
        /* do nothing */
        break;
#ifdef LDL_ENABLE_SX1272
    case LDL_RADIO_SX1272:
    {
        bool low_rate = (config->bw == LDL_BW_125) && ((config->sf == LDL_SF_11) || (config->sf == LDL_SF_12));
        uint8_t bw;

        switch(config->bw){
        default:
        case LDL_BW_125:
            bw = 0x00U;
            break;
        case LDL_BW_250:
            bw = 0x40U;
            break;
        case LDL_BW_500:
            bw = 0x80U;
            break;
        }

        /* bandwidth            (2bit)
         * codingRate           (3bit) (CR_5)
         * implicitHeaderModeOn (1bit) (0)
         * rxPayloadCrcOn       (1bit) (1)
         * lowDataRateOptimize  (1bit)      */
        writeReg(self, RegModemConfig1, bw | 8U | 0U | (config->crc ? 2U : 0U) | (low_rate ? 1U : 0U));
    }
        break;
#endif
#ifdef LDL_ENABLE_SX1276
    case LDL_RADIO_SX1276:
    {
        uint8_t bw;

        switch(config->bw){
        default:
        case LDL_BW_125:
            bw = 0x70U;
            break;
        case LDL_BW_250:
            bw = 0x80U;
            break;
        case LDL_BW_500:
            bw = 0x90U;
            break;
        }

        /* bandwidth            (4bit)
         * codingRate           (3bit) (CR_5)
         * implicitHeaderModeOn (1bit) (0)  */
        writeReg(self, RegModemConfig1, bw | 2U | 0U);
    }
        break;
#endif
    }
}

static void setModemConfig2(struct ldl_radio *self, const struct modem_config *config)
{
    uint8_t sf = ((uint8_t)config->sf & 0xfU) << 4;

    switch(self->type){
    default:
    case LDL_RADIO_NONE:
        /* do nothing */
        break;
#ifdef LDL_ENABLE_SX1272
    case LDL_RADIO_SX1272:

        /* spreadingFactor      (4bit)
         * txContinuousMode     (1bit) (0)
         * agcAutoOn            (1bit) (1)
         * symbTimeout(9:8)     (2bit) (0)  */
        writeReg(self, RegModemConfig2, sf | 0U | 4U | (U8(config->timeout >> 8) & 3U));
        break;
#endif
#ifdef LDL_ENABLE_SX1276
    case LDL_RADIO_SX1276:

        /* spreadingFactor      (4bit)
         * txContinuousMode     (1bit) (0)
         * rxPayloadCrcOn       (1bit) (1)
         * symbTimeout(9:8)     (2bit) (0)  */
        writeReg(self, RegModemConfig2, sf | 0U | (config->crc ? 4U : 0U) | 0U | (U8(config->timeout >> 8) & 3U));
        break;
#endif
    }
}

static void setModemConfig3(struct ldl_radio *self, const struct modem_config *config)
{
#ifdef LDL_ENABLE_SX1276
    if(self->type == LDL_RADIO_SX1276){

        bool low_rate = (config->bw == LDL_BW_125) && ((config->sf == LDL_SF_11) || (config->sf == LDL_SF_12));

        /* unused               (4bit) (0)
         * lowDataRateOptimize  (1bit)
         * agcAutoOn            (1bit) (1)
         * unused               (2bit) (0)  */
        writeReg(self, RegModemConfig3, 0U | (low_rate ? 8U : 0U) | 4U | 0U);
    }
#endif
}

static void setFreq(struct ldl_radio *self, uint32_t freq)
{
    uint32_t f = (uint32_t)(((uint64_t)freq << 19U) / 32000000UL);

    writeReg(self, RegFrfMsb, U8(f >> 16));
    writeReg(self, RegFrfMid, U8(f >> 8));
    writeReg(self, RegFrfLsb, U8(f));
}

static uint8_t readFIFO(struct ldl_radio *self, uint8_t *data, uint8_t max)
{
    uint8_t size = readReg(self, RegRxNbBytes);

    size = (size > max) ? max : size;

    if(size > 0U){

        writeReg(self, RegFifoAddrPtr, 0U);     // this driver always puts packets at address 0

        burstRead(self, RegFifo, data, size);
    }

    return size;
}

static uint8_t readReg(struct ldl_radio *self, enum ldl_radio_sx1272_sx1276_register reg)
{
    uint8_t data;
    uint8_t opcode = U8(reg) & 0x7FU;

    self->chip_read(self->chip, opcode, &data, U8(sizeof(data)));

#ifdef LDL_ENABLE_RADIO_DEBUG
    LDL_Radio_debugLogPush(self, opcode, &data, sizeof(data));
#endif

    return data;
}

static void burstRead(struct ldl_radio *self, enum ldl_radio_sx1272_sx1276_register reg, uint8_t *data, uint8_t len)
{
    uint8_t opcode = U8(reg) & 0x7FU;

    self->chip_read(self->chip, opcode, data, len);

#ifdef LDL_ENABLE_RADIO_DEBUG
    LDL_Radio_debugLogPush(self, opcode, data, len);
#endif
}

static void writeReg(struct ldl_radio *self, enum ldl_radio_sx1272_sx1276_register reg, uint8_t data)
{
    uint8_t opcode = U8(reg) | 0x80U;

    self->chip_write(self->chip, opcode, &data, 1U);

#ifdef LDL_ENABLE_RADIO_DEBUG
    LDL_Radio_debugLogPush(self, opcode, &data, sizeof(data));
#endif
}

static void burstWrite(struct ldl_radio *self, enum ldl_radio_sx1272_sx1276_register reg, const uint8_t *data, uint8_t len)
{
    uint8_t opcode = U8(reg) | 0x80U;

    self->chip_write(self->chip, opcode | 0x80U, data, len);

#ifdef LDL_ENABLE_RADIO_DEBUG
    LDL_Radio_debugLogPush(self, opcode, data, len);
#endif
}

static void setXTALReg(struct ldl_radio *self, enum ldl_radio_sx1272_sx1276_register reg)
{
    uint8_t value = readReg(self, reg);

    if((value & 0x10U) > 0U){

        if(self->xtal == LDL_RADIO_XTAL_CRYSTAL){

            writeReg(self, reg, value & ~(0x10U));
        }
    }
    else{

        if(self->xtal == LDL_RADIO_XTAL_TCXO){

            writeReg(self, reg, value | 0x10U);
        }
    }
}

static void setXTAL(struct ldl_radio *self)
{
    switch(self->type){
    default:
    case LDL_RADIO_NONE:
        /* do nothing */
        break;
#ifdef LDL_ENABLE_SX1272
    case LDL_RADIO_SX1272:
        setXTALReg(self, SX1272RegTcxo);
        break;
#endif
#ifdef LDL_ENABLE_SX1276
    case LDL_RADIO_SX1276:
        setXTALReg(self, SX1276RegTcxo);
        break;
#endif
    }
}

#ifdef LDL_ENABLE_RADIO_DEBUG
static void rxDiagnostics(struct ldl_radio *self)
{
    (void)readReg(self, RegOpMode);
    (void)readReg(self, RegIrqFlags);
    (void)readReg(self, RegModemStat);
    (void)readReg(self, RegHopChannel);             // PLL locked and CRC status
    (void)readReg(self, RegRxHeaderCntValueMsb);    // number of headers rx since last sleep
    (void)readReg(self, RegRxHeaderCntValueLsb);
    (void)readReg(self, RegRxPacketCntValueMsb);    // number of packets rx since last sleep
    (void)readReg(self, RegRxPacketCntValueLsb);
    (void)readReg(self, RegRxNbBytes);              // number of bytes in last rx packet
    (void)readReg(self, LoraRegFeiMsb);
    (void)readReg(self, RegFeiMid);
    (void)readReg(self, LoraRegFeiLsb);
}
#endif

#endif
