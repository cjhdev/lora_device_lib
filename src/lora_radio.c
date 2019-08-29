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

#include <string.h>

#include "lora_debug.h"
#include "lora_radio.h"
#include "lora_system.h"
#include "lora_board.h"

#if defined(LORA_ENABLE_SX1272) || defined(LORA_ENABLE_SX1276)

enum lora_radio_sx1272_register {
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
    RegTcxo=0x58,
    RegPaDac=0x5A,
    RegPll=0x5C,
    RegPllLowPn=0x5E,
    RegFormerTemp=0x6C,
    RegBitRateFrac=0x70
};

/* static function prototypes *****************************************/

static uint8_t readFIFO(struct lora_radio *self, uint8_t *data, uint8_t max);
static void writeFIFO(struct lora_radio *self, const uint8_t *data, uint8_t len);
static void setFreq(struct lora_radio *self, uint32_t freq);
static void setModemConfig(struct lora_radio *self, enum lora_signal_bandwidth bw, enum lora_spreading_factor sf);
static void setPower(struct lora_radio *self, int16_t dbm);
static uint8_t readReg(const struct lora_board *self, uint8_t reg);
static void writeReg(const struct lora_board *self, uint8_t reg, uint8_t data);
static void burstRead(const struct lora_board *self, uint8_t reg, uint8_t *data, uint8_t len);
static void burstWrite(const struct lora_board *self, uint8_t reg, const uint8_t *data, uint8_t len);
static void setOpRX(struct lora_radio *self);
static void setOpTX(struct lora_radio *self);
static void setOpRXContinuous(struct lora_radio *self);
static void setOpStandby(struct lora_radio *self);
static void setOpSleep(struct lora_radio *self);
static void setOp(struct lora_radio *self, uint8_t op);
static void enableLora(struct lora_radio *self);
static uint8_t crSetting(const struct lora_radio *self, enum lora_coding_rate cr);
static uint8_t bwSetting(const struct lora_radio *self, enum lora_signal_bandwidth bw);
static uint8_t sfSetting(const struct lora_radio *self, enum lora_spreading_factor sf);

/* functions **********************************************************/

void LDL_Radio_init(struct lora_radio *self, enum lora_radio_type type, const struct lora_board *board)
{
    LORA_PEDANTIC(self != NULL)
    LORA_PEDANTIC(board != NULL)
    
    /* the following are mandatory functions */
    LORA_ASSERT(board->select != NULL)
    LORA_ASSERT(board->write != NULL)
    LORA_ASSERT(board->read != NULL)
    LORA_ASSERT(board->reset != NULL)
    
    (void)memset(self, 0, sizeof(*self));
    self->board = board;

    self->type = type;
}

void LDL_Radio_setPA(struct lora_radio *self, enum lora_radio_pa pa)
{
    LORA_PEDANTIC(self != NULL)
    
    self->pa = pa;    
}

void LDL_Radio_reset(struct lora_radio *self, bool state)
{
    LORA_PEDANTIC(self != NULL)
    
    self->board->reset(self->board->receiver, state);
}

void LDL_Radio_transmit(struct lora_radio *self, const struct lora_radio_tx_setting *settings, const void *data, uint8_t len)
{
    LORA_PEDANTIC(self != NULL)
    LORA_PEDANTIC(settings != NULL)
    LORA_PEDANTIC((data != NULL) || (len == 0U))
    LORA_PEDANTIC(settings->freq != 0U)
    
    self->dio_mapping1 = 0x40U;
    
    setOpStandby(self);
    
    setModemConfig(self, settings->bw, settings->sf);    

    setFreq(self, settings->freq);
    setPower(self, settings->dbm);
    
    writeReg(self->board, RegSyncWord, 0x34);                                               // set sync word
    writeReg(self->board, RegPaRamp, (readReg(self->board, RegPaRamp) & 0xf0U) | 0x08U);    // 50us PA ramp
    writeReg(self->board, RegInvertIQ, readReg(self->board, RegInvertIQ) & ~(0x40U));       // non-invert IQ    
    writeReg(self->board, RegDioMapping1, self->dio_mapping1);                              // DIO0 (TX_COMPLETE) DIO1 (RX_DONE)
    writeReg(self->board, RegIrqFlags, 0xff);                                               // clear all interrupts
    writeReg(self->board, RegIrqFlagsMask, 0xf7U);                                          // unmask TX_DONE interrupt                    
    
    writeFIFO(self, data, len);
    
    setOpTX(self);    
}

void LDL_Radio_receive(struct lora_radio *self, const struct lora_radio_rx_setting *settings)
{
    LORA_PEDANTIC(self != NULL)
    LORA_PEDANTIC(settings != NULL)
    LORA_PEDANTIC(settings->freq != 0U)
    
    self->dio_mapping1 = 0U;
    
    setOpStandby(self);
    
    setModemConfig(self, settings->bw, settings->sf);
    
    setFreq(self, settings->freq);                                                  // set carrier frequency        
    
    writeReg(self->board, RegSymbTimeoutLsb, settings->timeout);                    // set symbol timeout
    writeReg(self->board, RegSyncWord, 0x34);                                       // set sync word
    writeReg(self->board, RegLna, 0x23U);                                           // LNA gain to max, LNA boost enable    
    writeReg(self->board, RegPayloadMaxLength, settings->max);                      // max payload
    writeReg(self->board, RegInvertIQ, readReg(self->board, RegInvertIQ) | 0x40U);  // invert IQ    
    writeReg(self->board, RegDioMapping1, self->dio_mapping1);                      // DIO0 (RX_TIMEOUT) DIO1 (RX_DONE)    
    writeReg(self->board, RegIrqFlags, 0xff);                                       // clear all interrupts
    writeReg(self->board, RegIrqFlagsMask, 0x3fU);                                  // unmask RX_TIMEOUT and RX_DONE interrupt                    
    
    setOpRX(self);
}

uint8_t LDL_Radio_collect(struct lora_radio *self, struct lora_radio_packet_metadata *meta, void *data, uint8_t max)
{   
    LORA_PEDANTIC(self != NULL)
    LORA_PEDANTIC((data != NULL) || (max == 0U))
    
    uint8_t retval;
    
    (void)memset(meta, 0, sizeof(*meta));
    
    retval = readFIFO(self, data, max);
    
    meta->rssi = (int16_t)readReg(self->board, RegPktRssiValue) - 157;
    meta->snr = (int8_t)(readReg(self->board, RegPktSnrValue) / 4);
    
    return retval;
}

enum lora_radio_event LDL_Radio_signal(struct lora_radio *self, uint8_t n)
{    
    LORA_PEDANTIC(self != NULL)
    
    enum lora_radio_event retval = LORA_RADIO_EVENT_NONE;
    
    switch(n){
    case 0U:  
    
        switch(self->dio_mapping1){
        case 0U:
            retval = LORA_RADIO_EVENT_RX_READY;
            break;
        case 0x40U:
            retval = LORA_RADIO_EVENT_TX_COMPLETE;
            break;
        default:
            /* do nothing */
            break;
        }         
        break;
        
    case 1U:
    
        switch(self->dio_mapping1){
        case 0U:
            retval = LORA_RADIO_EVENT_RX_TIMEOUT;
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

void LDL_Radio_entropyBegin(struct lora_radio *self)
{
    enableLora(self);
    setOpStandby(self);
    
    writeReg(self->board, RegIrqFlags, 0xff);         // clear all interrupts
    writeReg(self->board, RegIrqFlagsMask, 0xffU);    // mask all interrupts
    
    /* application note instructions */
    switch(self->type){
    default:
        break;
#ifdef LORA_ENABLE_SX1272        
    case LORA_RADIO_SX1272:
        writeReg(self->board, RegModemConfig1, 0x0aU);
        writeReg(self->board, RegModemConfig2, 0x74U);    
        break;
#endif    
#ifdef LORA_ENABLE_SX1276
    case LORA_RADIO_SX1276:
        writeReg(self->board, RegModemConfig1, 0x72U);
        writeReg(self->board, RegModemConfig2, 0x70U);
        break;
#endif    
    }   
    
    setOpRXContinuous(self);
}

unsigned int LDL_Radio_entropyEnd(struct lora_radio *self)
{
    unsigned int retval = 0U;
    size_t i;
    
    for(i=0U; i < (sizeof(unsigned int)*8U); i++){
        
        retval <<= 1;
        retval |= readReg(self->board, RegRssiWideband) & 0x1U;
    }
    
    setOpSleep(self);
    
    return retval;
}

void LDL_Radio_sleep(struct lora_radio *self)
{
    LORA_PEDANTIC(self != NULL)    
    
    enableLora(self);
}

void LDL_Radio_clearInterrupt(struct lora_radio *self)
{
    LORA_PEDANTIC(self != NULL)
    
    writeReg(self->board, RegIrqFlags, 0xff);         // clear all interrupts
    writeReg(self->board, RegIrqFlagsMask, 0xffU);    // mask all interrupts
    
    setOpSleep(self);
}

#ifdef LORA_ENABLE_RADIO_TEST
void LDL_Radio_setFreq(struct lora_radio *self, uint32_t freq)
{
    setFreq(self, freq);
}

void LDL_Radio_setModemConfig(struct lora_radio *self, enum lora_signal_bandwidth bw, enum lora_spreading_factor sf)
{
    setModemConfig(self, bw, sf);
}

void LDL_Radio_setPower(struct lora_radio *self, int16_t dbm)
{
    setPower(self, dbm);
}

void LDL_Radio_enableLora(struct lora_radio *self)
{   
    enableLora(self);
}
#endif

/* static functions ***************************************************/

static void enableLora(struct lora_radio *self)
{
    setOpSleep(self);    
    writeReg(self->board, RegOpMode, readReg(self->board, RegOpMode) | 0x80U);      
}

static void setOp(struct lora_radio *self, uint8_t op)
{
    writeReg(self->board, RegOpMode, (readReg(self->board, RegOpMode) & ~(0x7U)) | (op & 0x7U));    
}

static void setOpSleep(struct lora_radio *self)
{
    setOp(self, 0U);
}

static void setOpStandby(struct lora_radio *self)
{   
    setOp(self, 1U);    
}

static void setOpRX(struct lora_radio *self)
{   
    setOp(self, 6U);
}

static void setOpRXContinuous(struct lora_radio *self)
{   
    setOp(self, 5U);    
}

static void setOpTX(struct lora_radio *self)
{   
    setOp(self, 3U);    
}

static void setModemConfig(struct lora_radio *self, enum lora_signal_bandwidth bw, enum lora_spreading_factor sf)
{
    bool low_rate = ((bw == BW_125) && ((sf == SF_11) || (sf == SF_12))) ? true : false;
    
    switch(self->type){
    default:
        break;
#ifdef LORA_ENABLE_SX1272          
    case LORA_RADIO_SX1272:
        /* bandwidth            (2bit)
         * codingRate           (3bit)
         * implicitHeaderModeOn (1bit) (0)
         * rxPayloadCrcOn       (1bit) (1)
         * lowDataRateOptimize  (1bit)      */
        writeReg(self->board, RegModemConfig1, bwSetting(self, bw) | crSetting(self, CR_5) | 0U | 2U | (low_rate ? 1U : 0U));
        
        /* spreadingFactor      (4bit)
         * txContinuousMode     (1bit) (0)
         * agcAutoOn            (1bit) (1)
         * symbTimeout(9:8)     (2bit) (0)  */
        writeReg(self->board, RegModemConfig2, sfSetting(self, sf) | 0U | 4U | 0U);    
        break;
#endif
#ifdef LORA_ENABLE_SX1276
    case LORA_RADIO_SX1276:
        /* bandwidth            (4bit)
         * codingRate           (3bit)
         * implicitHeaderModeOn (1bit) (0)  */
        writeReg(self->board, RegModemConfig1, bwSetting(self, bw) | crSetting(self, CR_5) | 0U);
        
        /* spreadingFactor      (4bit)
         * txContinuousMode     (1bit) (0)
         * rxPayloadCrcOn       (1bit) (1)
         * symbTimeout(9:8)     (2bit) (0)  */
        writeReg(self->board, RegModemConfig2, sfSetting(self, sf) | 0U | 4U | 0U);
        
        /* unused               (4bit) (0)
         * lowDataRateOptimize  (1bit) 
         * agcAutoOn            (1bit) (1)
         * unused               (2bit) (0)  */
        writeReg(self->board, RegModemConfig3, 0U | (low_rate ? 8U : 0U) | 4U | 0U);        
        break;
#endif    
    }
}

static void setPower(struct lora_radio *self, int16_t dbm)
{
    /* Todo: 
     * 
     * - compensate for output gains/losses here
     * - adjust current limit trim
     * 
     * */
     
     dbm /= 100L;

    switch(self->type){
#ifdef LORA_ENABLE_SX1272        
    case LORA_RADIO_SX1272:
    {
        uint8_t paConfig;
        uint8_t paDac;    
        
        paConfig = readReg(self->board, RegPaConfig);         
        paConfig &= ~(0xfU);

        switch(self->pa){
        case LORA_RADIO_PA_RFO:
        
            /* -1 to 14dbm */
            paConfig &= ~(0x80U);
            paConfig |= (dbm > 14) ? 0xfU : (uint8_t)( (dbm < -1) ? 0U : (dbm + 1) );
            
            writeReg(self->board, RegPaConfig, paConfig);            
            break;
        
        case LORA_RADIO_PA_BOOST:
        
            paDac = readReg(self->board, RegPaDac); 
                
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
                paConfig |= (dbm > 17) ? 0xfU : ( (dbm < 2) ? 0U : (dbm - 2) ); 
            }
            
            writeReg(self->board, RegPaDac, paDac);
            writeReg(self->board, RegPaConfig, paConfig);            
            break;
        default:
            break;        
        }
    }
        break;
#endif              
#ifdef LORA_ENABLE_SX1276
    case LORA_RADIO_SX1276:
    {
        uint8_t paConfig;
        uint8_t paDac;    
        
        paConfig = readReg(self->board, RegPaConfig);         
        paConfig &= ~(0xfU);
    
        switch(self->pa){
        case LORA_RADIO_PA_RFO:
        
            /* todo */
            paConfig |= 0x7eU;
            writeReg(self->board, RegPaConfig, paConfig);            
            break;
        
        case LORA_RADIO_PA_BOOST:
        
            /* regpadac address == 0x4d */
            paDac = readReg(self->board, 0x4d); 
                
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
                paConfig |= (dbm > 17) ? 0xfU : ( (dbm < 2) ? 0U : (dbm - 2) ); 
            }
            
            /* regpadac address == 0x4d */
            writeReg(self->board, 0x4d, paDac);
            writeReg(self->board, RegPaConfig, paConfig);   
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

static uint8_t bwSetting(const struct lora_radio *self, enum lora_signal_bandwidth bw)
{
    uint8_t retval = 0U;
    
    switch(self->type){
    default:
        break;
#ifdef LORA_ENABLE_SX1272          
    case LORA_RADIO_SX1272:    
        switch(bw){
        default:
        case BW_125:
            retval = 0x00U;
            break;
        case BW_250:
            retval = 0x40U;
            break;
        case BW_500:
            retval = 0x80U;
            break;
        }
        break;
#endif
#ifdef LORA_ENABLE_SX1276          
    case LORA_RADIO_SX1276:    
        switch(bw){
        default:
        case BW_125:
            retval = 0x70U;
            break;
        case BW_250:
            retval = 0x80U;
            break;
        case BW_500:
            retval = 0x90U;
            break;
        }
        break;
#endif
    }
    
    return retval;    
}

static uint8_t sfSetting(const struct lora_radio *self, enum lora_spreading_factor sf)
{
    uint8_t retval = 0U;
    switch(sf){
    default:
    case SF_7:
        retval = 0x70U;
        break;
    case SF_8:
        retval = 0x80U;
        break;
    case SF_9:
        retval = 0x90U;
        break;
    case SF_10:
        retval = 0xa0U;
        break;
    case SF_11:
        retval = 0xb0U;
        break;
    case SF_12:
        retval = 0xc0U;
        break;
    }
    return retval;
}

static uint8_t crSetting(const struct lora_radio *self, enum lora_coding_rate cr)
{
    uint8_t retval = 0U;
    
    switch(self->type){
    default:
        break;
#ifdef LORA_ENABLE_SX1272
    case LORA_RADIO_SX1272:
        retval = 8U;    // CR_5
        break;
#endif
#ifdef LORA_ENABLE_SX1276
    case LORA_RADIO_SX1276:
        retval = 2U;    // CR_5
        break;
#endif
    }
    
    return retval;
}

static void setFreq(struct lora_radio *self, uint32_t freq)
{
    uint32_t f = (uint32_t)(((uint64_t)freq << 19U) / 32000000U);
        
    writeReg(self->board, RegFrfMsb, f >> 16);
    writeReg(self->board, RegFrfMid, f >> 8);
    writeReg(self->board, RegFrfLsb, f);    
}

static uint8_t readFIFO(struct lora_radio *self, uint8_t *data, uint8_t max)
{
    uint8_t size = readReg(self->board, RegRxNbBytes);
    
    size = (size > max) ? max : size;
    
    if(size > 0U){
    
        writeReg(self->board, RegFifoAddrPtr, readReg(self->board, RegFifoRxCurrentAddr));
        
        burstRead(self->board, RegFifo, data, size);
    }

    return size;
}

static void writeFIFO(struct lora_radio *self, const uint8_t *data, uint8_t len)
{
    writeReg(self->board, RegFifoTxBaseAddr, 0x00U);    // set tx base
    writeReg(self->board, RegFifoAddrPtr, 0x00U);       // set address pointer
    writeReg(self->board, LoraRegPayloadLength, len);
    burstWrite(self->board, RegFifo, data, len);        // write into fifo
}

static uint8_t readReg(const struct lora_board *self, uint8_t reg)
{
    uint8_t data;
    burstRead(self, reg, &data, sizeof(data));
    return data;
}

static void writeReg(const struct lora_board *self, uint8_t reg, uint8_t data)
{
    burstWrite(self, reg, &data, sizeof(data));
}

static void burstWrite(const struct lora_board *self, uint8_t reg, const uint8_t *data, uint8_t len)
{
    uint8_t i;

    if(len > 0U){

        self->select(self->receiver, true);

        self->write(self->receiver, reg | 0x80U);

        for(i=0; i < len; i++){

            self->write(self->receiver, data[i]);
        }

        self->select(self->receiver, false);
    }
}

static void burstRead(const struct lora_board *self, uint8_t reg, uint8_t *data, uint8_t len)
{
    uint8_t i;

    if(len > 0U){

        self->select(self->receiver, true);

        self->write(self->receiver, reg & 0x7fU);

        for(i=0U; i < len; i++){

            data[i] = self->read(self->receiver);
        }

        self->select(self->receiver, false);
    }
}

#endif
