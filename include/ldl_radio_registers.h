/* Copyright (c) 2020 Cameron Harper
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

#ifndef LDL_RADIO_REGISTERS_H
#define LDL_RADIO_REGISTERS_H

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

#endif
