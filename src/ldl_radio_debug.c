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

#include "ldl_internal.h"
#include "ldl_radio_debug.h"
#include "ldl_radio.h"
#include "ldl_debug.h"
#include "ldl_radio_registers.h"

#ifdef LDL_ENABLE_RADIO_DEBUG

/* static function prototypes *****************************************/

static void debugRegister(struct ldl_radio *self, const char *fn, const struct ldl_radio_debug_log_entry *entry);

/* functions **********************************************************/

void LDL_Radio_debugLogReset(struct ldl_radio *self)
{
    self->debug.pos = 0U;
    self->debug.overflow = false;
}

void LDL_Radio_debugLogPush(struct ldl_radio *self, uint8_t opcode, const uint8_t *data, size_t size)
{
    LDL_ASSERT(size > 0)

    if(self->debug.pos < sizeof(self->debug.log)/sizeof(*self->debug.log)){

        self->debug.log[self->debug.pos].opcode = opcode;
        self->debug.log[self->debug.pos].value = *data;
        self->debug.log[self->debug.pos].size = U8(size);
        self->debug.pos++;
    }
    else{

        self->debug.overflow = true;
    }
}

void LDL_Radio_debugLogFlush(struct ldl_radio *self, const char *fn)
{
    size_t i;

    for(i=0U; i < self->debug.pos; i++){

        debugRegister(self, fn, &self->debug.log[i]);
    }

    self->debug.pos = 0U;
    self->debug.overflow = false;
}

/* static functions ***************************************************/

static void debugRegister(struct ldl_radio *self, const char *fn, const struct ldl_radio_debug_log_entry *entry)
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

#ifdef LDL_ENABLE_SX1272
        if(self->type == LDL_RADIO_SX1272){

            LDL_TRACE_PART("LongRangeMode=%u AccessSharedRegister=%s Mode=%s",
                (data >> 7) & 1U,
                (data & 0x40) ? "FSK" : "LORA",
                opMode[data & 7]
            )
        }
        else
#endif        
        {

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

#ifdef LDL_ENABLE_SX1272
        if(self->type == LDL_RADIO_SX1272){

            LDL_TRACE_PART("Bw=%u CodingRate=%u ImplicitHeaderModeOn=%u RxPayloadCrcOn=%u LowDataRateOptimize=%u",
                (data >> 6) & 3U,
                (data >> 3) & 7U,
                (data >> 2) & 1U,
                (data >> 1) & 1U,
                data & 1U
            )
        }
        else
#endif        
        {

            LDL_TRACE_PART("Bw=%u CodingRate=%u ImplicitHeaderModeOn=%u",
                (data >> 4) & 0xfU,
                (data >> 1) & 7U,
                data & 1U
            )
        }
        break;

    case RegModemConfig2:

        LDL_TRACE_PART("RegModemConfig2): reg=0x%02X value=0x%02X ", reg, data)

#ifdef LDL_ENABLE_SX1272
        if(self->type == LDL_RADIO_SX1272){

            LDL_TRACE_PART("SpreadingFactor=%u TxContinuousMode=%u AgcAutoOn=%u SymbTimeout=%u",
                (data >> 4) & 0xfU,
                (data >> 3) & 1U,
                (data >> 2) & 1U,
                data & 3U
            )
        }
        else
#endif        
        {

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

#ifdef LDL_ENABLE_SX1272
        if(self->type == LDL_RADIO_SX1272){

            LDL_TRACE_PART("LnaGain=%u LnaBoost=%u",
                (data >> 5) & 7U,
                data & 3U
            )
        }
        else
#endif        
        {

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
