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

#ifndef LORA_MAC_COMMANDS_H
#define LORA_MAC_COMMANDS_H

#include <stdint.h>
#include <stdbool.h>

struct lora_stream;

enum lora_mac_cmd_type {
    
    LINK_CHECK,
    LINK_ADR,
    DUTY_CYCLE,
    RX_PARAM_SETUP,
    DEV_STATUS,
    NEW_CHANNEL,
    RX_TIMING_SETUP,
    TX_PARAM_SETUP,
    DL_CHANNEL,
    
    PING_SLOT_INFO,
    PING_SLOT_CHANNEL,
    PING_SLOT_FREQ,
    BEACON_TIMING,
    BEACON_FREQ,
};

struct lora_link_check_ans {
                
    uint8_t margin;
    uint8_t gwCount;
};

struct lora_link_adr_req {
    
    uint8_t dataRate;
    uint8_t txPower;
    uint16_t channelMask;
    uint8_t channelMaskControl;
    uint8_t nbTrans;        
};

struct lora_link_adr_ans {
    
    bool powerOK;
    bool dataRateOK;
    bool channelMaskOK;
};

struct lora_duty_cycle_req {
    
    uint8_t maxDutyCycle;
};

struct lora_rx_param_setup_req {
    
    uint8_t rx1DROffset;
    uint8_t rx2DataRate;
    uint32_t freq;
};

struct lora_rx_param_setup_ans {
    
    bool rx1DROffsetOK;
    bool rx2DataRateOK;
    bool channelOK;
};

struct lora_dev_status_ans {
    
    uint8_t battery;
    uint8_t margin;
};

struct lora_new_channel_req {
    
    uint8_t chIndex;
    uint32_t freq;
    uint8_t maxDR;
    uint8_t minDR;
};
    
struct lora_new_channel_ans {
    
    bool dataRateRangeOK;
    bool channelFrequencyOK;
};

struct lora_dl_channel_req {
    
    uint8_t chIndex;
    uint32_t freq;    
};

struct lora_dl_channel_ans {
    
    bool uplinkFreqOK;
    bool channelFrequencyOK;
};

struct lora_rx_timing_setup_req {
    
    uint8_t delay;
};

struct lora_tx_param_setup_req {
    
    bool downlinkDwell;
    bool uplinkDwell;
    uint8_t maxEIRP;
};

struct lora_downstream_cmd {
  
    enum lora_mac_cmd_type type;
  
    union {
      
        struct lora_link_check_ans linkCheckAns;
        struct lora_link_adr_req linkADRReq;
        struct lora_duty_cycle_req dutyCycleReq;
        struct lora_rx_param_setup_req rxParamSetupReq;
        /* dev_status_req */
        struct lora_new_channel_req newChannelReq;
        struct lora_dl_channel_req dlChannelReq;
        struct lora_rx_timing_setup_req rxTimingSetupReq;
        struct lora_tx_param_setup_req txParamSetupReq;
        
    } fields;    
};

struct lora_upstream_cmd {
  
    enum lora_mac_cmd_type type;
  
    union {
      
        struct lora_link_adr_ans linkADRAns;
        /* duty_cycle_ans */        
        struct lora_rx_param_setup_ans rxParamSetupAns;
        struct lora_dev_status_ans devStatusAns;
        struct lora_new_channel_ans newChannelAns;
        struct lora_dl_channel_ans dlChannelAns;
        /* rx_timing_setup_ans */
        /* tx_param_setup_ans */
        
    } fields;    
};

uint8_t LDL_MAC_sizeofCommandUp(enum lora_mac_cmd_type type);
bool LDL_MAC_putLinkCheckReq(struct lora_stream *s);
bool LDL_MAC_putLinkCheckAns(struct lora_stream *s, const struct lora_link_check_ans *value);
bool LDL_MAC_putLinkADRReq(struct lora_stream *s, const struct lora_link_adr_req *value);
bool LDL_MAC_putLinkADRAns(struct lora_stream *s, const struct lora_link_adr_ans *value);
bool LDL_MAC_putDutyCycleReq(struct lora_stream *s, const struct lora_duty_cycle_req *value);
bool LDL_MAC_putDutyCycleAns(struct lora_stream *s);
bool LDL_MAC_putRXParamSetupReq(struct lora_stream *s, const struct lora_rx_param_setup_req *value);
bool LDL_MAC_putDevStatusReq(struct lora_stream *s);
bool LDL_MAC_putDevStatusAns(struct lora_stream *s, const struct lora_dev_status_ans *value);
bool LDL_MAC_putNewChannelReq(struct lora_stream *s, const struct lora_new_channel_req *value);
bool LDL_MAC_putRXParamSetupAns(struct lora_stream *s, const struct lora_rx_param_setup_ans *value);
bool LDL_MAC_putNewChannelAns(struct lora_stream *s, const struct lora_new_channel_ans *value);
bool LDL_MAC_putDLChannelReq(struct lora_stream *s, const struct lora_dl_channel_req *value);
bool LDL_MAC_putDLChannelAns(struct lora_stream *s, const struct lora_dl_channel_ans *value);
bool LDL_MAC_putRXTimingSetupReq(struct lora_stream *s, const struct lora_rx_timing_setup_req *value);
bool LDL_MAC_putRXTimingSetupAns(struct lora_stream *s);
bool LDL_MAC_putTXParamSetupReq(struct lora_stream *s, const struct lora_tx_param_setup_req *value);
bool LDL_MAC_putTXParamSetupAns(struct lora_stream *s);
bool LDL_MAC_getDownCommand(struct lora_stream *s, struct lora_downstream_cmd *cmd);
bool LDL_MAC_getUpCommand(struct lora_stream *s, struct lora_upstream_cmd *cmd);
bool LDL_MAC_peekNextCommand(struct lora_stream *s, enum lora_mac_cmd_type *type);

#endif
