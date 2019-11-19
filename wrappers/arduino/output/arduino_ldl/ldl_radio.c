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

#include "ldl_debug.h"
#include "ldl_radio.h"
#include "ldl_chip.h"
#include "ldl_platform.h"

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
    RegTcxo=0x58,
    RegPaDac=0x5A,
    RegPll=0x5C,
    RegPllLowPn=0x5E,
    RegFormerTemp=0x6C,
    RegBitRateFrac=0x70
};

/* static function prototypes *****************************************/

static uint8_t readFIFO(struct ldl_radio *self, uint8_t *data, uint8_t max);
static void writeFIFO(struct ldl_radio *self, const uint8_t *data, uint8_t len);
static void setFreq(struct ldl_radio *self, uint32_t freq);
static void setModemConfig(struct ldl_radio *self, enum ldl_signal_bandwidth bw, enum ldl_spreading_factor sf);
static void setPower(struct ldl_radio *self, int16_t dbm);
static uint8_t readReg(struct ldl_radio *self, uint8_t reg);
static void writeReg(struct ldl_radio *self, uint8_t reg, uint8_t data);
static void burstRead(struct ldl_radio *self, uint8_t reg, uint8_t *data, uint8_t len);
static void burstWrite(struct ldl_radio *self, uint8_t reg, const uint8_t *data, uint8_t len);
static void setOpRX(struct ldl_radio *self);
static void setOpTX(struct ldl_radio *self);
static void setOpRXContinuous(struct ldl_radio *self);
static void setOpStandby(struct ldl_radio *self);
static void setOpSleep(struct ldl_radio *self);
static void setOp(struct ldl_radio *self, uint8_t op);
static void enableLora(struct ldl_radio *self);
static uint8_t crSetting(const struct ldl_radio *self, enum ldl_coding_rate cr);
static uint8_t bwSetting(const struct ldl_radio *self, enum ldl_signal_bandwidth bw);
static uint8_t sfSetting(const struct ldl_radio *self, enum ldl_spreading_factor sf);

/* functions **********************************************************/

void LDL_Radio_init(struct ldl_radio *self, enum ldl_radio_type type, void *board)
{
    LDL_PEDANTIC(self != NULL)
    
    (void)memset(self, 0, sizeof(*self));
    self->board = board;

    self->type = type;
}

void LDL_Radio_setPA(struct ldl_radio *self, enum ldl_radio_pa pa)
{
    LDL_PEDANTIC(self != NULL)
    
    self->pa = pa;    
}

void LDL_Radio_setHandler(struct ldl_radio *self, struct ldl_mac *mac, ldl_radio_event_fn handler)
{
    LDL_PEDANTIC(self != NULL)
    
    self->handler = handler;
    self->mac = mac;
}

void LDL_Radio_interrupt(struct ldl_radio *self, uint8_t n)
{
    LDL_PEDANTIC(self != NULL)
    
    if(self->handler != NULL){
        
        self->handler(self->mac, LDL_Radio_signal(self, n));
    }
}

void LDL_Radio_reset(struct ldl_radio *self, bool state)
{
    LDL_PEDANTIC(self != NULL)
    
    LDL_Chip_reset(self->board, state);
}

void LDL_Radio_transmit(struct ldl_radio *self, const struct ldl_radio_tx_setting *settings, const void *data, uint8_t len)
{
    LDL_PEDANTIC(self != NULL)
    LDL_PEDANTIC(settings != NULL)
    LDL_PEDANTIC((data != NULL) || (len == 0U))
    LDL_PEDANTIC(settings->freq != 0U)
    
    self->dio_mapping1 = 0x40U;
    
    setOpStandby(self);
    
    setModemConfig(self, settings->bw, settings->sf);    

    setFreq(self, settings->freq);
    setPower(self, settings->dbm);
    
    writeReg(self, RegSyncWord, 0x34);                                               // set sync word
    writeReg(self, RegPaRamp, (readReg(self, RegPaRamp) & 0xf0U) | 0x08U);    // 50us PA ramp
    writeReg(self, RegInvertIQ, readReg(self, RegInvertIQ) & ~(0x40U));       // non-invert IQ    
    writeReg(self, RegDioMapping1, self->dio_mapping1);                              // DIO0 (TX_COMPLETE) DIO1 (RX_DONE)
    writeReg(self, RegIrqFlags, 0xff);                                               // clear all interrupts
    writeReg(self, RegIrqFlagsMask, 0xf7U);                                          // unmask TX_DONE interrupt                    
    
    writeFIFO(self, data, len);
    
    setOpTX(self);    
}

void LDL_Radio_receive(struct ldl_radio *self, const struct ldl_radio_rx_setting *settings)
{
    LDL_PEDANTIC(self != NULL)
    LDL_PEDANTIC(settings != NULL)
    LDL_PEDANTIC(settings->freq != 0U)
    
    self->dio_mapping1 = 0U;
    
    setOpStandby(self);
    
    setModemConfig(self, settings->bw, settings->sf);
    
    setFreq(self, settings->freq);                                                  // set carrier frequency        
    
    writeReg(self, RegSymbTimeoutLsb, settings->timeout);                    // set symbol timeout
    writeReg(self, RegSyncWord, 0x34);                                       // set sync word
    writeReg(self, RegLna, 0x23U);                                           // LNA gain to max, LNA boost enable    
    writeReg(self, RegPayloadMaxLength, settings->max);                      // max payload
    writeReg(self, RegInvertIQ, readReg(self, RegInvertIQ) | 0x40U);  // invert IQ    
    writeReg(self, RegDioMapping1, self->dio_mapping1);                      // DIO0 (RX_TIMEOUT) DIO1 (RX_DONE)    
    writeReg(self, RegIrqFlags, 0xff);                                       // clear all interrupts
    writeReg(self, RegIrqFlagsMask, 0x3fU);                                  // unmask RX_TIMEOUT and RX_DONE interrupt                    
    
    setOpRX(self);
}

uint8_t LDL_Radio_collect(struct ldl_radio *self, struct ldl_radio_packet_metadata *meta, void *data, uint8_t max)
{   
    LDL_PEDANTIC(self != NULL)
    LDL_PEDANTIC((data != NULL) || (max == 0U))
    
    uint8_t retval;
    
    (void)memset(meta, 0, sizeof(*meta));
    
    retval = readFIFO(self, data, max);
    
    meta->rssi = (int16_t)readReg(self, RegPktRssiValue) - 157;
    meta->snr = (int8_t)(readReg(self, RegPktSnrValue) / 4);
    
    return retval;
}

enum ldl_radio_event LDL_Radio_signal(struct ldl_radio *self, uint8_t n)
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

void LDL_Radio_entropyBegin(struct ldl_radio *self)
{
    enableLora(self);
    setOpStandby(self);
    
    writeReg(self, RegIrqFlags, 0xff);         // clear all interrupts
    writeReg(self, RegIrqFlagsMask, 0xffU);    // mask all interrupts
    
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
    
    setOpRXContinuous(self);
}

unsigned int LDL_Radio_entropyEnd(struct ldl_radio *self)
{
    unsigned int retval = 0U;
    size_t i;
    
    for(i=0U; i < (sizeof(unsigned int)*8U); i++){
        
        retval <<= 1;
        retval |= readReg(self, RegRssiWideband) & 0x1U;
    }
    
    setOpSleep(self);
    
    return retval;
}

void LDL_Radio_sleep(struct ldl_radio *self)
{
    LDL_PEDANTIC(self != NULL)    
    
    enableLora(self);
}

void LDL_Radio_clearInterrupt(struct ldl_radio *self)
{
    LDL_PEDANTIC(self != NULL)
    
    writeReg(self, RegIrqFlags, 0xff);         // clear all interrupts
    writeReg(self, RegIrqFlagsMask, 0xffU);    // mask all interrupts
    
    setOpSleep(self);
}

#ifdef LDL_ENABLE_RADIO_TEST
void LDL_Radio_setFreq(struct ldl_radio *self, uint32_t freq)
{
    setFreq(self, freq);
}

void LDL_Radio_setModemConfig(struct ldl_radio *self, enum ldl_signal_bandwidth bw, enum ldl_spreading_factor sf)
{
    setModemConfig(self, bw, sf);
}

void LDL_Radio_setPower(struct ldl_radio *self, int16_t dbm)
{
    setPower(self, dbm);
}

void LDL_Radio_enableLora(struct ldl_radio *self)
{   
    enableLora(self);
}
#endif

/* static functions ***************************************************/

static void enableLora(struct ldl_radio *self)
{
    setOpSleep(self);    
    writeReg(self, RegOpMode, readReg(self, RegOpMode) | 0x80U);      
}

static void setOp(struct ldl_radio *self, uint8_t op)
{
    writeReg(self, RegOpMode, (readReg(self, RegOpMode) & ~(0x7U)) | (op & 0x7U));    
}

static void setOpSleep(struct ldl_radio *self)
{
    setOp(self, 0U);
}

static void setOpStandby(struct ldl_radio *self)
{   
    setOp(self, 1U);    
}

static void setOpRX(struct ldl_radio *self)
{   
    setOp(self, 6U);
}

static void setOpRXContinuous(struct ldl_radio *self)
{   
    setOp(self, 5U);    
}

static void setOpTX(struct ldl_radio *self)
{   
    setOp(self, 3U);    
}

static void setModemConfig(struct ldl_radio *self, enum ldl_signal_bandwidth bw, enum ldl_spreading_factor sf)
{
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
        writeReg(self, RegModemConfig2, sfSetting(self, sf) | 0U | 4U | 0U);    
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
        writeReg(self, RegModemConfig2, sfSetting(self, sf) | 0U | 4U | 0U);
        
        /* unused               (4bit) (0)
         * lowDataRateOptimize  (1bit) 
         * agcAutoOn            (1bit) (1)
         * unused               (2bit) (0)  */
        writeReg(self, RegModemConfig3, 0U | (low_rate ? 8U : 0U) | 4U | 0U);        
        break;
#endif    
    }
}

static void setPower(struct ldl_radio *self, int16_t dbm)
{
    /* Todo: 
     * 
     * - compensate for output gains/losses here
     * - adjust current limit trim
     * 
     * */
     
     dbm /= 100;

    switch(self->type){
#ifdef LDL_ENABLE_SX1272        
    case LDL_RADIO_SX1272:
    {
        uint8_t paConfig;
        uint8_t paDac;    
        
        paConfig = readReg(self, RegPaConfig);         
        paConfig &= ~(0xfU);

        switch(self->pa){
        case LDL_RADIO_PA_RFO:
        
            /* -1 to 14dbm */
            paConfig &= ~(0x80U);
            paConfig |= (dbm > 14) ? 0xf : (uint8_t)( (dbm < -1) ? 0 : (dbm + 1) );
            
            writeReg(self, RegPaConfig, paConfig);            
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
    
        switch(self->pa){
        case LDL_RADIO_PA_RFO:
        
            /* todo */
            paConfig |= 0x7eU;
            writeReg(self, RegPaConfig, paConfig);            
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
    uint32_t f = (uint32_t)(((uint64_t)freq << 19U) / 32000000U);
        
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
    burstRead(self, reg, &data, sizeof(data));
    return data;
}

static void writeReg(struct ldl_radio *self, uint8_t reg, uint8_t data)
{
    burstWrite(self, reg, &data, sizeof(data));
}

static void burstWrite(struct ldl_radio *self, uint8_t reg, const uint8_t *data, uint8_t len)
{
    uint8_t i;

    if(len > 0U){

        LDL_Chip_select(self->board, true);
        
        LDL_Chip_write(self->board, reg | 0x80U);

        for(i=0; i < len; i++){

            LDL_Chip_write(self->board, data[i]);
        }

        LDL_Chip_select(self->board, false);
    }
}

static void burstRead(struct ldl_radio *self, uint8_t reg, uint8_t *data, uint8_t len)
{
    uint8_t i;

    if(len > 0U){

        LDL_Chip_select(self->board, true);

        LDL_Chip_write(self->board, reg & 0x7fU);

        for(i=0U; i < len; i++){

            data[i] = LDL_Chip_read(self->board);
        }

        LDL_Chip_select(self->board, false);
    }
}

#endif
