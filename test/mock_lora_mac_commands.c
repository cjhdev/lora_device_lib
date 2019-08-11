#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "cmocka.h"
#include "lora_mac_commands.h"

bool LDL_MAC_putLinkCheckReq(struct lora_stream *s)
{
    return mock_type(bool);
}

bool LDL_MAC_putLinkCheckAns(struct lora_stream *s, const struct lora_link_check_ans *value)
{
    return mock_type(bool);
}

bool LDL_MAC_putLinkADRReq(struct lora_stream *s, const struct lora_link_adr_req *value)
{
    return mock_type(bool);
}

bool LDL_MAC_putLinkADRAns(struct lora_stream *s, const struct lora_link_adr_ans *value)
{
    return mock_type(bool);
}

bool LDL_MAC_putDutyCycleReq(struct lora_stream *s, const struct lora_duty_cycle_req *value)
{
    return mock_type(bool);
}

bool LDL_MAC_putDutyCycleAns(struct lora_stream *s)
{
    return mock_type(bool);
}

bool LDL_MAC_putRXParamSetupReq(struct lora_stream *s, const struct lora_rx_param_setup_req *value)
{
    return mock_type(bool);
}

bool LDL_MAC_putDevStatusReq(struct lora_stream *s)
{
    return mock_type(bool);
}

bool LDL_MAC_putDevStatusAns(struct lora_stream *s, const struct lora_dev_status_ans *value)
{
    return mock_type(bool);
}

bool LDL_MAC_putNewChannelReq(struct lora_stream *s, const struct lora_new_channel_req *value)
{
    return mock_type(bool);
}

bool LDL_MAC_putRXParamSetupAns(struct lora_stream *s, const struct lora_rx_param_setup_ans *value)
{
    return mock_type(bool);
}

bool LDL_MAC_putNewChannelAns(struct lora_stream *s, const struct lora_new_channel_ans *value)
{
    return mock_type(bool);
}

bool LDL_MAC_putDLChannelReq(struct lora_stream *s, const struct lora_dl_channel_req *value)
{
    return mock_type(bool);
}

bool LDL_MAC_putDLChannelAns(struct lora_stream *s, const struct lora_dl_channel_ans *value)
{
    return mock_type(bool);
}

bool LDL_MAC_putRXTimingSetupReq(struct lora_stream *s, const struct lora_rx_timing_setup_req *value)
{
    return mock_type(bool);
}

bool LDL_MAC_putRXTimingSetupAns(struct lora_stream *s)
{
    return mock_type(bool);
}

bool LDL_MAC_putTXParamSetupReq(struct lora_stream *s, const struct lora_tx_param_setup_req *value)
{
    return mock_type(bool);
}

bool LDL_MAC_putTXParamSetupAns(struct lora_stream *s)
{
    return mock_type(bool);
}

bool LDL_MAC_getDownCommand(struct lora_stream *s, struct lora_downstream_cmd *cmd)
{
    return mock_type(bool);
}

bool LDL_MAC_getUpCommand(struct lora_stream *s, struct lora_upstream_cmd *cmd)
{
    return mock_type(bool);
}

bool LDL_MAC_peekNextCommand(struct lora_stream *s, enum lora_mac_cmd_type *type)
{
    return mock_type(bool);
}

uint8_t LDL_MAC_sizeofCommandUp(enum lora_mac_cmd_type type)
{
    return mock();
}
