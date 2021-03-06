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

#include "ldl_mac_commands.h"
#include "ldl_stream.h"
#include "ldl_debug.h"
#include "ldl_internal.h"

struct type_to_tag {

    uint8_t tag;
    enum ldl_mac_cmd_type type;
};

static uint8_t typeToTag(enum ldl_mac_cmd_type type);
static bool tagToType(uint8_t tag, enum ldl_mac_cmd_type *type);

static const struct type_to_tag tags[] = {
    {2U, LDL_CMD_LINK_CHECK},
    {3U, LDL_CMD_LINK_ADR},
    {4U, LDL_CMD_DUTY_CYCLE},
    {5U, LDL_CMD_RX_PARAM_SETUP},
    {6U, LDL_CMD_DEV_STATUS},
    {7U, LDL_CMD_NEW_CHANNEL},
    {8U, LDL_CMD_RX_TIMING_SETUP},
    {9U, LDL_CMD_TX_PARAM_SETUP},
    {10U, LDL_CMD_DL_CHANNEL},
    {13U, LDL_CMD_DEVICE_TIME},
#if defined(LDL_ENABLE_L2_1_1)
    {11U, LDL_CMD_REKEY},
    {12U, LDL_CMD_ADR_PARAM_SETUP},
    {14U, LDL_CMD_FORCE_REJOIN},
    {15U, LDL_CMD_REJOIN_PARAM_SETUP},
#endif
#if defined(LDL_ENABLE_CLASS_B)
    {16U, LDL_CMD_PING_SLOT_INFO},
    {17U, LDL_CMD_PING_SLOT_CHANNEL},
    {18U, LDL_CMD_BEACON_TIMING},
    {19U, LDL_CMD_BEACON_FREQ},
#endif

};

/* functions **********************************************************/

bool LDL_MAC_peekNextCommand(struct ldl_stream *s, enum ldl_mac_cmd_type *type)
{
    uint8_t tag;
    bool retval = false;

    if(LDL_Stream_peek(s, &tag)){

        retval = tagToType(tag, type);
    }

    return retval;
}

uint8_t LDL_MAC_sizeofCommandUp(enum ldl_mac_cmd_type type)
{
    uint8_t retval = 0U;

    switch(type){
    default:
    case LDL_CMD_LINK_CHECK:
    case LDL_CMD_DUTY_CYCLE:
    case LDL_CMD_RX_TIMING_SETUP:
    case LDL_CMD_TX_PARAM_SETUP:
    case LDL_CMD_ADR_PARAM_SETUP:
    case LDL_CMD_DEVICE_TIME:
        retval = 1U;
        break;
    case LDL_CMD_LINK_ADR:
    case LDL_CMD_RX_PARAM_SETUP:
    case LDL_CMD_NEW_CHANNEL:
    case LDL_CMD_DL_CHANNEL:
    case LDL_CMD_REKEY:
    case LDL_CMD_REJOIN_PARAM_SETUP:
        retval = 2U;
        break;
    case LDL_CMD_DEV_STATUS:
    case LDL_CMD_FORCE_REJOIN:
        retval = 3U;
        break;
#ifdef LDL_ENABLE_CLASS_B
    case LDL_CMD_BEACON_TIMING:
        retval = 1U;
        break;
    case LDL_CMD_PING_SLOT_INFO:
    case LDL_CMD_BEACON_FREQ:
        retval = 2U;
        break;
    case LDL_CMD_PING_SLOT_CHANNEL:
        retval = 5U;
        break;
#endif
    }

    return retval;
}

void LDL_MAC_putLinkCheckReq(struct ldl_stream *s)
{
    (void)LDL_Stream_putU8(s, typeToTag(LDL_CMD_LINK_CHECK));
}

void LDL_MAC_putLinkADRAns(struct ldl_stream *s, const struct ldl_link_adr_ans *value)
{
    uint8_t buf;

    buf = (value->powerOK ? 4U : 0U) | (value->dataRateOK ? 2U : 0U) | (value->channelMaskOK ? 1U : 0U);

    (void)LDL_Stream_putU8(s, typeToTag(LDL_CMD_LINK_ADR));
    (void)LDL_Stream_putU8(s, buf);
}

void LDL_MAC_putDutyCycleAns(struct ldl_stream *s)
{
    (void)LDL_Stream_putU8(s, typeToTag(LDL_CMD_DUTY_CYCLE));
}

void LDL_MAC_putRXParamSetupAns(struct ldl_stream *s, const struct ldl_rx_param_setup_ans *value)
{
    uint8_t buf;

    buf = (value->rx1DROffsetOK ? 4U : 0U) | (value->rx2DataRateOK ? 2U : 0U) | (value->channelOK ? 1U : 0U);

    (void)LDL_Stream_putU8(s, typeToTag(LDL_CMD_RX_PARAM_SETUP));
    (void)LDL_Stream_putU8(s, buf);
}

void LDL_MAC_putDevStatusAns(struct ldl_stream *s, const struct ldl_dev_status_ans *value)
{
    (void)LDL_Stream_putU8(s, typeToTag(LDL_CMD_DEV_STATUS));
    (void)LDL_Stream_putU8(s, value->battery);
    (void)LDL_Stream_putU8(s, U8(value->margin) & 0x3fU);
}

void LDL_MAC_putNewChannelAns(struct ldl_stream *s, const struct ldl_new_channel_ans *value)
{
    uint8_t buf;

    buf = (value->dataRateRangeOK ? 2U : 0U) | (value->channelFreqOK ? 1U : 0U);

    (void)LDL_Stream_putU8(s, typeToTag(LDL_CMD_NEW_CHANNEL));
    (void)LDL_Stream_putU8(s, buf);
}


void LDL_MAC_putDLChannelAns(struct ldl_stream *s, const struct ldl_dl_channel_ans *value)
{
    uint8_t buf;

    buf = (value->uplinkFreqOK ? 2U : 0U) | (value->channelFreqOK ? 1U : 0U);

    (void)LDL_Stream_putU8(s, typeToTag(LDL_CMD_DL_CHANNEL));
    (void)LDL_Stream_putU8(s, buf);
}

void LDL_MAC_putRXTimingSetupAns(struct ldl_stream *s)
{
    (void)LDL_Stream_putU8(s, typeToTag(LDL_CMD_RX_TIMING_SETUP));
}

void LDL_MAC_putTXParamSetupAns(struct ldl_stream *s)
{
    (void)LDL_Stream_putU8(s, typeToTag(LDL_CMD_TX_PARAM_SETUP));
}

void LDL_MAC_putRekeyInd(struct ldl_stream *s, const struct ldl_rekey_ind *value)
{
    (void)LDL_Stream_putU8(s, typeToTag(LDL_CMD_REKEY));
    (void)LDL_Stream_putU8(s, value->version);
}

void LDL_MAC_putADRParamSetupAns(struct ldl_stream *s)
{
    (void)LDL_Stream_putU8(s, typeToTag(LDL_CMD_ADR_PARAM_SETUP));
}

void LDL_MAC_putDeviceTimeReq(struct ldl_stream *s)
{
    (void)LDL_Stream_putU8(s, typeToTag(LDL_CMD_DEVICE_TIME));
}

void LDL_MAC_putRejoinParamSetupAns(struct ldl_stream *s, struct ldl_rejoin_param_setup_ans *value)
{
    uint8_t buf;

    buf = (value->timeOK ? 1U : 0U);

    (void)LDL_Stream_putU8(s, typeToTag(LDL_CMD_REJOIN_PARAM_SETUP));
    (void)LDL_Stream_putU8(s, buf);
}

#ifdef LDL_ENABLE_CLASS_B
void LDL_MAC_putPingSlotInfoReq(struct ldl_stream *s, const struct ldl_ping_slot_info_req *value)
{
    (void)LDL_Stream_putU8(s, typeToTag(LDL_CMD_PING_SLOT_INFO));
    (void)LDL_Stream_putU8(s, value->periodicity & 7U);
}

void LDL_MAC_putPingSlotChannelAns(struct ldl_stream *s, const struct ldl_ping_slot_channel_ans *value)
{
    uint8_t buf = (value->dataRateOK ? 2U : 0U) | (value->channelFreqOK ? 1U : 0U);

    (void)LDL_Stream_putU8(s, typeToTag(LDL_CMD_PING_SLOT_CHANNEL));
    (void)LDL_Stream_putU8(s, buf);
}

void LDL_MAC_putBeaconTimingReq(struct ldl_stream *s)
{
    (void)LDL_Stream_putU8(s, typeToTag(LDL_CMD_BEACON_TIMING));
}

void LDL_MAC_putBeaconFreqAns(struct ldl_stream *s, const struct ldl_beacon_freq_ans *value)
{
    (void)LDL_Stream_putU8(s, typeToTag(LDL_CMD_BEACON_FREQ));
    (void)LDL_Stream_putU8(s, value->beaconFrequencyOK ? 1U : 0U);
}
#endif

bool LDL_MAC_getDownCommand(struct ldl_stream *s, struct ldl_downstream_cmd *cmd)
{
    uint8_t tag;
    bool retval = false;

    if(LDL_Stream_getU8(s, &tag)){

        retval = tagToType(tag, &cmd->type);

        if(retval){

            switch(cmd->type){
            default:
                /* impossible */
                break;

            case LDL_CMD_LINK_CHECK:

                (void)LDL_Stream_getU8(s, &cmd->fields.linkCheck.margin);
                (void)LDL_Stream_getU8(s, &cmd->fields.linkCheck.gwCount);
                break;

            case LDL_CMD_LINK_ADR:
            {
                uint8_t buf;

                (void)LDL_Stream_getU8(s, &buf);

                cmd->fields.linkADR.dataRate = buf >> 4;
                cmd->fields.linkADR.txPower = buf & 0xfU;

                (void)LDL_Stream_getU16(s, &cmd->fields.linkADR.channelMask);

                (void)LDL_Stream_getU8(s, &buf);

                cmd->fields.linkADR.channelMaskControl = (buf >> 4) & 0x7U;
                cmd->fields.linkADR.nbTrans = buf & 0xfU;
            }
                break;

            case LDL_CMD_DUTY_CYCLE:

                (void)LDL_Stream_getU8(s, &cmd->fields.dutyCycle.maxDutyCycle);
                cmd->fields.dutyCycle.maxDutyCycle &= 0xfU;
                break;

            case LDL_CMD_RX_PARAM_SETUP:
            {
                uint8_t DLsettings;

                (void)LDL_Stream_getU8(s, &DLsettings);
                cmd->fields.rxParamSetup.rx1DROffset = (DLsettings >> 4) & 0x7U;
                cmd->fields.rxParamSetup.rx2DataRate = (DLsettings)      & 0xfU;
                (void)LDL_Stream_getU24(s, &cmd->fields.rxParamSetup.freq);

                cmd->fields.rxParamSetup.freq *= U32(100);
            }
                break;

            case LDL_CMD_DEV_STATUS:

                // no args
                break;

            case LDL_CMD_NEW_CHANNEL:
            {
                uint8_t buf;

                (void)LDL_Stream_getU8(s, &cmd->fields.newChannel.chIndex);
                (void)LDL_Stream_getU24(s, &cmd->fields.newChannel.freq);
                (void)LDL_Stream_getU8(s, &buf);

                cmd->fields.newChannel.maxDR = buf >> 4;
                cmd->fields.newChannel.minDR = buf & 0xfU;

                cmd->fields.newChannel.freq *= U32(100);
            }
                break;

            case LDL_CMD_DL_CHANNEL:

                (void)LDL_Stream_getU8(s, &cmd->fields.dlChannel.chIndex);
                (void)LDL_Stream_getU24(s, &cmd->fields.dlChannel.freq);

                cmd->fields.dlChannel.freq *= U32(100);
                break;

            case LDL_CMD_RX_TIMING_SETUP:

                (void)LDL_Stream_getU8(s, &cmd->fields.rxTimingSetup.delay);
                cmd->fields.rxTimingSetup.delay &= 0xfU;
                break;

            case LDL_CMD_TX_PARAM_SETUP:
            {
                uint8_t buf;

                (void)LDL_Stream_getU8(s, &buf);

                cmd->fields.txParamSetup = buf & 0x3fU;
            }
                break;

            case LDL_CMD_DEVICE_TIME:

                (void)LDL_Stream_getU32(s, &cmd->fields.deviceTime.seconds);
                (void)LDL_Stream_getU8(s, &cmd->fields.deviceTime.fractions);
                break;

#if defined(LDL_ENABLE_L2_1_1)
            case LDL_CMD_REKEY:

                (void)LDL_Stream_getU8(s, &cmd->fields.rekey.version);
                cmd->fields.rekey.version &= 0xfU;
                break;

            case LDL_CMD_ADR_PARAM_SETUP:
            {
                uint8_t buf;

                (void)LDL_Stream_getU8(s, &buf);

                cmd->fields.adrParamSetup.limit_exp = buf >> 4;
                cmd->fields.adrParamSetup.delay_exp = buf & 0xfU;
            }
                break;

            case LDL_CMD_FORCE_REJOIN:
            {
                uint16_t buf;

                (void)LDL_Stream_getU16(s, &buf);

                cmd->fields.forceRejoin.period = U8(buf >> 10) & 0x7U;
                cmd->fields.forceRejoin.max_retries = U8(buf >> 7) & 0x7U;
                cmd->fields.forceRejoin.rejoin_type = U8(buf >> 4) & 0x7U;
                cmd->fields.forceRejoin.dr = U8(buf) & 0xfU;
            }
                break;

            case LDL_CMD_REJOIN_PARAM_SETUP:
            {
                uint8_t buf;

                (void)LDL_Stream_getU8(s, &buf);

                cmd->fields.rejoinParamSetup.maxTimeN = buf >> 4;
                cmd->fields.rejoinParamSetup.maxCountN = buf & 0xfU;
            }
                break;
#endif
#ifdef LDL_ENABLE_CLASS_B
            case LDL_CMD_PING_SLOT_INFO:

                // no args
                break;

            case LDL_CMD_PING_SLOT_CHANNEL:

                (void)LDL_Stream_getU24(s, &cmd->fields.pingSlotChannel.frequency);
                (void)LDL_Stream_getU8(s, &cmd->fields.pingSlotChannel.dr);

                cmd->fields.pingSlotChannel.frequency *= 100UL;
                cmd->fields.pingSlotChannel.dr &= 0xfU;
                break;

            case LDL_CMD_BEACON_TIMING:

                (void)LDL_Stream_getU16(s, &cmd->fields.beaconTiming.delay);
                (void)LDL_Stream_getU8(s, &cmd->fields.beaconTiming.channel);
                break;

            case LDL_CMD_BEACON_FREQ:

                (void)LDL_Stream_getU24(s, &cmd->fields.pingSlotChannel.frequency);

                cmd->fields.beaconFreq.freq *= 100UL;
                break;
#endif
            }
        }
    }

    return LDL_Stream_error(s) ? false : retval;
}

/* static functions ***************************************************/

static uint8_t typeToTag(enum ldl_mac_cmd_type type)
{
    uint8_t retval = 0U;
    uint8_t i;

    for(i=0U; i < (sizeof(tags)/sizeof(*tags)); i++){

        if(tags[i].type == type){

            retval = tags[i].tag;
            break;
        }
    }

    return retval;
}

static bool tagToType(uint8_t tag, enum ldl_mac_cmd_type *type)
{
    bool retval = false;
    uint8_t i;

    for(i=0U; i < (sizeof(tags)/sizeof(*tags)); i++){

        if(tags[i].tag == tag){

            *type = tags[i].type;
            retval = true;
            break;
        }
    }

    return retval;
}
