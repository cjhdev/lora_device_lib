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

#ifndef LORA_FRAME_H
#define LORA_FRAME_H

/** @file */

#ifdef __cplusplus
extern "C" {
#endif

#include "lora_platform.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* types **************************************************************/

enum lora_frame_type {
    FRAME_TYPE_JOIN_REQ = 0,
    FRAME_TYPE_JOIN_ACCEPT,
    FRAME_TYPE_DATA_UNCONFIRMED_UP,
    FRAME_TYPE_DATA_UNCONFIRMED_DOWN,
    FRAME_TYPE_DATA_CONFIRMED_UP,
    FRAME_TYPE_DATA_CONFIRMED_DOWN,    
};

struct lora_frame_data {
    
    uint32_t devAddr;
    uint16_t counter;
    bool ack;
    bool adr;
    bool adrAckReq;
    bool pending;

    const uint8_t *opts;
    uint8_t optsLen;

    uint8_t port;
    const uint8_t *data;
    uint8_t dataLen;
};

struct lora_frame_join_accept {
    
    uint32_t appNonce;
    uint32_t netID;
    uint32_t devAddr;
    uint8_t rx1DataRateOffset;
    uint8_t rx2DataRate;
    uint8_t rxDelay;
    
    uint32_t cfList[5U];
    enum { NO_CFLIST, FREQ_CFLIST, MASK_CFLIST } cfListType;    
};

struct lora_frame_join_request {
    
    uint8_t appEUI[8U];
    uint8_t devEUI[8U];
    uint16_t devNonce;    
};

struct lora_frame {

    enum lora_frame_type type;
    
    union {
        
        struct lora_frame_data data;
        struct lora_frame_join_accept joinAccept;
        struct lora_frame_join_request joinRequest;        
        
    } fields;
    
    /* true if MIC validated */
    bool valid;    
};

/* function prototypes ************************************************/

size_t LDL_Frame_putData(enum lora_frame_type type, const void *nwkSKey, const void *appSKey, const struct lora_frame_data *f, void *out, size_t max);
size_t LDL_Frame_putJoinRequest(const void *key, const struct lora_frame_join_request *f, void *out, size_t max);
size_t LDL_Frame_putJoinAccept(const void *key, const struct lora_frame_join_accept *f, void *out, size_t max);
bool LDL_Frame_decode(const void *appKey, const void *nwkSKey, const void *appSKey, void *in, size_t len, struct lora_frame *f);
size_t LDL_Frame_getPhyPayloadSize(size_t dataLen, size_t optsLen);
bool LDL_Frame_isUpstream(enum lora_frame_type type);
size_t LDL_Frame_phyOverhead(void);
size_t LDL_Frame_dataOverhead(void);

#ifdef __cplusplus
}
#endif

#endif
