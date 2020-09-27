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
#include "ldl_platform.h"
#include "ldl_system.h"

#if defined(LDL_ENABLE_SX1272) || defined(LDL_ENABLE_SX1276)

enum ldl_radio_sx1272_register {
    RegFifo=0x00,
    RegOpMode=0x01,
    RegBitrateMsb=0x02,
    RegBitrateLsb=0x03,
    RegFdevMsb=0x04,
    RegFdevLsb=0x05,
    RegFrfMsb=0x06,
    RegFrfMid=0x07,
    RegFrfLsb=0x08,
    RegPaConfig=0x09,
    RegPaRamp=0x0A,
    RegOcp=0x0B,
    RegLna=0x0C,
    RegRxConfig=0x0D,
    RegFifoAddrPtr=0x0D,
    RegRssiConfig=0x0E,
    RegFifoTxBaseAddr=0x0E,
    RegRssiCollision=0x0F,
    RegFifoRxBaseAddr=0x0F,
    RegRssiThresh=0x10,
    RegFifoRxCurrentAddr=0x10,
    RegRssiValue=0x11,
    RegIrqFlagsMask=0x11,
    RegRxBw=0x12,
    RegIrqFlags=0x12,
    RegAfcBw=0x13,
    RegRxNbBytes=0x13,
    RegOokPeak=0x14,
    RegRxHeaderCntValueMsb=0x14,
    RegOokFix=0x15,
    RegRxHeaderCntValueLsb=0x15,
    RegOokAvg=0x16,
    RegRxPacketCntValueMsb=0x16,
    RegRxpacketCntValueLsb=0x17,
    RegModemStat=0x18,
    RegPktSnrValue=0x19,
    RegAfcFei=0x1A,
    RegPktRssiValue=0x1A,
    RegAfcMsb=0x1B,
    LoraRegRssiValue=0x1B,
    RegAfcLsb=0x1C,
    RegHopChannel=0x1C,
    RegFeiMsb=0x1D,
    RegModemConfig1=0x1D,
    RegFeiLsb=0x1E,
    RegModemConfig2=0x1E,
    RegPreambleDetect=0x1F,
    RegSymbTimeoutLsb=0x1F,
    RegRxTimeout1=0x20,
    LoraRegPreambleMsb=0x20,
    RegRxTimeout2=0x21,
    LoraRegPreambleLsb=0x21,
    RegRxTimeout3=0x22,
    LoraRegPayloadLength=0x22,
    RegRxDelay=0x23,
    RegPayloadMaxLength=0x23,
    RegOsc=0x24,
    RegHopPeriod=0x24,
    RegPreambleMsb=0x25,
    RegFifoRxByteAddr=0x25,
    RegModemConfig3=0x26,
    RegPreambleLsb=0x26,
    RegSyncConfig=0x27,
    LoraRegFeiMsb=0x28,
    RegSyncValue1=0x28,
    LoraFeiMib=0x29,
    RegSyncValue2=0x29,
    LoraRegFeiLsb=0x2A,
    RegSyncValue3=0x2A,
    RegSyncValue4=0x2B,
    RegRssiWideband=0x2C,
    RegSyncValue5=0x2C,
    RegSyncValue6=0x2D,
    RegSyncValue7=0x2E,
    RegSyncValue8=0x2F,
    RegPacketConfig1=0x30,
    RegPacketConfig2=0x31,
    RegDetectOptimize=0x31,
    RegPayloadLength=0x32,
    RegNodeAdrs=0x33,
    RegInvertIQ=0x33,
    RegBroadcastAdrs=0x34,
    RegFifoThresh=0x35,
    RegSeqConfig1=0x36,
    RegSeqConfig2=0x37,
    RegDetectionThreshold=0x37,
    RegTimerResol=0x38,
    RegTimer1Coef=0x39,
    RegSyncWord=0x39,
    RegTimer2Coef=0x3A,
    RegImageCal=0x3B,
    RegTemp=0x3C,
    RegLowBat=0x3D,
    RegIrqFlags1=0x3E,
    RegIrqFlags2=0x3F,
    RegDioMapping1=0x40,
    RegDioMapping2=0x41,
    RegVersion=0x42,
    RegAgcRef=0x43,
    RegAgcThresh1=0x44,
    RegAgcThresh2=0x45,
    RegAgcThresh3=0x46,
    RegPllHop=0x4B,
    RegTcxo=0x4B,
    RegPaDac=0x5A,
    RegPll=0x5C,
    RegPllLowPn=0x5E,
    RegFormerTemp=0x6C,
    RegBitRateFrac=0x70
};

const struct ldl_radio_interface LDL_Radio_interface = {

    .set_mode = LDL_Radio_setMode,
    .read_entropy = LDL_Radio_readEntropy,
    .read_buffer = LDL_Radio_readBuffer,
    .transmit = LDL_Radio_transmit,
    .receive = LDL_Radio_receive,
    .get_xtal_delay = LDL_Radio_getXTALDelay,
    .receive_entropy = LDL_Radio_receiveEntropy
};

/* static function prototypes *****************************************/

static enum ldl_radio_event interruptToEvent(struct ldl_radio *self, uint8_t n);
static uint8_t readFIFO(struct ldl_radio *self, uint8_t *data, uint8_t max);
static void writeFIFO(struct ldl_radio *self, const uint8_t *data, uint8_t len);
static void setFreq(struct ldl_radio *self, uint32_t freq);
static void setModemConfig(struct ldl_radio *self, enum ldl_signal_bandwidth bw, enum ldl_spreading_factor sf, uint32_t timeout);
static void setPower(struct ldl_radio *self, int16_t dbm);
static uint8_t readReg(struct ldl_radio *self, uint8_t reg);
static void writeReg(struct ldl_radio *self, uint8_t reg, uint8_t data);
static void burstRead(struct ldl_radio *self, uint8_t reg, uint8_t *data, uint8_t len);
static void burstWrite(struct ldl_radio *self, uint8_t reg, const uint8_t *data, uint8_t len);
static uint8_t getOpMode(struct ldl_radio *self);
static void setOpRXSingle(struct ldl_radio *self);
static void setOpTX(struct ldl_radio *self);
static void setOpRXContinuous(struct ldl_radio *self);
static void setOpStandby(struct ldl_radio *self);
static void setOpSleep(struct ldl_radio *self);
static uint8_t crSetting(const struct ldl_radio *self, enum ldl_coding_rate cr);
static uint8_t bwSetting(const struct ldl_radio *self, enum ldl_signal_bandwidth bw);
static uint8_t sfSetting(const struct ldl_radio *self, enum ldl_spreading_factor sf);

/* functions **********************************************************/

void LDL_Radio_init(struct ldl_radio *self, const struct ldl_radio_init_arg *arg)
{
    LDL_PEDANTIC(self != NULL)
    LDL_PEDANTIC(arg != NULL)

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
}

void LDL_Radio_setEventCallback(struct ldl_radio *self, void *ctx, ldl_radio_event_fn cb)
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

    LDL_DEBUG(NULL, "setMode(%u) from %u", mode, self->mode)

    switch(mode){
    case LDL_RADIO_MODE_RESET:

        self->chip_set_mode(self->chip, LDL_CHIP_MODE_RESET);
        break;

    case LDL_RADIO_MODE_SLEEP:

        switch(self->mode){
        case LDL_RADIO_MODE_RESET:

            self->chip_set_mode(self->chip, LDL_CHIP_MODE_SLEEP);
            /* do not write to registers coming out of reset! */
            break;

        case LDL_RADIO_MODE_SLEEP:
            setOpSleep(self);
            break;

        case LDL_RADIO_MODE_STANDBY:

            setOpStandby(self);
            writeReg(self, RegIrqFlags, 0xff);
            writeReg(self, RegIrqFlagsMask, 0xffU);
            self->chip_set_mode(self->chip, LDL_CHIP_MODE_SLEEP);
            setOpSleep(self);
        }
        break;

    case LDL_RADIO_MODE_STANDBY:

        /* you will have timing problems shifting
         * from reset to standby. This code only
         * exists to make the chip interface behave properly */
        if(self->mode == LDL_RADIO_MODE_RESET){

            LDL_Radio_setMode(self, LDL_RADIO_MODE_SLEEP);
        }

        switch(self->mode){
        case LDL_RADIO_MODE_RESET:
            break;
        case LDL_RADIO_MODE_SLEEP:

            self->chip_set_mode(self->chip, LDL_CHIP_MODE_STANDBY);
            setOpSleep(self);

            if(self->xtal == LDL_RADIO_XTAL_TCXO){

                writeReg(self, RegTcxo, readReg(self, RegTcxo) | 0x10U);
            }
            else{

                writeReg(self, RegTcxo, readReg(self, RegTcxo) & ~(0x10U));
            }

            setOpStandby(self);
            break;

        case LDL_RADIO_MODE_STANDBY:

            self->chip_set_mode(self->chip, LDL_CHIP_MODE_STANDBY);
            setOpStandby(self);
            writeReg(self, RegIrqFlags, 0xff);
            writeReg(self, RegIrqFlagsMask, 0xffU);
            break;
        }
        break;
    }

    self->mode = mode;
}

void LDL_Radio_transmit(struct ldl_radio *self, const struct ldl_radio_tx_setting *settings, const void *data, uint8_t len)
{
    LDL_PEDANTIC(self != NULL)
    LDL_PEDANTIC(settings != NULL)
    LDL_PEDANTIC((data != NULL) || (len == 0U))
    LDL_PEDANTIC(settings->freq != 0U)

    self->dio_mapping1 = 0x40U;

    writeReg(self, RegIrqFlags, 0xff);
    setPower(self, settings->dbm);                                          // configure PA and accessory IO
    setModemConfig(self, settings->bw, settings->sf, 0U);                   // configure LoRa modem
    setFreq(self, settings->freq);                                          // set frequency
    writeReg(self, RegSyncWord, 0x34);                                      // set sync word
    writeReg(self, RegPaRamp, (readReg(self, RegPaRamp) & 0xf0U) | 0x08U);  // 50us PA ramp
    writeReg(self, RegInvertIQ, 0x27U);                                     // non-invert IQ
    writeReg(self, RegDioMapping1, self->dio_mapping1);                     // DIO0 (TX_COMPLETE) DIO1 (RX_DONE)
    writeReg(self, RegIrqFlagsMask, 0xf7U);                                 // unmask TX_DONE interrupt
    writeFIFO(self, data, len);                                             // write in the data
    setOpTX(self);                                                          // TX
}

void LDL_Radio_receive(struct ldl_radio *self, const struct ldl_radio_rx_setting *settings)
{
    LDL_PEDANTIC(self != NULL)
    LDL_PEDANTIC(settings != NULL)
    LDL_PEDANTIC(settings->freq != 0U)

    self->dio_mapping1 = 0U;

    writeReg(self, RegIrqFlags, 0xff);                                  // clear all interrupts
    self->chip_set_mode(self->chip, LDL_CHIP_MODE_RX);                  // configure accessory IO
    setModemConfig(self, settings->bw, settings->sf, settings->timeout);// configure LoRa modem
    setFreq(self, settings->freq);                                      // set carrier frequency
    writeReg(self, RegSyncWord, 0x34);                                  // set sync word
    writeReg(self, RegLna, 0x23U);                                      // LNA gain to max, LNA boost enable
    writeReg(self, RegPayloadMaxLength, 0xffU);                         // max payload
    writeReg(self, RegInvertIQ, 0x40U + 0x27U);                         // invert IQ
    writeReg(self, RegDioMapping1, self->dio_mapping1);                 // DIO0 (RX_TIMEOUT) DIO1 (RX_DONE)
    writeReg(self, RegIrqFlagsMask, 0x3fU);                             // unmask RX_TIMEOUT and RX_DONE interrupt

    writeReg(self, RegFifoRxBaseAddr, 0U);
    writeReg(self, RegFifoAddrPtr, 0U);

    setOpRXSingle(self);                                                // single RX
}

uint8_t LDL_Radio_readBuffer(struct ldl_radio *self, struct ldl_radio_packet_metadata *meta, void *data, uint8_t max)
{
    LDL_PEDANTIC(self != NULL)
    LDL_PEDANTIC((data != NULL) || (max == 0U))

    uint8_t retval;

    retval = readFIFO(self, data, max);
    meta->rssi = (int16_t)readReg(self, RegPktRssiValue) - 157;
    meta->snr = ((int16_t)readReg(self, RegPktSnrValue)) * 100 / 4;

    return retval;
}

void LDL_Radio_receiveEntropy(struct ldl_radio *self)
{
    LDL_PEDANTIC(self != NULL)

    self->chip_set_mode(self->chip, LDL_CHIP_MODE_RX);   // configure accessory IO
    writeReg(self, RegIrqFlags, 0xff);                              // clear all interrupts
    writeReg(self, RegIrqFlagsMask, 0xffU);                         // mask all interrupts

    /* application note instructions */
    switch(self->type){
    default:
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
}

unsigned int LDL_Radio_readEntropy(struct ldl_radio *self)
{
    size_t i;
    unsigned int retval = 0U;

    LDL_PEDANTIC(self != NULL)

    /* sample wideband RSSI */
    for(i=0U; i < (sizeof(unsigned int)*8U); i++){

        retval <<= 1;
        retval |= readReg(self, RegRssiWideband) & 0x1U;
    }

    setOpStandby(self);                                                 // standby
    self->chip_set_mode(self->chip, LDL_CHIP_MODE_STANDBY);  // configure accessory IO
    writeReg(self, RegIrqFlags, 0xff);                                  // clear all interrupts
    writeReg(self, RegIrqFlagsMask, 0xffU);                             // mask all interrupts

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

uint32_t LDL_Radio_getAirTime(uint32_t tps, enum ldl_signal_bandwidth bw, enum ldl_spreading_factor sf, uint8_t size, bool crc)
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

    Ts = ((((uint32_t)1UL) << sf) * tps) / LDL_Radio_bwToNumber(bw);
    Tpreamble = (Ts * 12UL) +  (Ts / 4UL);

    numerator = (8UL * (uint32_t)size) - (4UL * (uint32_t)sf) + 28UL + ( crc ? 16UL : 0UL ) - ( header ? 20UL : 0UL );
    denom = 4UL * ((uint32_t)sf - ( lowDataRateOptimize ? 2UL : 0UL ));

    Npayload = 8UL + ((((numerator / denom) + (((numerator % denom) != 0UL) ? 1UL : 0UL)) * ((uint32_t)CR_5 + 4UL)));

    Tpayload = Npayload * Ts;

    Tpacket = Tpreamble + Tpayload;

    return Tpacket;
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
        retval = 125000UL;
        break;
    case LDL_BW_250:
        retval = 250000UL;
        break;
    case LDL_BW_500:
        retval = 500000UL;
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

static uint8_t getOpMode(struct ldl_radio *self)
{
    return readReg(self, RegOpMode) & 0xf8U;
}

static void setOpSleep(struct ldl_radio *self)
{
    writeReg(self, RegOpMode, getOpMode(self));
    writeReg(self, RegOpMode, getOpMode(self) | 0x80U);
}

static void setOpStandby(struct ldl_radio *self)
{
    writeReg(self, RegOpMode, getOpMode(self) | 1U);   // standby
}

static void setOpTX(struct ldl_radio *self)
{
    writeReg(self, RegOpMode, getOpMode(self) | 3U);   // tx
}

static void setOpRXContinuous(struct ldl_radio *self)
{
    writeReg(self, RegOpMode, getOpMode(self) | 5U);   // rx continuous
}

static void setOpRXSingle(struct ldl_radio *self)
{
    writeReg(self, RegOpMode, getOpMode(self) | 6U);   // rx single
}

static void setModemConfig(struct ldl_radio *self, enum ldl_signal_bandwidth bw, enum ldl_spreading_factor sf, uint32_t timeout)
{
    uint16_t _timeout;

    if(timeout < 4U){

        _timeout = 4U;
    }
    else if(timeout > 1023U){

        _timeout = 1023U;
    }
    else{

        _timeout = (uint16_t)timeout;
    }
    LDL_DEBUG(self, "setModemConfig timeout=%u", _timeout);

    bool low_rate = ((bw == LDL_BW_125) && ((sf == LDL_SF_11) || (sf == LDL_SF_12))) ? true : false;

    switch(self->type){
    default:
        break;
#ifdef LDL_ENABLE_SX1272
    case LDL_RADIO_SX1272:
        /* bandwidth            (2bit)
         * codingRate           (3bit)
         * implicitHeaderModeOn (1bit) (0)
         * rxPayloadCrcOn       (1bit) (1)
         * lowDataRateOptimize  (1bit)      */
        writeReg(self, RegModemConfig1, bwSetting(self, bw) | crSetting(self, CR_5) | 0U | 2U | (low_rate ? 1U : 0U));

        /* spreadingFactor      (4bit)
         * txContinuousMode     (1bit) (0)
         * agcAutoOn            (1bit) (1)
         * symbTimeout(9:8)     (2bit) (0)  */
        writeReg(self, RegModemConfig2, sfSetting(self, sf) | 0U | 4U | ((_timeout >> 8) & 3U));

        writeReg(self, RegSymbTimeoutLsb, _timeout);
        break;
#endif
#ifdef LDL_ENABLE_SX1276
    case LDL_RADIO_SX1276:
        /* bandwidth            (4bit)
         * codingRate           (3bit)
         * implicitHeaderModeOn (1bit) (0)  */
        writeReg(self, RegModemConfig1, bwSetting(self, bw) | crSetting(self, CR_5) | 0U);

        /* spreadingFactor      (4bit)
         * txContinuousMode     (1bit) (0)
         * rxPayloadCrcOn       (1bit) (1)
         * symbTimeout(9:8)     (2bit) (0)  */
        writeReg(self, RegModemConfig2, sfSetting(self, sf) | 0U | 4U | 0U | ((_timeout >> 8) & 3U));

        /* unused               (4bit) (0)
         * lowDataRateOptimize  (1bit)
         * agcAutoOn            (1bit) (1)
         * unused               (2bit) (0)  */
        writeReg(self, RegModemConfig3, 0U | (low_rate ? 8U : 0U) | 4U | 0U);

        writeReg(self, RegSymbTimeoutLsb, _timeout);
        break;
#endif
    }

}

static void setPower(struct ldl_radio *self, int16_t dbm)
{
    /* Todo:
     *
     * - adjust current limit trim
     *
     * */
    enum ldl_radio_pa pa = self->pa;

    /* compensate for transmitter */
    dbm += self->tx_gain;

    dbm /= 100;

    switch(self->type){
#ifdef LDL_ENABLE_SX1272
    case LDL_RADIO_SX1272:
    {
        uint8_t paConfig;
        uint8_t paDac;

        paConfig = readReg(self, RegPaConfig);
        paConfig &= ~(0xfU);

        if(pa == LDL_RADIO_PA_AUTO){

            pa = (dbm <= 14) ? LDL_RADIO_PA_RFO : LDL_RADIO_PA_BOOST;
        }

        switch(pa){
        case LDL_RADIO_PA_RFO:

            /* -1 to 14dbm */
            paConfig &= ~(0x80U);
            paConfig |= (dbm > 14) ? 0xf : (uint8_t)( (dbm < -1) ? 0 : (dbm + 1) );

            writeReg(self, RegPaConfig, paConfig);
            self->chip_set_mode(self->chip, LDL_CHIP_MODE_TX_RFO);
            break;

        case LDL_RADIO_PA_BOOST:

            paDac = readReg(self, RegPaDac);

            paConfig |= 0x80U;
            paDac &= ~(7U);

            /* fixed 20dbm */
            if(dbm >= 20){

                paDac |= 7U;
                paConfig |= 0xfU;
            }
            /* 2 dbm to 17dbm */
            else{

                paDac |= 4U;
                paConfig |= (dbm > 17) ? 0xf : ( (dbm < 2) ? 0 : (dbm - 2) );
            }

            writeReg(self, RegPaDac, paDac);
            writeReg(self, RegPaConfig, paConfig);

            self->chip_set_mode(self->chip, LDL_CHIP_MODE_TX_BOOST);
            break;
        default:
            break;
        }
    }
        break;
#endif
#ifdef LDL_ENABLE_SX1276
    case LDL_RADIO_SX1276:
    {
        uint8_t paConfig;
        uint8_t paDac;

        paConfig = readReg(self, RegPaConfig);
        paConfig &= ~(0xfU);

        if(pa == LDL_RADIO_PA_AUTO){

            pa = (dbm <= 14) ? LDL_RADIO_PA_RFO : LDL_RADIO_PA_BOOST;
        }

        switch(pa){
        case LDL_RADIO_PA_RFO:

            /* todo */
            paConfig |= 0x7eU;
            writeReg(self, RegPaConfig, paConfig);
            self->chip_set_mode(self->chip, LDL_CHIP_MODE_TX_RFO);
            break;

        case LDL_RADIO_PA_BOOST:

            /* regpadac address == 0x4d */
            paDac = readReg(self, 0x4d);

            paConfig |= 0x80U;
            paConfig &= ~0x70U;
            paDac &= ~(7U);

            /* fixed 20dbm */
            if(dbm >= 20){

                paDac |= 7U;
                paConfig |= 0xfU;
            }
            /* 2 dbm to 17dbm */
            else{

                paDac |= 4U;
                paConfig |= (dbm > 17) ? 0xf : ( (dbm < 2) ? 0 : (dbm - 2) );
            }

            /* regpadac address == 0x4d */
            writeReg(self, 0x4d, paDac);
            writeReg(self, RegPaConfig, paConfig);
            self->chip_set_mode(self->chip, LDL_CHIP_MODE_TX_BOOST);
            break;
        default:
            break;
        }
    }
        break;
#endif
    default:
        break;
    }
}

static uint8_t bwSetting(const struct ldl_radio *self, enum ldl_signal_bandwidth bw)
{
    uint8_t retval = 0U;

    switch(self->type){
    default:
        break;
#ifdef LDL_ENABLE_SX1272
    case LDL_RADIO_SX1272:
        switch(bw){
        default:
        case LDL_BW_125:
            retval = 0x00U;
            break;
        case LDL_BW_250:
            retval = 0x40U;
            break;
        case LDL_BW_500:
            retval = 0x80U;
            break;
        }
        break;
#endif
#ifdef LDL_ENABLE_SX1276
    case LDL_RADIO_SX1276:
        switch(bw){
        default:
        case LDL_BW_125:
            retval = 0x70U;
            break;
        case LDL_BW_250:
            retval = 0x80U;
            break;
        case LDL_BW_500:
            retval = 0x90U;
            break;
        }
        break;
#endif
    }

    return retval;
}

static uint8_t sfSetting(const struct ldl_radio *self, enum ldl_spreading_factor sf)
{
    uint8_t retval = 0U;

    (void)self;

    switch(sf){
    default:
    case LDL_SF_7:
        retval = 0x70U;
        break;
    case LDL_SF_8:
        retval = 0x80U;
        break;
    case LDL_SF_9:
        retval = 0x90U;
        break;
    case LDL_SF_10:
        retval = 0xa0U;
        break;
    case LDL_SF_11:
        retval = 0xb0U;
        break;
    case LDL_SF_12:
        retval = 0xc0U;
        break;
    }
    return retval;
}

static uint8_t crSetting(const struct ldl_radio *self, enum ldl_coding_rate cr)
{
    uint8_t retval = 0U;

    (void)cr;

    switch(self->type){
    default:
        break;
#ifdef LDL_ENABLE_SX1272
    case LDL_RADIO_SX1272:
        retval = 8U;    // CR_5
        break;
#endif
#ifdef LDL_ENABLE_SX1276
    case LDL_RADIO_SX1276:
        retval = 2U;    // CR_5
        break;
#endif
    }

    return retval;
}

static void setFreq(struct ldl_radio *self, uint32_t freq)
{
    uint32_t f = (uint32_t)(((uint64_t)freq << 19U) / 32000000UL);

    writeReg(self, RegFrfMsb, f >> 16);
    writeReg(self, RegFrfMid, f >> 8);
    writeReg(self, RegFrfLsb, f);
}

static uint8_t readFIFO(struct ldl_radio *self, uint8_t *data, uint8_t max)
{
    uint8_t size = readReg(self, RegRxNbBytes);

    size = (size > max) ? max : size;

    if(size > 0U){

        writeReg(self, RegFifoAddrPtr, readReg(self, RegFifoRxCurrentAddr));

        burstRead(self, RegFifo, data, size);
    }

    return size;
}

static void writeFIFO(struct ldl_radio *self, const uint8_t *data, uint8_t len)
{
    writeReg(self, RegFifoTxBaseAddr, 0x00U);    // set tx base
    writeReg(self, RegFifoAddrPtr, 0x00U);       // set address pointer
    writeReg(self, LoraRegPayloadLength, len);
    burstWrite(self, RegFifo, data, len);        // write into fifo
}

static uint8_t readReg(struct ldl_radio *self, uint8_t reg)
{
    uint8_t data;
    self->chip_read(self->chip, reg, &data, sizeof(data));
    return data;
}

static void writeReg(struct ldl_radio *self, uint8_t reg, uint8_t data)
{
    self->chip_write(self->chip, reg, &data, sizeof(data));
}

static void burstWrite(struct ldl_radio *self, uint8_t reg, const uint8_t *data, uint8_t len)
{
    self->chip_write(self->chip, reg, data, len);
}

static void burstRead(struct ldl_radio *self, uint8_t reg, uint8_t *data, uint8_t len)
{
    self->chip_read(self->chip, reg, data, len);
}

#endif
