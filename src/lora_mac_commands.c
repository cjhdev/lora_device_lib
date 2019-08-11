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

#include "lora_mac_commands.h"
#include "lora_stream.h"
#include "lora_debug.h"

struct type_to_tag {
  
    uint8_t tag;    
    enum lora_mac_cmd_type type;    
};

static uint8_t typeToTag(enum lora_mac_cmd_type type);    
static bool tagToType(uint8_t tag, enum lora_mac_cmd_type *type);

static bool putU8(struct lora_stream *self, uint8_t in);
static bool getU8(struct lora_stream *self, uint8_t *out);
static bool getU16(struct lora_stream *self, uint16_t *out);
static bool getU24(struct lora_stream *self, uint32_t *out);

static bool getLinkCheckAns(struct lora_stream *s, struct lora_link_check_ans *value);
static bool getLinkADRReq(struct lora_stream *s, struct lora_link_adr_req *value);
static bool getRXParamSetupReq(struct lora_stream *s, struct lora_rx_param_setup_req *value);
static bool getNewChannelReq(struct lora_stream *s, struct lora_new_channel_req *value);
static bool getDLChannelReq(struct lora_stream *s, struct lora_dl_channel_req *value);
static bool getRXTimingSetupReq(struct lora_stream *s, struct lora_rx_timing_setup_req *value);
static bool getTXParamSetupReq(struct lora_stream *s, struct lora_tx_param_setup_req *value);
static bool getDutyCycleReq(struct lora_stream *s, struct lora_duty_cycle_req *value);

#ifndef LORA_DISABLE_FULL_CODEC
static bool getLinkADRAns(struct lora_stream *s, struct lora_link_adr_ans *value);
static bool getRXParamSetupAns(struct lora_stream *s, struct lora_rx_param_setup_ans *value);
static bool getDevStatusAns(struct lora_stream *s, struct lora_dev_status_ans *value);
static bool getNewChannelAns(struct lora_stream *s, struct lora_new_channel_ans *value);
static bool getDLChannelAns(struct lora_stream *s, struct lora_dl_channel_ans *value);
static bool putU24(struct lora_stream *self, uint32_t in);
static bool putU16(struct lora_stream *self, uint16_t in);
#endif

static const struct type_to_tag tags[] = {
    {2U, LINK_CHECK},
    {3U, LINK_ADR},
    {4U, DUTY_CYCLE},
    {5U, RX_PARAM_SETUP},
    {6U, DEV_STATUS},
    {7U, NEW_CHANNEL},
    {8U, RX_TIMING_SETUP},
    {9U, TX_PARAM_SETUP},
    {10U, DL_CHANNEL},
    {16U, PING_SLOT_INFO},
    {17U, PING_SLOT_CHANNEL},
    {18U, BEACON_TIMING},
    {19U, BEACON_FREQ}
};

/* functions **********************************************************/

bool LDL_MAC_peekNextCommand(struct lora_stream *s, enum lora_mac_cmd_type *type)
{
    uint8_t tag;
    bool retval = false;
    
    if(LDL_Stream_peek(s, &tag)){
        
        retval = tagToType(tag, type);            
    }
    
    return retval;
}

uint8_t LDL_MAC_sizeofCommandUp(enum lora_mac_cmd_type type)
{
    uint8_t retval = 0U;
    
    switch(type){
    case LINK_CHECK:
        retval = 1U;
        break;
    case LINK_ADR:
    case DUTY_CYCLE:
    case RX_PARAM_SETUP:
    case DEV_STATUS:
    case NEW_CHANNEL:
    case RX_TIMING_SETUP:
    case TX_PARAM_SETUP:
    case DL_CHANNEL:
    case PING_SLOT_INFO:
    case PING_SLOT_CHANNEL:
    case PING_SLOT_FREQ:
    case BEACON_TIMING:
    case BEACON_FREQ:
    default:      
        break;
    }
    
    return retval;
}

bool LDL_MAC_putLinkCheckReq(struct lora_stream *s)
{
    return putU8(s, typeToTag(LINK_CHECK));
}

bool LDL_MAC_putLinkADRAns(struct lora_stream *s, const struct lora_link_adr_ans *value)
{
    bool retval = false;
    uint8_t buf;
    size_t offset = LDL_Stream_tell(s);
    
    if(putU8(s, typeToTag(LINK_ADR))){
        
        buf = (value->powerOK ? 4U : 0U) | (value->dataRateOK ? 2U : 0U) | (value->channelMaskOK ? 1U : 0U);
            
        if(putU8(s, buf)){
            
            retval = true;
        }
        else{
            
            LDL_Stream_seekSet(s, offset);
        }
    }
    
    return retval;
}

bool LDL_MAC_putDutyCycleAns(struct lora_stream *s)
{
    return putU8(s, typeToTag(DUTY_CYCLE));
}

bool LDL_MAC_putRXParamSetupAns(struct lora_stream *s, const struct lora_rx_param_setup_ans *value)
{
    bool retval = false;
    size_t offset = LDL_Stream_tell(s);
    
    if(putU8(s, typeToTag(RX_PARAM_SETUP))){
            
        if(putU8(s, (value->rx1DROffsetOK ? 4U : 0U) | (value->rx2DataRateOK ? 2U : 0U) | (value->channelOK ? 1U : 0U))){
            
            retval = true;
        }
        else{
            
            (void)LDL_Stream_seekSet(s, offset);
        }
    }
    
    return retval;
}

bool LDL_MAC_putDevStatusAns(struct lora_stream *s, const struct lora_dev_status_ans *value)
{
    bool retval = false;
    size_t offset = LDL_Stream_tell(s);
    
    if(putU8(s, typeToTag(DEV_STATUS))){
    
        if(putU8(s, value->battery)){
        
            if(putU8(s, value->margin)){
                
                retval = true;
            }
        }
    }
    
    if(!retval){
        
        (void)LDL_Stream_seekSet(s, offset);
    }
    
    return retval;
}


bool LDL_MAC_putNewChannelAns(struct lora_stream *s, const struct lora_new_channel_ans *value)
{
    bool retval = false;
    size_t offset = LDL_Stream_tell(s);
    
    if(putU8(s, typeToTag(NEW_CHANNEL))){
    
        if(putU8(s, (value->dataRateRangeOK ? 2U : 0U) | (value->channelFrequencyOK ? 1U : 0U))){
            
            retval = true;
        }
        else{
            
            (void)LDL_Stream_seekSet(s, offset);
        }
    }
    
    return retval;
}


bool LDL_MAC_putDLChannelAns(struct lora_stream *s, const struct lora_dl_channel_ans *value)
{
    bool retval = false;
    size_t offset = LDL_Stream_tell(s);
    
    if(putU8(s, typeToTag(DL_CHANNEL))){
    
        if(putU8(s, (value->uplinkFreqOK ? 2U : 0U) | (value->channelFrequencyOK ? 1U : 0U))){
            
            retval = true;
        }
        else{
            
            (void)LDL_Stream_seekSet(s, offset);
        }
    }
    
    return retval;
}

bool LDL_MAC_putRXTimingSetupAns(struct lora_stream *s)
{
    return putU8(s, typeToTag(RX_TIMING_SETUP));
}

bool LDL_MAC_putTXParamSetupAns(struct lora_stream *s)
{
    return putU8(s, typeToTag(TX_PARAM_SETUP));
}

#ifndef LORA_DISABLE_FULL_CODEC
bool LDL_MAC_putLinkCheckAns(struct lora_stream *s, const struct lora_link_check_ans *value)
{
    bool retval = false;
    size_t offset = LDL_Stream_tell(s);
    
    if(putU8(s, typeToTag(LINK_CHECK))){
        
        if(putU8(s, value->margin)){
            
            if(putU8(s, value->gwCount)){                
                
                retval = true;
            }
        }
    }
    
    if(!retval){
        
        (void)LDL_Stream_seekSet(s, offset);
    }
    
    return retval;
}

bool LDL_MAC_putLinkADRReq(struct lora_stream *s, const struct lora_link_adr_req *value)
{
    bool retval = false;
    size_t offset = LDL_Stream_tell(s);
 
    if(putU8(s, typeToTag(LINK_ADR))){
        
        if(putU8(s, (value->dataRate << 4)|(value->txPower & 0xfU))){
            
            if(putU16(s, value->channelMask)){
    
                uint8_t buf = (value->nbTrans & 0xfU);
                buf |= ((value->channelMaskControl & 0x7U) << 4);
                    
                if(putU8(s, buf)){                                
                    
                    retval = true;                        
                }                
            }
        }
    }
    
    if(!retval){
        
        (void)LDL_Stream_seekSet(s, offset);
    }
 
    return retval;
}

bool LDL_MAC_putDutyCycleReq(struct lora_stream *s, const struct lora_duty_cycle_req *value)
{
    bool retval = false;
    size_t offset = LDL_Stream_tell(s);
    
    if(putU8(s, typeToTag(DUTY_CYCLE))){
    
        if(putU8(s, value->maxDutyCycle)){
        
            retval = true;
        }
        else{
            
            (void)LDL_Stream_seekSet(s, offset);
        }
    }
    
    return retval;
}

bool LDL_MAC_putRXParamSetupReq(struct lora_stream *s, const struct lora_rx_param_setup_req *value)
{
    bool retval = false;
    size_t offset = LDL_Stream_tell(s);
    
    if(putU8(s, typeToTag(RX_PARAM_SETUP))){
    
        if(putU8(s, (value->rx1DROffset << 4)|(value->rx2DataRate & 0xfU))){
        
            if(putU24(s, value->freq)){
                            
                retval = true;                    
            }
        }
    }
    
    if(!retval){
        
        (void)LDL_Stream_seekSet(s, offset);
    }
    
    return retval;
}

bool LDL_MAC_putDevStatusReq(struct lora_stream *s)
{
    return putU8(s, typeToTag(DEV_STATUS));
}

bool LDL_MAC_putNewChannelReq(struct lora_stream *s, const struct lora_new_channel_req *value)
{
    bool retval = false;
    size_t offset = LDL_Stream_tell(s);
    
    if(putU8(s, typeToTag(NEW_CHANNEL))){
    
        if(putU8(s, value->chIndex)){
        
            if(putU24(s, value->freq)){
                
                if(putU8(s, (value->maxDR << 4)|(value->minDR))){
                    
                    retval = true;
                }                    
            }
        }
    }
    
    if(!retval){
        
        (void)LDL_Stream_seekSet(s, offset);
    }
    
    return retval;
}

bool LDL_MAC_putDLChannelReq(struct lora_stream *s, const struct lora_dl_channel_req *value)
{
    bool retval = false;
    size_t offset = LDL_Stream_tell(s);
    
    if(putU8(s, typeToTag(NEW_CHANNEL))){
    
        if(putU8(s, value->chIndex)){
        
            if(putU24(s, value->freq)){
                                
                retval = true;                        
            }
        }
    }
    
    if(!retval){
        
        (void)LDL_Stream_seekSet(s, offset);
    }
    
    return retval;
}

bool LDL_MAC_putTXParamSetupReq(struct lora_stream *s, const struct lora_tx_param_setup_req *value)
{
    bool retval = false;
    size_t offset = LDL_Stream_tell(s);
    
    if(putU8(s, typeToTag(TX_PARAM_SETUP))){
    
        if(putU8(s, (value->downlinkDwell ? 0x20U : 0U) | (value->uplinkDwell ? 0x10U : 0U) | value->maxEIRP )){
            
            retval = true;
        }
        else{
            
            (void)LDL_Stream_seekSet(s, offset);
        }
    }
    
    return retval;    
}

bool LDL_MAC_putRXTimingSetupReq(struct lora_stream *s, const struct lora_rx_timing_setup_req *value)
{
    bool retval = false;
    size_t offset = LDL_Stream_tell(s);
    
    if(putU8(s, typeToTag(RX_TIMING_SETUP))){
    
        if(putU8(s, value->delay)){
            
            retval = true;
        }
        else{
            
            (void)LDL_Stream_seekSet(s, offset);
        }
    }
    
    return retval;
}
#endif

bool LDL_MAC_getDownCommand(struct lora_stream *s, struct lora_downstream_cmd *cmd)
{
    uint8_t tag;
    bool retval = false;
    
    if(getU8(s, &tag)){
        
        if(tagToType(tag, &cmd->type)){
            
            switch(cmd->type){
            default:
            case LINK_CHECK:
            
                retval = getLinkCheckAns(s, &cmd->fields.linkCheckAns);
                break;
                
            case LINK_ADR:                    
            
                retval = getLinkADRReq(s, &cmd->fields.linkADRReq);
                break;
            
            case DUTY_CYCLE:                
            
                retval = getDutyCycleReq(s, &cmd->fields.dutyCycleReq);
                break;
            
            case RX_PARAM_SETUP:
            
                retval = getRXParamSetupReq(s, &cmd->fields.rxParamSetupReq);
                break;
            
            case DEV_STATUS:
            
                retval = true;
                break;
            
            case NEW_CHANNEL:
            
                retval = getNewChannelReq(s, &cmd->fields.newChannelReq);
                break;
                
            case DL_CHANNEL:
            
                retval = getDLChannelReq(s, &cmd->fields.dlChannelReq);
                break;
            
            case RX_TIMING_SETUP:
            
                retval = getRXTimingSetupReq(s, &cmd->fields.rxTimingSetupReq);
                break;
            
            case TX_PARAM_SETUP:
            
                retval = getTXParamSetupReq(s, &cmd->fields.txParamSetupReq);
                break;
            }
        }
        else{
            
            LORA_INFO("cannot recognise MAC command")
        }
    }
    
    return retval;
}

#ifndef LORA_DISABLE_FULL_CODEC
bool LDL_MAC_getUpCommand(struct lora_stream *s, struct lora_upstream_cmd *cmd)
{
    uint8_t tag;
    bool retval = false;
    
    if(getU8(s, &tag)){
        
        if(tagToType(tag, &cmd->type)){
    
            switch(cmd->type){
            default:
            case LINK_CHECK:
            case DUTY_CYCLE:                
            case RX_TIMING_SETUP:
            case TX_PARAM_SETUP:
            
                retval = true;
                break;
                
            case LINK_ADR:                    
            
                retval = getLinkADRAns(s, &cmd->fields.linkADRAns);
                break;
            
            case RX_PARAM_SETUP:
            
                retval = getRXParamSetupAns(s, &cmd->fields.rxParamSetupAns);
                break;
            
            case DEV_STATUS:
            
                retval = getDevStatusAns(s, &cmd->fields.devStatusAns);
                break;
            
            case NEW_CHANNEL:
            
                retval = getNewChannelAns(s, &cmd->fields.newChannelAns);
                break;
                
            case DL_CHANNEL:
            
                retval = getDLChannelAns(s, &cmd->fields.dlChannelAns);
                break;            
            }
        }
        else{
            
            LORA_INFO("cannot recognise MAC command")
        }    
    }
    
    return retval;
}
#endif

/* static functions ***************************************************/

static uint8_t typeToTag(enum lora_mac_cmd_type type)
{
    return tags[type].tag;
}

static bool tagToType(uint8_t tag, enum lora_mac_cmd_type *type)
{
    bool retval = false;
    size_t i;
    
    for(i=0U; i < (sizeof(tags)/sizeof(*tags)); i++){
        
        if(tags[i].tag == tag){
            
            *type = tags[i].type;
            retval = true;
            break;
        }
    }
    
    return retval;
}

#ifndef LORA_DISABLE_FULL_CODEC
static bool getLinkADRAns(struct lora_stream *s, struct lora_link_adr_ans *value)
{
    bool retval = false;
    
    uint8_t buf;
    
    if(getU8(s, &buf)){
    
        value->powerOK = ((buf & 4U) == 4U);
        value->dataRateOK = ((buf & 2U) == 2U);
        value->channelMaskOK = ((buf & 1U) == 1U);
        
        retval = true;        
    }
    
    return retval;
}

static bool getRXParamSetupAns(struct lora_stream *s, struct lora_rx_param_setup_ans *value)
{
    bool retval = false;
    
    uint8_t buf;
    
    if(getU8(s, &buf)){
    
        value->rx1DROffsetOK = ((buf & 4U) == 4U);
        value->rx2DataRateOK = ((buf & 2U) == 2U);
        value->channelOK = ((buf & 1U) == 1U);
        
        retval = true;        
    }
    
    return retval;
}

static bool getDevStatusAns(struct lora_stream *s, struct lora_dev_status_ans *value)
{
    bool retval = false;
    
    if(getU8(s, &value->battery)){
        
        if(getU8(s, &value->margin)){
            
            retval = true;
        }
    }
    
    return retval;
}

static bool getNewChannelAns(struct lora_stream *s, struct lora_new_channel_ans *value)
{
    bool retval = false;
    
    uint8_t buf;
    
    if(getU8(s, &buf)){
    
        value->dataRateRangeOK = ((buf & 2U) == 2U);
        value->channelFrequencyOK = ((buf & 1U) == 1U);
        
        retval = true;        
    }
    
    return retval;
}

static bool getDLChannelAns(struct lora_stream *s, struct lora_dl_channel_ans *value)
{
    bool retval = false;
    
    uint8_t buf;
    
    if(getU8(s, &buf)){
    
        value->channelFrequencyOK = ((buf & 2U) == 2U);
        value->uplinkFreqOK = ((buf & 1U) == 1U);
        
        retval = true;        
    }
    
    return retval;
}

static bool putU24(struct lora_stream *self, uint32_t in)
{
    uint8_t out[] = {
        in, 
        in >> 8,
        in >> 16
    };
    
    return LDL_Stream_write(self, out, sizeof(out));
}

static bool putU16(struct lora_stream *self, uint16_t in)
{
    uint8_t out[] = {
        in, 
        in >> 8        
    };
    
    return LDL_Stream_write(self, out, sizeof(out));
}

#endif

static bool getLinkCheckAns(struct lora_stream *s, struct lora_link_check_ans *value)
{
    bool retval = false;
    uint8_t buf;
    
    if(getU8(s, &buf)){
        
        if(getU8(s, &value->gwCount)){
            
            retval = true;
        }
    }
    
    return retval;
}

static bool getLinkADRReq(struct lora_stream *s, struct lora_link_adr_req *value)
{
    bool retval = false;
    
    uint8_t buf;
    
    if(getU8(s, &buf)){
    
        value->dataRate = buf >> 4;
        value->txPower = buf & 0xfU;
    
        if(getU16(s, &value->channelMask)){
            
            if(getU8(s, &buf)){
            
                value->channelMaskControl = (buf >> 4) & 0x7U;
                value->nbTrans = buf & 0xfU;
                
                retval = true;                        
            }            
        }                    
    }
    
    return retval;
}

static bool getDutyCycleReq(struct lora_stream *s, struct lora_duty_cycle_req *value)
{
    bool retval = false;
    
    if(getU8(s, &value->maxDutyCycle)){
        
        value->maxDutyCycle = value->maxDutyCycle & 0xfU;        
        retval = true;
    }
    
    return retval;
}

static bool getRXParamSetupReq(struct lora_stream *s, struct lora_rx_param_setup_req *value)
{
    bool retval = false;
    
    if(getU8(s, &value->rx1DROffset)){
        
        if(getU24(s, &value->freq)){
        
            retval = true;        
        }       
    }
    
    return retval;    
}

static bool getNewChannelReq(struct lora_stream *s, struct lora_new_channel_req *value)
{
    bool retval = false;
    uint8_t buf;
    
    if(getU8(s, &value->chIndex)){
        
        if(getU24(s, &value->freq)){
                    
            if(getU8(s, &buf)){

                value->maxDR = buf >> 4;
                value->minDR = buf & 0xfU;                        
                
                retval = true;
            }
        }
    }
    
    return retval;
}

static bool getDLChannelReq(struct lora_stream *s, struct lora_dl_channel_req *value)
{
    bool retval = false;
    
    if(getU8(s, &value->chIndex)){
        
        if(getU24(s, &value->freq)){
        
            retval = true;                                
        }
    }
    
    return retval;
}

static bool getRXTimingSetupReq(struct lora_stream *s, struct lora_rx_timing_setup_req *value)
{
    bool retval = false;
    
    if(getU8(s, &value->delay)){
            
        value->delay &= 0xfU;        
        retval = true;
    }
    
    return retval;
}

static bool getTXParamSetupReq(struct lora_stream *s, struct lora_tx_param_setup_req *value)
{
    bool retval = false;
    uint8_t buf;
    
    if(getU8(s, &buf)){
            
        value->downlinkDwell = ((buf & 0x20U) == 0x20U); 
        value->uplinkDwell = ((buf & 0x10U) == 0x10U); 
        value->maxEIRP = buf & 0xfU; 
    
        retval = true;
    }
    
    return retval;
}

#if 0

static bool getPingSlotInfoReq(struct lora_stream *s, struct lora_ping_slot_info_req *value)
{
    bool retval = false;
    uint8_t buf;
    
    if(getU8(s, &buf)){
        
        value->periodicity = (buf >> 4U) & 0x7U;
        value->dataRate = buf & 0xfU;
        
        retval = true;
    }
    
    return retval
}

static bool getPingSlotChannelReq(struct lora_stream *s, struct lora_ping_slot_channel_req *value)
{
    bool retval = false;
    
    
    return retval
}

static bool getPingSlotFreqAns(struct lora_stream *s, struct lora_ping_slot_freq_ans *value)
{
    bool retval = false;
    
    
    return retval
}

static bool getBeaconTimingReq(struct lora_stream *s, struct lora_beacon_timing_req *value)
{
    bool retval = false;
    
    
    return retval
}

static bool getBeaconTimingAns(struct lora_stream *s, struct lora_beacon_timing_ans *value)
{
    bool retval = false;
    
    
    return retval
}

static bool getBeaconFreqReq(struct lora_stream *s, struct lora_beacon_freq_req *value)
{
    bool retval = false;
    
    
    return retval
}

static bool getBeaconFreqAns(struct lora_stream *s, struct lora_beacon_freq_ans *value)
{
    bool retval = false;
    
    
    return retval
}

#endif

static bool putU8(struct lora_stream *self, uint8_t in)
{
    return LDL_Stream_write(self, &in, sizeof(in));
}

static bool getU24(struct lora_stream *self, uint32_t *out)
{
    uint8_t buf[3U];
    bool retval;
    
    *out = 0U;
    
    retval = LDL_Stream_read(self, buf, sizeof(buf));
        
    *out |= buf[2];
    *out <<= 8;
    *out |= buf[1];
    *out <<= 8;
    *out |= buf[0];
    
    return retval;
}

static bool getU16(struct lora_stream *self, uint16_t *out)
{
    uint8_t buf[2U];
    bool retval;
    
    *out = 0U;
    
    retval = LDL_Stream_read(self, buf, sizeof(buf));
        
    *out |= buf[1];
    *out <<= 8;
    *out |= buf[0];
        
    return retval;
}

static bool getU8(struct lora_stream *self, uint8_t *out)
{
    return LDL_Stream_read(self, out, sizeof(*out));
}
