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

#include "ldl_mac_commands.h"
#include "ldl_stream.h"
#include "ldl_debug.h"

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
    {11U, LDL_CMD_REKEY},
    {12U, LDL_CMD_ADR_PARAM_SETUP},
    {13U, LDL_CMD_DEVICE_TIME},
    {14U, LDL_CMD_FORCE_REJOIN},
    {15U, LDL_CMD_REJOIN_PARAM_SETUP}   
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
    default:      
        break;
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
    (void)LDL_Stream_putU8(s, value->margin);           
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
    (void)LDL_Stream_putU8(s, typeToTag(LDL_CMD_REJOIN_PARAM_SETUP));
    (void)LDL_Stream_putU8(s, value->timeOK);
}

bool LDL_MAC_getDownCommand(struct ldl_stream *s, struct ldl_downstream_cmd *cmd)
{
    uint8_t tag;
    
    if(LDL_Stream_getU8(s, &tag)){
        
        if(tagToType(tag, &cmd->type)){
            
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
            
                (void)LDL_Stream_getU8(s, &cmd->fields.rxParamSetup.rx1DROffset);
                (void)LDL_Stream_getU24(s, &cmd->fields.rxParamSetup.freq);                 
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
            }
                break;
                
            case LDL_CMD_DL_CHANNEL:
            
                (void)LDL_Stream_getU8(s, &cmd->fields.dlChannel.chIndex);
                (void)LDL_Stream_getU24(s, &cmd->fields.dlChannel.freq);     
                break;
            
            case LDL_CMD_RX_TIMING_SETUP:
            
                (void)LDL_Stream_getU8(s, &cmd->fields.rxTimingSetup.delay);
                cmd->fields.rxTimingSetup.delay &= 0xfU;   
                break;
            
            case LDL_CMD_TX_PARAM_SETUP:
            {
                uint8_t buf;
                
                (void)LDL_Stream_getU8(s, &buf);
                
                cmd->fields.txParamSetup.downlinkDwell = ((buf & 0x20U) == 0x20U); 
                cmd->fields.txParamSetup.uplinkDwell = ((buf & 0x10U) == 0x10U); 
                cmd->fields.txParamSetup.maxEIRP = buf & 0xfU; 
            }
                break;
            
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
            
            case LDL_CMD_DEVICE_TIME:
            
                (void)LDL_Stream_getU32(s, &cmd->fields.deviceTime.seconds);
                (void)LDL_Stream_getU8(s, &cmd->fields.deviceTime.fractions);
                break;
            
            case LDL_CMD_FORCE_REJOIN:
            {
                uint16_t buf;
    
                (void)LDL_Stream_getU16(s, &buf);
                
                cmd->fields.forceRejoin.period = (buf >> 10) & 0x7U;
                cmd->fields.forceRejoin.max_retries = (buf >> 7) & 0x7U;
                cmd->fields.forceRejoin.rejoin_type = (buf >> 4) & 0x7U;
                cmd->fields.forceRejoin.dr = buf & 0xfU;
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
            }
        }
    }
    
    return !LDL_Stream_error(s);
}

/* static functions ***************************************************/

static uint8_t typeToTag(enum ldl_mac_cmd_type type)
{
    return tags[type].tag;
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
