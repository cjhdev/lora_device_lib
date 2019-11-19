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

#ifndef LDL_MAC_COMMANDS_H
#define LDL_MAC_COMMANDS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

struct ldl_stream;

enum ldl_mac_cmd_type {
    
    LDL_CMD_LINK_CHECK,
    LDL_CMD_LINK_ADR,
    LDL_CMD_DUTY_CYCLE,
    LDL_CMD_RX_PARAM_SETUP,
    LDL_CMD_DEV_STATUS,
    LDL_CMD_NEW_CHANNEL,
    LDL_CMD_RX_TIMING_SETUP,
    LDL_CMD_TX_PARAM_SETUP,
    LDL_CMD_DL_CHANNEL,
    
    LDL_CMD_REKEY,
    LDL_CMD_ADR_PARAM_SETUP,
    LDL_CMD_DEVICE_TIME,
    LDL_CMD_FORCE_REJOIN,
    LDL_CMD_REJOIN_PARAM_SETUP
};

struct ldl_link_check_ans {
                
    uint8_t margin;
    uint8_t gwCount;
};

struct ldl_link_adr_req {
    
    uint8_t dataRate;
    uint8_t txPower;
    uint16_t channelMask;
    uint8_t channelMaskControl;
    uint8_t nbTrans;        
};

struct ldl_link_adr_ans {
    
    bool powerOK;
    bool dataRateOK;
    bool channelMaskOK;
};

struct ldl_duty_cycle_req {
    
    uint8_t maxDutyCycle;
};

struct ldl_rx_param_setup_req {
    
    uint8_t rx1DROffset;
    uint8_t rx2DataRate;
    uint32_t freq;
};

struct ldl_rx_param_setup_ans {
    
    bool rx1DROffsetOK;
    bool rx2DataRateOK;
    bool channelOK;
};

struct ldl_dev_status_ans {
    
    uint8_t battery;
    uint8_t margin;
};

struct ldl_new_channel_req {
    
    uint8_t chIndex;
    uint32_t freq;
    uint8_t maxDR;
    uint8_t minDR;
};
    
struct ldl_new_channel_ans {
    
    bool dataRateRangeOK;
    bool channelFreqOK;
};

struct ldl_dl_channel_req {
    
    uint8_t chIndex;
    uint32_t freq;    
};

struct ldl_dl_channel_ans {
    
    bool uplinkFreqOK;
    bool channelFreqOK;
};

struct ldl_rx_timing_setup_req {
    
    uint8_t delay;
};

struct ldl_tx_param_setup_req {
    
    bool downlinkDwell;
    bool uplinkDwell;
    uint8_t maxEIRP;
};

struct ldl_rekey_ind {
    
    uint8_t version;
};

struct ldl_rekey_conf {
    
    uint8_t version;
};

struct ldl_adr_param_setup_req {
    
    uint8_t limit_exp;
    uint8_t delay_exp;
};

struct ldl_device_time_ans {
    
    uint32_t seconds;
    uint8_t fractions;
};

struct ldl_force_rejoin_req {
    
    uint8_t period;
    uint8_t max_retries;
    uint8_t rejoin_type;
    uint8_t dr;
};

struct ldl_rejoin_param_setup_req {
  
    uint8_t maxTimeN;
    uint8_t maxCountN;
};

struct ldl_rejoin_param_setup_ans {
    
    uint8_t timeOK;
};

struct ldl_downstream_cmd {
  
    enum ldl_mac_cmd_type type;
  
    union {
      
        struct ldl_link_check_ans linkCheck;
        struct ldl_link_adr_req linkADR;
        struct ldl_duty_cycle_req dutyCycle;
        struct ldl_rx_param_setup_req rxParamSetup;
        /* dev_status_req */
        struct ldl_new_channel_req newChannel;
        struct ldl_dl_channel_req dlChannel;
        struct ldl_rx_timing_setup_req rxTimingSetup;
        struct ldl_tx_param_setup_req txParamSetup;        
        struct ldl_rekey_conf rekey;
        struct ldl_adr_param_setup_req adrParamSetup;
        struct ldl_device_time_ans deviceTime;
        struct ldl_force_rejoin_req forceRejoin;
        struct ldl_rejoin_param_setup_req rejoinParamSetup;        
        
    } fields;    
};

struct ldl_upstream_cmd {
  
    enum ldl_mac_cmd_type type;
  
    union {
      
        struct ldl_link_adr_ans linkADR;
        /* duty_cycle_ans */        
        struct ldl_rx_param_setup_ans rxParamSetup;
        struct ldl_dev_status_ans devStatus;
        struct ldl_new_channel_ans newChannel;
        struct ldl_dl_channel_ans dlChannel;
        /* rx_timing_setup_ans */
        /* tx_param_setup_ans */
        struct ldl_rekey_ind rekey;
        struct ldl_rejoin_param_setup_ans rejoinParamSetup;
        
    } fields;    
};


void LDL_MAC_putLinkCheckReq(struct ldl_stream *s);
void LDL_MAC_putLinkCheckAns(struct ldl_stream *s, const struct ldl_link_check_ans *value);
void LDL_MAC_putLinkADRReq(struct ldl_stream *s, const struct ldl_link_adr_req *value);
void LDL_MAC_putLinkADRAns(struct ldl_stream *s, const struct ldl_link_adr_ans *value);
void LDL_MAC_putDutyCycleReq(struct ldl_stream *s, const struct ldl_duty_cycle_req *value);
void LDL_MAC_putDutyCycleAns(struct ldl_stream *s);
void LDL_MAC_putRXParamSetupReq(struct ldl_stream *s, const struct ldl_rx_param_setup_req *value);
void LDL_MAC_putDevStatusReq(struct ldl_stream *s);
void LDL_MAC_putDevStatusAns(struct ldl_stream *s, const struct ldl_dev_status_ans *value);
void LDL_MAC_putNewChannelReq(struct ldl_stream *s, const struct ldl_new_channel_req *value);
void LDL_MAC_putRXParamSetupAns(struct ldl_stream *s, const struct ldl_rx_param_setup_ans *value);
void LDL_MAC_putNewChannelAns(struct ldl_stream *s, const struct ldl_new_channel_ans *value);
void LDL_MAC_putDLChannelReq(struct ldl_stream *s, const struct ldl_dl_channel_req *value);
void LDL_MAC_putDLChannelAns(struct ldl_stream *s, const struct ldl_dl_channel_ans *value);
void LDL_MAC_putRXTimingSetupReq(struct ldl_stream *s, const struct ldl_rx_timing_setup_req *value);
void LDL_MAC_putRXTimingSetupAns(struct ldl_stream *s);
void LDL_MAC_putTXParamSetupReq(struct ldl_stream *s, const struct ldl_tx_param_setup_req *value);
void LDL_MAC_putTXParamSetupAns(struct ldl_stream *s);
void LDL_MAC_putRekeyInd(struct ldl_stream *s, const struct ldl_rekey_ind *value);
void LDL_MAC_putADRParamSetupAns(struct ldl_stream *s);
void LDL_MAC_putDeviceTimeReq(struct ldl_stream *s);
void LDL_MAC_putRejoinParamSetupAns(struct ldl_stream *s, struct ldl_rejoin_param_setup_ans *value);

bool LDL_MAC_getDownCommand(struct ldl_stream *s, struct ldl_downstream_cmd *cmd);
bool LDL_MAC_getUpCommand(struct ldl_stream *s, struct ldl_upstream_cmd *cmd);

bool LDL_MAC_peekNextCommand(struct ldl_stream *s, enum ldl_mac_cmd_type *type);

uint8_t LDL_MAC_sizeofCommandUp(enum ldl_mac_cmd_type type);

#ifdef __cplusplus
}
#endif

#endif
