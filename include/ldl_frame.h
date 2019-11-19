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

#ifndef LDL_FRAME_H
#define LDL_FRAME_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/* types **************************************************************/

enum ldl_frame_type {
    FRAME_TYPE_JOIN_REQ = 0,
    FRAME_TYPE_JOIN_ACCEPT,
    FRAME_TYPE_DATA_UNCONFIRMED_UP,
    FRAME_TYPE_DATA_UNCONFIRMED_DOWN,
    FRAME_TYPE_DATA_CONFIRMED_UP,
    FRAME_TYPE_DATA_CONFIRMED_DOWN,    
    FRAME_TYPE_REJOIN_REQ
};

struct ldl_frame_data {
    
    enum ldl_frame_type type;
    
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
    
    uint32_t mic;
};

struct ldl_frame_data_offset {

    uint8_t opts;
    uint8_t data;
};

enum ldl_frame_rejoin_type {
        
    LDL_REJOIN_TYPE_1,
    LDL_REJOIN_TYPE_2,
    LDL_REJOIN_TYPE_3
};

struct ldl_frame_rejoin_request {
    
    enum ldl_frame_rejoin_type type;
    uint32_t netID;
    const uint8_t *devEUI;
    uint16_t rjCount;    
    uint32_t mic;
};

struct ldl_frame_join_request {
    
    const uint8_t *joinEUI;
    const uint8_t *devEUI;
    uint16_t devNonce;    
    uint32_t mic;
};

struct ldl_frame_down {

    enum ldl_frame_type type;
    
    /* join accept */
    uint32_t joinNonce;
    uint32_t netID;
    uint32_t devAddr;
    uint8_t rx1DataRateOffset;
    uint8_t rx2DataRate;
    uint8_t rxDelay;
    bool optNeg;
    uint8_t *cfList;
    uint8_t cfListLen;
    
    /* data */
    /*uint32_t devAddr;*/
    uint16_t counter;
    bool ack;
    bool adr;
    bool adrAckReq;
    bool pending;

    uint8_t *opts;      /* NULL when not present */
    uint8_t optsLen;    /* 0..15; 0 when not present */

    bool dataPresent;   /* possible to have port without data */

    uint8_t port;       /* valid when dataPresent is true */
    
    uint8_t *data;      /* NULL when not present */
    uint8_t dataLen;    /* 0 when not present */
    
    uint32_t mic;    
};

/* function prototypes ************************************************/

void LDL_Frame_updateMIC(void *msg, uint8_t len, uint32_t mic);
uint8_t LDL_Frame_putData(const struct ldl_frame_data *f, void *out, uint8_t max, struct ldl_frame_data_offset *off);
uint8_t LDL_Frame_putJoinRequest(const struct ldl_frame_join_request *f, void *out, uint8_t max);
uint8_t LDL_Frame_putRejoinRequest(const struct ldl_frame_rejoin_request *f, void *out, uint8_t max);
bool LDL_Frame_decode(struct ldl_frame_down *f, void *in, uint8_t len);

uint8_t LDL_Frame_sizeofJoinAccept(bool withCFList);
uint8_t LDL_Frame_getPhyPayloadSize(uint8_t dataLen, uint8_t optsLen);
uint8_t LDL_Frame_phyOverhead(void);
uint8_t LDL_Frame_dataOverhead(void);

#ifdef __cplusplus
}
#endif

#endif
