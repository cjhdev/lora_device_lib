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
#include "ldl_sx127x.h"
#include "ldl_system.h"
#include "ldl_internal.h"

#if defined(LDL_ENABLE_SX1272) || defined(LDL_ENABLE_SX1276)

enum ldl_radio_sx1272_sx1276_register {
    RegFifo=0x0,
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
    RegRxPacketCntValueLsb=0x17,
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
    RegFeiMid=0x29,
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
    SX1272RegTcxo=0x58,
    SX1276RegTcxo=0x4B,
    SX1272RegPaDac=0x5A,
    SX1276RegPaDac=0x4D,
    RegPll=0x5C,
    RegPllLowPn=0x5E,
    RegFormerTemp=0x6C,
    RegBitRateFrac=0x70
};

struct modem_config {

    enum ldl_signal_bandwidth bw;
    enum ldl_spreading_factor sf;
    bool crc;
    uint16_t timeout;
};

static const struct ldl_radio_interface interface = {

    .set_mode = LDL_SX127X_setMode,
    .read_entropy = LDL_SX127X_readEntropy,
    .read_buffer = LDL_SX127X_readBuffer,
    .transmit = LDL_SX127X_transmit,
    .receive = LDL_SX127X_receive,
    .receive_entropy = LDL_SX127X_receiveEntropy,
    .get_status = LDL_SX127X_getStatus
};

/* static function prototypes *****************************************/

#ifdef LDL_ENABLE_SX1272
static void SX1272_setPower(struct ldl_radio *self, int16_t dbm);
static void SX1272_setModemConfig1(struct ldl_radio *self, const struct modem_config *config);
static void SX1272_setModemConfig2(struct ldl_radio *self, const struct modem_config *config);
#endif

#ifdef LDL_ENABLE_SX1276
static void SX1276_setPower(struct ldl_radio *self, int16_t dbm);
static void SX1276_setModemConfig1(struct ldl_radio *self, const struct modem_config *config);
static void SX1276_setModemConfig2(struct ldl_radio *self, const struct modem_config *config);
static void SX1276_setModemConfig3(struct ldl_radio *self, const struct modem_config *config);
#endif

static void init_state(struct ldl_radio *self, enum ldl_radio_type type, const struct ldl_sx127x_init_arg *arg);
static uint8_t readFIFO(struct ldl_radio *self, uint8_t *data, uint8_t max);
static void setFreq(struct ldl_radio *self, uint32_t freq);
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


#ifdef LDL_ENABLE_RADIO_DEBUG
static void debugLogReset(struct ldl_radio *self);
static void debugLogPush(struct ldl_radio *self, uint8_t opcode, const uint8_t *data, size_t size);
static void debugLogFlush(struct ldl_radio *self, const char *fn);
static void rxDiagnostics(struct ldl_radio *self);
static void debugRegister(struct ldl_radio *self, const char *fn, const struct ldl_sx127x_debug_log_entry *entry);
#endif

/* functions **********************************************************/

#ifdef LDL_ENABLE_SX1272
void LDL_SX1272_init(struct ldl_radio *self, const struct ldl_sx127x_init_arg *arg)
{
    init_state(self, LDL_RADIO_SX1272, arg);
}

const struct ldl_radio_interface *LDL_SX1272_getInterface(void)
{
    return &interface;
}
#endif

#ifdef LDL_ENABLE_SX1276
void LDL_SX1276_init(struct ldl_radio *self, const struct ldl_sx127x_init_arg *arg)
{
    init_state(self, LDL_RADIO_SX1276, arg);
}

const struct ldl_radio_interface *LDL_SX1276_getInterface(void)
{
    return &interface;
}
#endif

void LDL_SX127X_setMode(struct ldl_radio *self, enum ldl_radio_mode mode)
{
    LDL_PEDANTIC(self != NULL)

#ifdef LDL_ENABLE_RADIO_DEBUG
    debugLogReset(self);
#endif

    switch(mode){
    default:
    {
        LDL_ABORT()
    }
        break;

    case LDL_RADIO_MODE_RESET:
    {
#ifdef LDL_ENABLE_RADIO_DEBUG
        if(self->mode == LDL_RADIO_MODE_RX){

            rxDiagnostics(self);
        }
#endif
        self->chip_set_mode(self->chip, LDL_CHIP_MODE_RESET);
    }
        break;

    case LDL_RADIO_MODE_BOOT:
    {
        switch(self->mode){
        case LDL_RADIO_MODE_RESET:

            self->chip_set_mode(self->chip, LDL_CHIP_MODE_SLEEP);
            /* cannot read/write until chip has booted */
            break;

        default:
            LDL_ABORT()
            break;
        }
    }
        break;

    case LDL_RADIO_MODE_SLEEP:
    {
        switch(self->mode){
        case LDL_RADIO_MODE_BOOT:

            setOpSleep(self);
            self->chip_set_mode(self->chip, LDL_CHIP_MODE_SLEEP);
            break;

        case LDL_RADIO_MODE_SLEEP:
            break;

        case LDL_RADIO_MODE_RX:
        case LDL_RADIO_MODE_TX:

            writeReg(self, RegIrqFlags, 0xff);
            writeReg(self, RegIrqFlagsMask, 0xff);

            self->chip_set_mode(self->chip, LDL_CHIP_MODE_SLEEP);
            setOpSleep(self);
            break;

        case LDL_RADIO_MODE_HOLD:

            self->chip_set_mode(self->chip, LDL_CHIP_MODE_SLEEP);
            setOpSleep(self);
            break;

        default:
            LDL_ABORT()
            break;
        }
    }
        break;

    case LDL_RADIO_MODE_RX:
    case LDL_RADIO_MODE_TX:
    {
        switch(self->mode){
        case LDL_RADIO_MODE_SLEEP:

            self->chip_set_mode(self->chip, LDL_CHIP_MODE_STANDBY);

            if(self->xtal == LDL_RADIO_XTAL_TCXO){

                setXTAL(self);
            }
            setOpStandby(self);
            break;

        case LDL_RADIO_MODE_HOLD:

            if(self->xtal == LDL_RADIO_XTAL_CRYSTAL){

                setOpStandby(self);
            }
            break;

        default:
            LDL_ABORT()
            break;
        }
    }
        break;

    case LDL_RADIO_MODE_HOLD:

        switch(self->mode){
        case LDL_RADIO_MODE_RX:
        case LDL_RADIO_MODE_TX:

            writeReg(self, RegIrqFlags, 0xff);
            writeReg(self, RegIrqFlagsMask, 0xff);

            if(self->xtal == LDL_RADIO_XTAL_CRYSTAL){

                setOpSleep(self);
            }
            break;

        default:
            LDL_ABORT()
            break;
        }
        break;
    }

    LDL_DEBUG("new_mode=%i cur_mode=%i", mode, self->mode)

#ifdef LDL_ENABLE_RADIO_DEBUG
    debugLogFlush(self, __FUNCTION__);
#endif

    self->mode = mode;
}

void LDL_SX127X_transmit(struct ldl_radio *self, const struct ldl_radio_tx_setting *settings, const void *data, uint8_t len)
{
    LDL_PEDANTIC(self != NULL)
    LDL_PEDANTIC(settings != NULL)
    LDL_PEDANTIC((data != NULL) || (len == 0U))
    LDL_PEDANTIC(settings->freq != 0U)
    LDL_PEDANTIC(self->mode == LDL_RADIO_MODE_TX)
    LDL_PEDANTIC((self->type == LDL_RADIO_SX1272) || (self->type == LDL_RADIO_SX1276))

    struct modem_config config = {
        .sf = settings->sf,
        .bw = settings->bw,
        .timeout = 0U,
        .crc = true
    };

    int16_t dbm = settings->dbm + self->tx_gain;

#ifdef LDL_ENABLE_RADIO_DEBUG
    debugLogReset(self);
#endif

    if(dbm > settings->max_eirp){

        dbm = settings->max_eirp;
    }

#ifdef LDL_ENABLE_SX1272
    if(self->type == LDL_RADIO_SX1272){

        SX1272_setPower(self, dbm);
        SX1272_setModemConfig1(self, &config);
        SX1272_setModemConfig2(self, &config);
    }
#endif
#ifdef LDL_ENABLE_SX1276
    if(self->type == LDL_RADIO_SX1276){

        SX1276_setPower(self, dbm);
        SX1276_setModemConfig1(self, &config);
        SX1276_setModemConfig2(self, &config);
        SX1276_setModemConfig3(self, &config);
    }
#endif

    writeReg(self, RegSyncWord, 0x34U);                 // set sync word
    writeReg(self, RegInvertIQ, 0x27U);                 // non-invert IQ
    writeReg(self, RegDioMapping1, 0x40U);              // DIO0 (TX_COMPLETE) DIO1 (RX_DONE)
    writeReg(self, RegIrqFlags, 0xffU);                 // clear interrupts
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
    debugLogFlush(self, __FUNCTION__);
#endif
}

void LDL_SX127X_receive(struct ldl_radio *self, const struct ldl_radio_rx_setting *settings)
{
    LDL_PEDANTIC(self != NULL)
    LDL_PEDANTIC(settings != NULL)
    LDL_PEDANTIC(settings->freq != 0U)
    LDL_PEDANTIC(self->mode == LDL_RADIO_MODE_RX)
    LDL_PEDANTIC((self->type == LDL_RADIO_SX1272) || (self->type == LDL_RADIO_SX1276))

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
    debugLogReset(self);
#endif

    self->chip_set_mode(self->chip, LDL_CHIP_MODE_RX);                  // configure accessory IO

#ifdef LDL_ENABLE_SX1272
    if(self->type == LDL_RADIO_SX1272){

        SX1272_setModemConfig1(self, &config);
        SX1272_setModemConfig2(self, &config);
    }
#endif
#ifdef LDL_ENABLE_SX1276
    if(self->type == LDL_RADIO_SX1276){

        SX1276_setModemConfig1(self, &config);
        SX1276_setModemConfig2(self, &config);
        SX1276_setModemConfig3(self, &config);
    }
#endif

    writeReg(self, RegSymbTimeoutLsb, U8(timeout));         // set symbol timeout
    writeReg(self, RegSyncWord, 0x34);                      // set sync word
    writeReg(self, RegLna, 0x23);                           // LNA gain to max, LNA boost enable
    writeReg(self, RegPayloadMaxLength, settings->max);     // max payload
    writeReg(self, RegInvertIQ, U8(0x40 + 0x27));           // invert IQ
    writeReg(self, RegDioMapping1, 0U);                     // DIO0 (RX_TIMEOUT) DIO1 (RX_DONE)
    writeReg(self, RegIrqFlags, 0xff);                      // clear all interrupts
    writeReg(self, RegIrqFlagsMask, 0x3f);                  // unmask RX_TIMEOUT and RX_DONE interrupt
    writeReg(self, RegFifoAddrPtr, 0);

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
    debugLogFlush(self, __FUNCTION__);
#endif
}

uint8_t LDL_SX127X_readBuffer(struct ldl_radio *self, struct ldl_radio_packet_metadata *meta, void *data, uint8_t max)
{
    LDL_PEDANTIC(self != NULL)
    LDL_PEDANTIC((data != NULL) || (max == 0U))
    LDL_PEDANTIC(self->mode == LDL_RADIO_MODE_RX)
    LDL_PEDANTIC((self->type == LDL_RADIO_SX1272) || (self->type == LDL_RADIO_SX1276))

    uint8_t retval;

#ifdef LDL_ENABLE_RADIO_DEBUG
    debugLogReset(self);
#endif

    retval = readFIFO(self, data, max);

    int16_t offset = 0;
    int16_t rssi = S16(readReg(self, RegPktRssiValue));
    int16_t snr = S16(readReg(self, RegPktSnrValue));

#ifdef LDL_ENABLE_SX1272
    if(self->type == LDL_RADIO_SX1272){

        offset = 139;
    }
#endif
#ifdef LDL_ENABLE_SX1276
    if(self->type == LDL_RADIO_SX1276){

        offset = 157;
    }
#endif

    meta->snr = snr/S16(4);
    meta->rssi = rssi - offset;

#ifdef LDL_ENABLE_RADIO_DEBUG
    debugLogFlush(self, __FUNCTION__);
#endif

    return retval;
}

void LDL_SX127X_receiveEntropy(struct ldl_radio *self)
{
    LDL_PEDANTIC(self != NULL)
    LDL_PEDANTIC(self->mode == LDL_RADIO_MODE_RX)
    LDL_PEDANTIC((self->type == LDL_RADIO_SX1272) || (self->type == LDL_RADIO_SX1276))

#ifdef LDL_ENABLE_RADIO_DEBUG
    debugLogReset(self);
#endif

    self->chip_set_mode(self->chip, LDL_CHIP_MODE_RX);  // configure accessory IO
    writeReg(self, RegIrqFlags, 0xffU);                 // clear all interrupts
    writeReg(self, RegIrqFlagsMask, 0xffU);             // mask all interrupts

    /* application note instructions */
#ifdef LDL_ENABLE_SX1272
    if(self->type == LDL_RADIO_SX1272){

        writeReg(self, RegModemConfig1, 0x0aU);
        writeReg(self, RegModemConfig2, 0x74U);
    }
#endif
#ifdef LDL_ENABLE_SX1276
    if(self->type == LDL_RADIO_SX1276){

        writeReg(self, RegModemConfig1, 0x72U);
        writeReg(self, RegModemConfig2, 0x70U);
    }
#endif

    setOpRXContinuous(self);                                        // continuous RX

#ifdef LDL_ENABLE_RADIO_DEBUG
    debugLogFlush(self, __FUNCTION__);
#endif
}

uint32_t LDL_SX127X_readEntropy(struct ldl_radio *self)
{
    size_t i;
    uint32_t retval = 0U;

    LDL_PEDANTIC(self != NULL)
    LDL_PEDANTIC(self->mode == LDL_RADIO_MODE_RX)

#ifdef LDL_ENABLE_RADIO_DEBUG
    debugLogReset(self);
#endif

    /* sample wideband RSSI */
    for(i=0U; i < (sizeof(retval) << 3); i++){

        retval <<= 1;
        retval |= (U32(readReg(self, RegRssiWideband)) & 1U);
    }

    setOpStandby(self);                                     // standby
    self->chip_set_mode(self->chip, LDL_CHIP_MODE_STANDBY); // configure accessory IO
    writeReg(self, RegIrqFlags, 0xffU);                     // clear all interrupts
    writeReg(self, RegIrqFlagsMask, 0xffU);                 // mask all interrupts

#ifdef LDL_ENABLE_RADIO_DEBUG
    debugLogFlush(self, __FUNCTION__);
#endif

    return retval;
}

void LDL_SX127X_getStatus(struct ldl_radio *self, struct ldl_radio_status *status)
{
    uint8_t flags;

    flags = readReg(self, RegIrqFlags);

    status->tx = ((flags & 0x08U) > 0U);
    status->rx = ((flags & 0x40U) > 0U);
    status->timeout = ((flags & 0x80U) > 0U);
}

/* static functions ***************************************************/

static void init_state(struct ldl_radio *self, enum ldl_radio_type type, const struct ldl_sx127x_init_arg *arg)
{
    LDL_PEDANTIC(self != NULL)
    LDL_PEDANTIC(arg != NULL)

    LDL_PEDANTIC(arg->chip_read != NULL)
    LDL_PEDANTIC(arg->chip_write != NULL)
    LDL_PEDANTIC(arg->chip_set_mode != NULL)

    (void)memset(self, 0, sizeof(*self));

    self->type = type;

    self->chip = arg->chip;
    self->chip_read = arg->chip_read;
    self->chip_write = arg->chip_write;
    self->chip_set_mode = arg->chip_set_mode;

    self->state.sx127x.pa = arg->pa;
    self->tx_gain = arg->tx_gain;
    self->xtal = arg->xtal;
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

#ifdef LDL_ENABLE_SX1272
static void SX1272_setPower(struct ldl_radio *self, int16_t dbm)
{
    enum ldl_sx127x_pa pa = self->state.sx127x.pa;

    dbm /= S16(100);

    uint8_t paConfig;
    uint8_t paDac;

    if(pa == LDL_SX127X_PA_AUTO){

        pa = (dbm <= 14) ? LDL_SX127X_PA_RFO : LDL_SX127X_PA_BOOST;
    }

    switch(pa){
    default:
    case LDL_SX127X_PA_RFO:

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

    case LDL_SX127X_PA_BOOST:

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

static void SX1272_setModemConfig1(struct ldl_radio *self, const struct modem_config *config)
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
     * codingRate           (3bit) (LDL_CR_5)
     * implicitHeaderModeOn (1bit) (0)
     * rxPayloadCrcOn       (1bit) (1)
     * lowDataRateOptimize  (1bit)      */
    writeReg(self, RegModemConfig1, bw | 8U | 0U | (config->crc ? 2U : 0U) | (low_rate ? 1U : 0U));
}

static void SX1272_setModemConfig2(struct ldl_radio *self, const struct modem_config *config)
{
    uint8_t sf = (U8(config->sf) & U8(0xf)) << 4;

    /* spreadingFactor      (4bit)
     * txContinuousMode     (1bit) (0)
     * agcAutoOn            (1bit) (1)
     * symbTimeout(9:8)     (2bit) (0)  */
    writeReg(self, RegModemConfig2, sf | 0U | 4U | (U8(config->timeout >> 8) & 3U));
}
#endif

#ifdef LDL_ENABLE_SX1276
static void SX1276_setPower(struct ldl_radio *self, int16_t dbm)
{
    enum ldl_sx127x_pa pa = self->state.sx127x.pa;

    dbm /= S16(100);

    uint8_t paConfig;
    uint8_t paDac;

    if(pa == LDL_SX127X_PA_AUTO){

        pa = (dbm <= 14) ? LDL_SX127X_PA_RFO : LDL_SX127X_PA_BOOST;
    }

    switch(pa){
    default:
    /* -4dbm to 14dbm */
    case LDL_SX127X_PA_RFO:

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
    case LDL_SX127X_PA_BOOST:

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

static void SX1276_setModemConfig1(struct ldl_radio *self, const struct modem_config *config)
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
     * codingRate           (3bit) (LDL_CR_5)
     * implicitHeaderModeOn (1bit) (0)  */
    writeReg(self, RegModemConfig1, bw | 2U | 0U);
}

static void SX1276_setModemConfig2(struct ldl_radio *self, const struct modem_config *config)
{
    uint8_t sf = (U8(config->sf) & U8(0xf)) << 4;

    /* spreadingFactor      (4bit)
     * txContinuousMode     (1bit) (0)
     * rxPayloadCrcOn       (1bit) (1)
     * symbTimeout(9:8)     (2bit) (0)  */
    writeReg(self, RegModemConfig2, sf | 0U | (config->crc ? 4U : 0U) | 0U | (U8(config->timeout >> 8) & 3U));
}

static void SX1276_setModemConfig3(struct ldl_radio *self, const struct modem_config *config)
{
    bool low_rate = (config->bw == LDL_BW_125) && ((config->sf == LDL_SF_11) || (config->sf == LDL_SF_12));

    /* unused               (4bit) (0)
     * lowDataRateOptimize  (1bit)
     * agcAutoOn            (1bit) (1)
     * unused               (2bit) (0)  */
    writeReg(self, RegModemConfig3, 0U | (low_rate ? 8U : 0U) | 4U | 0U);
}
#endif

static void setFreq(struct ldl_radio *self, uint32_t freq)
{
    uint32_t f = U32((U64(freq) << 19) / U64(32000000));

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

    self->chip_read(self->chip, &opcode, sizeof(opcode), &data, U8(sizeof(data)));

#ifdef LDL_ENABLE_RADIO_DEBUG
    debugLogPush(self, opcode, &data, sizeof(data));
#endif

    return data;
}

static void burstRead(struct ldl_radio *self, enum ldl_radio_sx1272_sx1276_register reg, uint8_t *data, uint8_t len)
{
    uint8_t opcode = U8(reg) & 0x7FU;

    self->chip_read(self->chip, &opcode, sizeof(opcode), data, len);

#ifdef LDL_ENABLE_RADIO_DEBUG
    debugLogPush(self, opcode, data, len);
#endif
}

static void writeReg(struct ldl_radio *self, enum ldl_radio_sx1272_sx1276_register reg, uint8_t data)
{
    uint8_t opcode = U8(reg) | 0x80U;

    self->chip_write(self->chip, &opcode, sizeof(opcode), &data, 1U);

#ifdef LDL_ENABLE_RADIO_DEBUG
    debugLogPush(self, opcode, &data, sizeof(data));
#endif
}

static void burstWrite(struct ldl_radio *self, enum ldl_radio_sx1272_sx1276_register reg, const uint8_t *data, uint8_t len)
{
    uint8_t opcode = U8(reg) | 0x80U;

    self->chip_write(self->chip, &opcode, sizeof(opcode), data, len);

#ifdef LDL_ENABLE_RADIO_DEBUG
    debugLogPush(self, opcode, data, len);
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
static void debugLogReset(struct ldl_radio *self)
{
    self->state.sx127x.debug.pos = 0U;
    self->state.sx127x.debug.overflow = false;
}

static void debugLogPush(struct ldl_radio *self, uint8_t opcode, const uint8_t *data, size_t size)
{
    LDL_ASSERT(size > 0)

    if(self->state.sx127x.debug.pos < sizeof(self->state.sx127x.debug.log)/sizeof(*self->state.sx127x.debug.log)){

        self->state.sx127x.debug.log[self->state.sx127x.debug.pos].opcode = opcode;
        self->state.sx127x.debug.log[self->state.sx127x.debug.pos].value = *data;
        self->state.sx127x.debug.log[self->state.sx127x.debug.pos].size = U8(size);
        self->state.sx127x.debug.pos++;
    }
    else{

        self->state.sx127x.debug.overflow = true;
    }
}

static void debugLogFlush(struct ldl_radio *self, const char *fn)
{
    size_t i;

    for(i=0U; i < self->state.sx127x.debug.pos; i++){

        debugRegister(self, fn, &self->state.sx127x.debug.log[i]);
    }

    self->state.sx127x.debug.pos = 0U;
    self->state.sx127x.debug.overflow = false;
}


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

static void debugRegister(struct ldl_radio *self, const char *fn, const struct ldl_sx127x_debug_log_entry *entry)
{
    uint8_t reg = entry->opcode & 0x7fU;
    uint8_t data = entry->value;

    static const char *opMode[] = {
        "SLEEP",
        "STDBY",
        "FSTX",
        "TX",
        "FSRX",
        "RXCONTINUOUS",
        "RXSINGLE",
        "CAD"
    };

    LDL_TRACE_BEGIN()
    LDL_TRACE_PART("%s: %s(", fn, (entry->opcode & 0x80U) ? "WRITE" : "READ")

    switch(reg){
    case RegFifo:

        LDL_TRACE_PART("RegFifo): reg=0x%02X bytes=%u", reg, entry->size)
        break;

    case RegOpMode:

        LDL_TRACE_PART("RegOpMode): reg=0x%02X value=0x%02X ", reg, data)

        if(self->type == LDL_RADIO_SX1272){

            LDL_TRACE_PART("LongRangeMode=%u AccessSharedRegister=%s Mode=%s",
                (data >> 7) & 1U,
                (data & 0x40) ? "FSK" : "LORA",
                opMode[data & 7]
            )
        }
        else{

            LDL_TRACE_PART("LongRangeMode=%u AccessSharedRegister=%s LowFrequencyModeOn=%u Mode=%s",
                (data >> 7) & 1U,
                (data & 0x40) ? "FSK" : "LORA",
                (data >> 3) & 1U,
                opMode[data & 7]
            )
        }
        break;

    case RegModemConfig1:

        LDL_TRACE_PART("RegModemConfig1): reg=0x%02X value=0x%02X ", reg, data)

        if(self->type == LDL_RADIO_SX1272){

            LDL_TRACE_PART("Bw=%u CodingRate=%u ImplicitHeaderModeOn=%u RxPayloadCrcOn=%u LowDataRateOptimize=%u",
                (data >> 6) & 3U,
                (data >> 3) & 7U,
                (data >> 2) & 1U,
                (data >> 1) & 1U,
                data & 1U
            )
        }
        else{

            LDL_TRACE_PART("Bw=%u CodingRate=%u ImplicitHeaderModeOn=%u",
                (data >> 4) & 0xfU,
                (data >> 1) & 7U,
                data & 1U
            )
        }
        break;

    case RegModemConfig2:

        LDL_TRACE_PART("RegModemConfig2): reg=0x%02X value=0x%02X ", reg, data)

        if(self->type == LDL_RADIO_SX1272){

            LDL_TRACE_PART("SpreadingFactor=%u TxContinuousMode=%u AgcAutoOn=%u SymbTimeout=%u",
                (data >> 4) & 0xfU,
                (data >> 3) & 1U,
                (data >> 2) & 1U,
                data & 3U
            )
        }
        else{

            LDL_TRACE_PART("SpreadingFactor=%u TxContinuousMode=%u RxPayloadCrcOn=%u SymbTimeout=%u",
                (data >> 4 & 0xfU),
                (data >> 3 & 0x1U),
                (data >> 2 & 0x1U),
                (data & 0x3U)
            )
        }

        break;

    case RegModemConfig3:

        LDL_TRACE_PART("RegModemConfig3): reg=0x%02X value=0x%02X ", reg, data)
        LDL_TRACE_PART("LowDataRateOptimize=%u AgcAutoOn=%u",
            (data >> 3 & 0xfU),
            (data >> 2 & 0x1U)
        )
        break;

    case RegSymbTimeoutLsb:

        LDL_TRACE_PART("RegSymbTimeoutLsb): reg=0x%02X value=0x%02X ", reg, data)
        LDL_TRACE_PART("SymbTimeout=%u",
            data
        )
        break;

    case RegFrfMsb:

        LDL_TRACE_PART("RegFrfMsb): reg=0x%02X value=0x%02X", reg, data)
        break;

    case RegFrfMid:

        LDL_TRACE_PART("RegFrfMid): reg=0x%02X value=0x%02X", reg, data)
        break;

    case RegFrfLsb:

        LDL_TRACE_PART("RegFrfLsb): reg=0x%02X value=0x%02X", reg, data)
        break;

    case RegPaConfig:

        LDL_TRACE_PART("RegPaConfig): reg=0x%02X value=0x%02X ", reg, data)
        LDL_TRACE_PART("PaSelect=%u MaxPower=%u OutputPower=%u",
            (data >> 7) & 1U,
            (data >> 4) & 7U,
            data & 0xfU
        )
        break;

    case SX1272RegPaDac:
    case SX1276RegPaDac:

        LDL_TRACE_PART("RegPaDac): reg=0x%02X value=0x%02X", reg, data)
        break;

    case RegIrqFlagsMask:
    case RegIrqFlags:

        LDL_TRACE_PART("%s): reg=0x%02X value=0x%02X ", (reg == RegIrqFlagsMask) ? "RegIrqFlagsMask" : "RegIrqFlags", reg, data)
        LDL_TRACE_PART("RxTimeout=%u RxDone=%u PayloadCRCError=%u ValidHeader=%u TxDone=%u CadDone=%u FhssChangeChannel=%u CadDetected=%u",
            (data >> 7) & 1U,
            (data >> 6) & 1U,
            (data >> 5) & 1U,
            (data >> 4) & 1U,
            (data >> 3) & 1U,
            (data >> 2) & 1U,
            (data >> 1) & 1U,
            data & 1U
        )
        break;

    case RegPktRssiValue:

        LDL_TRACE_PART("RegPktRssiValue): reg=0x%02X value=0x%02X", reg, data)
        break;

    case RegPktSnrValue:

        LDL_TRACE_PART("RegPktSnrValue): reg=0x%02X value=0x%02X PacketSnr=%u", reg, data, data)
        break;

    case RegSyncWord:

        LDL_TRACE_PART("RegSyncWord): reg=0x%02X value=0x%02X", reg, data)
        break;

    case RegPayloadMaxLength:

        LDL_TRACE_PART("RegPayloadMaxLength): reg=0x%02X value=0x%02X", reg, data)
        break;

    case RegInvertIQ:

        LDL_TRACE_PART("RegInvertIQ): reg=0x%02X value=0x%02X ", reg, data)
        LDL_TRACE_PART("InvertIQ=%u", (data >> 6) & 1U)
        break;

    case RegDioMapping1:

        LDL_TRACE_PART("RegDioMapping1): reg=0x%02X value=0x%02X", reg, data)
        break;

    case RegDioMapping2:

        LDL_TRACE_PART("RegDioMapping2): reg=0x%02X value=0x%02X", reg, data)
        break;

    case SX1272RegTcxo:
    case SX1276RegTcxo:

        LDL_TRACE_PART("RegTcxo): reg=0x%02X value=0x%02X ", reg, data)
        LDL_TRACE_PART("TcxoInputOn=%s",
            (data & 0x10U) ? "TCXO" : "CRYSTAL"
        )
        break;

    case RegFifoRxBaseAddr:

        LDL_TRACE_PART("RegFifoRxBaseAddr): reg=0x%02X value=0x%02X", reg, data)
        break;

    case RegFifoAddrPtr:

        LDL_TRACE_PART("RegFifoAddrPtr): reg=0x%02X value=0x%02X", reg, data)
        break;

    case RegRssiWideband:

        LDL_TRACE_PART("RegRssiWideband): reg=0x%02X value=0x%02X", reg, data)
        break;

    case RegLna:

        LDL_TRACE_PART("RegLna): reg=0x%02X value=0x%02X ", reg, data)

        if(self->type == LDL_RADIO_SX1272){

            LDL_TRACE_PART("LnaGain=%u LnaBoost=%u",
                (data >> 5) & 7U,
                data & 3U
            )
        }
        else{

            LDL_TRACE_PART("LnaGain=%u LnaBoostLf=%u LnaBoostHf=%u",
                (data >> 5) & 7U,
                (data >> 3) & 3U,
                data & 3U
            )
        }
        break;

    case RegOcp:

        LDL_TRACE_PART("RegOcp): reg=0x%02X value=0x%02X ", reg, data)
        LDL_TRACE_PART("OcpOn=%u OcpTrim=%u",
            (data >> 5) & 1U,
            data & 0x1fU
        )
        break;

    case RegFifoTxBaseAddr:

        LDL_TRACE_PART("RegFifoTxBaseAddr): reg=0x%02X value=0x%02X ", reg, data)
        break;

    case LoraRegPayloadLength:

        LDL_TRACE_PART("LoraRegPayloadLength): reg=0x%02X value=0x%02X ", reg, data)
        LDL_TRACE_PART("PayloadLength=%u", data)
        break;

    case RegRxNbBytes:

        LDL_TRACE_PART("RegRxNbBytes): reg=0x%02X value=0x%02X ", reg, data)
        LDL_TRACE_PART("RxNbBytes=%u", data)
        break;

    case RegModemStat:

        LDL_TRACE_PART("RegModemStat): reg=0x%02X value=0x%02X ", reg, data)
        LDL_TRACE_PART("RxCodingRate=%u modemClear=%u hdrValid=%u rxOngoing=%u sync=%u detected=%u",
            (data >> 5) & 3U,
            (data >> 4) & 1U,
            (data >> 3) & 1U,
            (data >> 2) & 1U,
            (data >> 1) & 1U,
            data & 1U
        )
        break;

    case RegHopChannel:

        LDL_TRACE_PART("RegHopChannel): reg=0x%02X value=0x%02X ", reg, data)
        LDL_TRACE_PART("PllTimeout=%u CrcOnPayload=%u FhssPresentChannel=%u",
            (data >> 7) & 1U,
            (data >> 6) & 1U,
            data & 0x3fU
        )
        break;

    case RegRxHeaderCntValueMsb:

        LDL_TRACE_PART("RegRxHeaderCntValueMsb): reg=0x%02X value=0x%02X", reg, data)
        break;

    case RegRxHeaderCntValueLsb:

        LDL_TRACE_PART("RegRxHeaderCntValueLsb): reg=0x%02X value=0x%02X", reg, data)
        break;

    case RegRxPacketCntValueMsb:

        LDL_TRACE_PART("RegRxPacketCntValueMsb): reg=0x%02X value=0x%02X", reg, data)
        break;

    case RegRxPacketCntValueLsb:

        LDL_TRACE_PART("RegRxPacketCntValueLsb): reg=0x%02X value=0x%02X", reg, data)
        break;

    case LoraRegFeiMsb:

        LDL_TRACE_PART("RegFeiMsb): reg=0x%02X value=0x%02X", reg, data)
        break;

    case RegFeiMid:

        LDL_TRACE_PART("RegFeiMid): reg=0x%02X value=0x%02X", reg, data)
        break;

    case LoraRegFeiLsb:

        LDL_TRACE_PART("RegFeiLsb): reg=0x%02X value=0x%02X", reg, data)
        break;

    case RegFifoRxCurrentAddr:

        LDL_TRACE_PART("RegFifoRxCurrentAddr): reg=0x%02X value=0x%02X", reg, data)
        break;

    default:

        LDL_TRACE_PART("?): reg=0x%02X ", reg)
        if(entry->size > 1U){

            LDL_TRACE_PART("bytes=%u", entry->size)
        }
        else{

            LDL_TRACE_PART("value=0x%02X", data)
        }
        break;
    }

    LDL_TRACE_FINAL()
}

#endif
#endif
