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

#include "lora_aes.h"
#include "lora_cmac.h"
#include "lora_frame.h"
#include "lora_debug.h"
#include <stddef.h>
#include <string.h>

#include <stdio.h>

/* static function prototypes *****************************************/

static void cipherData(enum lora_frame_type type, const uint8_t *key, uint32_t devAddr, uint16_t counter, uint8_t *data, size_t len);

static uint32_t cmacData(enum lora_frame_type type, const uint8_t *key, uint32_t devAddr, uint16_t counter, const uint8_t *msg, size_t len);
static uint32_t cmacJoin(const uint8_t *key, const uint8_t *msg, size_t len);

static void xor128(uint8_t *acc, const uint8_t *op);

#ifndef LORA_DISABLE_FULL_CODEC
static size_t getEUI(const uint8_t *in, size_t max, uint8_t *value);
static size_t putU24(uint8_t *out, size_t max, uint32_t value);
#endif
static size_t getU32(const uint8_t *in, size_t max, uint32_t *value);
static size_t getU24(const uint8_t *in, size_t max, uint32_t *value);
static size_t getU16(const uint8_t *in, size_t max, uint16_t *value);
static size_t getU8(const uint8_t *in, size_t max, uint8_t *value);
static size_t putEUI(uint8_t *out, size_t max, const uint8_t *value);
static size_t putU32(uint8_t *out, size_t max, uint32_t value);
static size_t putU16(uint8_t *out, size_t max, uint16_t value);
static size_t putU8(uint8_t *out, size_t max, uint8_t value);

/* functions **********************************************************/


size_t LDL_Frame_putData(enum lora_frame_type type, const void *nwkSKey, const void *appSKey, const struct lora_frame_data *f, void *out, size_t max)
{
    size_t pos = 0U;
    uint8_t *ptr = (uint8_t *)out;
    
    if(f->optsLen <= 0xfU){
        
        if((6U + (size_t)f->optsLen + 3U + (size_t)f->dataLen + 4U) <= max){

            pos += putU8(&ptr[pos], max - pos, ((uint8_t)type) << 5);            
            pos += putU32(&ptr[pos], max - pos, f->devAddr);            
            pos += putU8(&ptr[pos], max - pos, (f->adr ? 0x80U : 0U) | (f->adrAckReq ? 0x40U : 0U) | (f->ack ? 0x20U : 0U) | (f->pending ? 0x10U : 0U) | (f->optsLen & 0xfU));
            pos += putU16(&ptr[pos], max - pos, f->counter);            
            (void)memcpy(&ptr[pos], f->opts, f->optsLen);
            pos += f->optsLen;

            if(f->data != NULL){

                pos += putU8(&ptr[pos], max - pos, f->port);

                (void)memcpy(&ptr[pos], f->data, f->dataLen);
                cipherData(type, (f->port == 0U) ? nwkSKey : appSKey, f->devAddr, f->counter, &ptr[pos], f->dataLen);
                pos += f->dataLen;                                
            }

            pos += putU32(&ptr[pos], max - pos, cmacData(type, nwkSKey, f->devAddr, f->counter, ptr, pos));            
        }
        else{

            LORA_INFO("frame size is too large")
        }
    }
    else{

        LORA_INFO("foptslen must be in range (0..15)")
    }
    
    return pos;
}

size_t LDL_Frame_putJoinRequest(const void *key, const struct lora_frame_join_request *f, void *out, size_t max)
{
    size_t pos = 0U;    
    uint8_t *ptr = (uint8_t *)out;
    
    if(max >= 23U){
    
        pos += putU8(&ptr[pos], max - pos, ((uint8_t)FRAME_TYPE_JOIN_REQ) << 5);
        pos += putEUI(&ptr[pos], max - pos, f->appEUI);
        pos += putEUI(&ptr[pos], max - pos, f->devEUI);
        pos += putU16(&ptr[pos], max - pos, f->devNonce);            
        pos += putU32(&ptr[pos], max - pos, cmacJoin(key, out, pos));        
    }
    else{
        
        LORA_INFO("buffer too short for join request message")
    }
    
    return pos;
}

#ifndef LORA_DISABLE_FULL_CODEC
size_t LDL_Frame_putJoinAccept(const void *key, const struct lora_frame_join_accept *f, void *out, size_t max)
{
    size_t i;
    size_t pos = 0U;    
    uint8_t *ptr = (uint8_t *)out;
    struct lora_aes_ctx aes_ctx;
    
    if(max >= (17U + ( (f->cfListType != NO_CFLIST) ? 16U : 0U ))){
    
        pos += putU8(&ptr[pos], max - pos, ((uint8_t)FRAME_TYPE_JOIN_ACCEPT) << 5);
        pos += putU24(&ptr[pos], max - pos, f->appNonce);
        pos += putU24(&ptr[pos], max - pos, f->netID);
        pos += putU32(&ptr[pos], max - pos, f->devAddr);
        pos += putU8(&ptr[pos], max - pos, (f->rx1DataRateOffset << 4) | (f->rx2DataRate & 0xfU));
        pos += putU8(&ptr[pos], max - pos, f->rxDelay);
        
        switch(f->cfListType){
        default:
        case NO_CFLIST:
            break;
        case FREQ_CFLIST:
        
            for(i=0U; i < sizeof(f->cfList)/sizeof(*f->cfList); i++){
                
                pos += putU24(&ptr[pos], max - pos, f->cfList[i] / 100U);
            }            
            
            pos += putU8(&ptr[pos], max - pos, 0U);
            break;
        
        case MASK_CFLIST:
        
            for(i=0U; i < sizeof(f->cfList)/sizeof(*f->cfList); i++){
                
                pos += putU16(&ptr[pos], max - pos, (uint16_t)f->cfList[i]);
            }            
            
            (void)memset(&ptr[pos], 0U, 5U);
            pos += 5U;
            
            pos += putU8(&ptr[pos], max - pos, 1U);
            break;
        }
        
        pos += putU32(&ptr[pos], max - pos, cmacJoin(key, ptr, pos));
    
        LDL_AES_init(&aes_ctx, key);
        LDL_AES_decrypt(&aes_ctx, &ptr[1]);
        
        if(f->cfListType != NO_CFLIST){
            
            LDL_AES_decrypt(&aes_ctx, &ptr[17]);
        }    
    }
    else{
        
        LORA_INFO("buffer too short for join accept message")
    }
    
    return pos;    
}
#endif

bool LDL_Frame_decode(const void *appKey, const void *nwkSKey, const void *appSKey, void *in, size_t len, struct lora_frame *f)
{
    static const enum lora_frame_type types[] = {
        FRAME_TYPE_JOIN_REQ,
        FRAME_TYPE_JOIN_ACCEPT,
        FRAME_TYPE_DATA_UNCONFIRMED_UP,
        FRAME_TYPE_DATA_UNCONFIRMED_DOWN,
        FRAME_TYPE_DATA_CONFIRMED_UP,
        FRAME_TYPE_DATA_CONFIRMED_DOWN,
    };

    uint8_t *ptr = (uint8_t *)in;
    size_t pos = 0U;
    uint32_t mic = 0U;        
    bool retval = false;
    size_t i;

    (void)memset(f, 0, sizeof(*f));
    
    if(len == 0U){
        
        LORA_INFO("frame too short")
    }
    else{
        
        uint8_t tag;
    
        pos += getU8(&ptr[pos], len - pos, &tag);
        
        if((tag & 0x1fU) != 0U){
            
            LORA_INFO("unsupported MHDR")
        }
        else if((tag >> 5) >= (uint8_t)(sizeof(types)/sizeof(*types))){
            
            LORA_INFO("unknown frame type")
        }
        else{

            f->type = types[(tag >> 5)];

            switch(f->type){
            default:        
            case FRAME_TYPE_JOIN_REQ:

#ifdef LORA_DISABLE_FULL_CODEC
                LORA_INFO("device does not need to decode a join-request")
#else                            
                if(len >= 23U){

                    pos += getEUI(&ptr[pos], len - pos, f->fields.joinRequest.appEUI);
                    pos += getEUI(&ptr[pos], len - pos, f->fields.joinRequest.devEUI);
                    pos += getU16(&ptr[pos], len - pos, &f->fields.joinRequest.devNonce);
                    pos += getU32(&ptr[pos], len - pos, &mic);

                    f->valid = (mic == cmacJoin(appKey, ptr, pos - sizeof(mic)));                    
                    
                    retval = true;
                }
                else{
                    
                    LORA_INFO("unexpected frame length for join request")
                }
#endif                
                break;
    
            case FRAME_TYPE_JOIN_ACCEPT:
            
                if(((len-pos) == 16U) || ((len-pos) == 32U)){
                    
                    uint8_t dlSettings = 0U;
                    struct lora_aes_ctx aes_ctx;   
                                 
                    LDL_AES_init(&aes_ctx, appKey);
                    LDL_AES_encrypt(&aes_ctx, &ptr[pos]);
                    if((len-pos) == 32U){                        
                        
                        LDL_AES_encrypt(&aes_ctx, &ptr[pos+16U]);
                    }
                    
                    pos += getU24(&ptr[pos], len - pos, &f->fields.joinAccept.appNonce);
                    pos += getU24(&ptr[pos], len - pos, &f->fields.joinAccept.netID);
                    pos += getU32(&ptr[pos], len - pos, &f->fields.joinAccept.devAddr);
                    pos += getU8(&ptr[pos], len - pos, &dlSettings);
                    pos += (getU8(&ptr[pos], len - pos, &f->fields.joinAccept.rxDelay) & 0xfU);

                    f->fields.joinAccept.rx1DataRateOffset = (dlSettings >> 4) & 0xfU;
                    f->fields.joinAccept.rx2DataRate = dlSettings & 0xfU;

                    f->fields.joinAccept.rxDelay = ( f->fields.joinAccept.rxDelay == 0U) ? 1U : f->fields.joinAccept.rxDelay;
                    
                    if((len - pos) > sizeof(mic)){
            
                        switch(ptr[pos + 15]){
                        case 0U:
                        
                            for(i=0U; i < sizeof(f->fields.joinAccept.cfList)/sizeof(*f->fields.joinAccept.cfList); i++){
                            
                                pos += getU24(&ptr[pos], len - pos, &f->fields.joinAccept.cfList[i]);
                                f->fields.joinAccept.cfList[i] *= 100U;
                            }
                            f->fields.joinAccept.cfListType = FREQ_CFLIST;
                            pos++;
                            break;                            
                        
                        case 1U:
                        
                            for(i=0U; i < sizeof(f->fields.joinAccept.cfList)/sizeof(*f->fields.joinAccept.cfList); i++){
                            
                                uint16_t mask = 0U;
                            
                                pos += getU16(&ptr[pos], len - pos, &mask);
                                f->fields.joinAccept.cfList[i] = mask;
                            }
                            f->fields.joinAccept.cfListType = MASK_CFLIST;
                            pos++;
                            pos++;
                            pos++;
                            pos++;
                            pos++;
                            pos++;
                            break;
                        
                        default:
                            LORA_INFO("unknown cflist type")
                            break;
                        }
                    }
                    
                    pos += getU32(&ptr[pos], len - pos, &mic);
                    
                    f->valid = (mic == cmacJoin(appKey, ptr, pos - sizeof(mic)));                    
                    
                    retval = true;
                }
                else{
                    
                    LORA_INFO("unexpected frame length for join accept")
                }                
                break;
    
            case FRAME_TYPE_DATA_UNCONFIRMED_UP:
            case FRAME_TYPE_DATA_UNCONFIRMED_DOWN:
            case FRAME_TYPE_DATA_CONFIRMED_UP:
            case FRAME_TYPE_DATA_CONFIRMED_DOWN:

                if((len-pos) >= (4U + 1U + 2U + 4U)){
            
                    uint8_t fhdr = 0U;
                    const uint8_t *key;

                    pos += getU32(&ptr[pos], len - pos, &f->fields.data.devAddr);
                    pos += getU8(&ptr[pos], len - pos, &fhdr);
                    
                    f->fields.data.adr = ((fhdr & 0x80U) > 0U) ? true : false;
                    f->fields.data.adrAckReq = ((fhdr & 0x40U) > 0U) ? true : false;
                    f->fields.data.ack = ((fhdr & 0x20U) > 0U) ? true : false;
                    f->fields.data.pending = ((fhdr & 0x10U) > 0U) ? true : false;
                    f->fields.data.optsLen = fhdr & 0xfU;
                    
                    pos += getU16(&ptr[pos], len - pos, &f->fields.data.counter);
                    
                    f->fields.data.opts = (f->fields.data.optsLen > 0U) ? &ptr[pos] : NULL; 
                    
                    if((len-pos) >= (f->fields.data.optsLen + 4U)){
                    
                        pos += f->fields.data.optsLen;
                        
                        if((len-pos) > sizeof(mic)){

                            pos += getU8(&ptr[pos], len - pos, &f->fields.data.port);
                            
                            f->fields.data.data = &ptr[pos];
                            f->fields.data.dataLen = (uint8_t)(len - (pos + sizeof(mic)));
                            pos += f->fields.data.dataLen;                            
                        }
                        
                        pos += getU32(&ptr[pos], len - pos, &mic);
                        
                        key = ((f->fields.data.data != NULL) && (f->fields.data.port != 0U)) ? appSKey : nwkSKey;
                        
                        f->valid = (cmacData(f->type, nwkSKey, f->fields.data.devAddr, f->fields.data.counter, ptr, pos - sizeof(mic)) == mic);
                        
                        cipherData(f->type, key, f->fields.data.devAddr, f->fields.data.counter, &ptr[pos - f->fields.data.dataLen - sizeof(mic)], f->fields.data.dataLen);
        
                        /* see spec 4.3.1.6 Frame options (FOptsLen in FCtrl, FOpts) */
                        if((f->fields.data.data != NULL) && (f->fields.data.optsLen > 0) && (f->fields.data.port == 0U)){
                            
                            LORA_INFO("cannot have options and port 0")                            
                        }
                        else{
                            
                            retval = true;
                        }                                
                    }
                    else{
                        
                        LORA_INFO("frame too short")            
                    }
                }
                else{
                    
                    LORA_INFO("frame too short")            
                }
                break;
            }
        }
    }
            
    return retval;
}

bool LDL_Frame_isUpstream(enum lora_frame_type type)
{
    bool retval;
    
    switch(type){
    case FRAME_TYPE_JOIN_REQ:
    case FRAME_TYPE_DATA_UNCONFIRMED_UP:
    case FRAME_TYPE_DATA_CONFIRMED_UP:
        retval = true;
        break;
    case FRAME_TYPE_JOIN_ACCEPT:    
    case FRAME_TYPE_DATA_UNCONFIRMED_DOWN:    
    case FRAME_TYPE_DATA_CONFIRMED_DOWN:
    default:
        retval = false;
        break;
    }
    
    return retval;
}

size_t LDL_Frame_dataOverhead(void)
{
    /* DevAddr + FCtrl + FCnt + FOpts + FPort */
    return (4U + 1U + 2U) + (1U);
}

size_t LDL_Frame_phyOverhead(void)
{
    /* MHDR + MIC */
    return 1U + 4U;
}


/* static functions ***************************************************/

static void cipherData(enum lora_frame_type type, const uint8_t *key, uint32_t devAddr, uint16_t counter, uint8_t *data, size_t len)
{
    struct lora_aes_ctx ctx;
    uint8_t a[16];
    uint8_t s[16];
    uint8_t pld[16];
    size_t k = (len / 16U) + (((len % 16U) != 0U) ? 1U : 0U);
    size_t i;
    size_t pos = 0U;
    size_t size;

    a[0] = 1U;
    a[1] = 0U;
    a[2] = 0U;
    a[3] = 0U;
    a[4] = 0U;
    a[5] = (LDL_Frame_isUpstream(type) ? 0U : 1U);
    a[6] = (uint8_t)devAddr;
    a[7] = (uint8_t)(devAddr >> 8);
    a[8] = (uint8_t)(devAddr >> 16);
    a[9] = (uint8_t)(devAddr >> 24);
    a[10] = (uint8_t)counter;
    a[11] = (uint8_t)(counter >> 8);
    a[12] = 0U;
    a[13] = 0U;
    a[14] = 0U;
    a[15] = 0U;

    LDL_AES_init(&ctx, key);

    for(i=0; i < k; i++){

        size = (i == (k-1U)) ? (len % sizeof(a)) : sizeof(a);

        xor128(pld, pld);
        
        (void)memcpy(pld, &data[pos], size);
        
        a[15] = (uint8_t)(i+1U);

        (void)memcpy(s, a, sizeof(s));
        LDL_AES_encrypt(&ctx, s);

        xor128(pld, s);

        (void)memcpy(&data[pos], pld, size);
        pos += sizeof(a);
    }
}

static uint32_t cmacData(enum lora_frame_type type, const uint8_t *key, uint32_t devAddr, uint16_t counter, const uint8_t *msg, size_t len)
{
    uint8_t b[16];
    struct lora_aes_ctx aes_ctx;
    struct lora_cmac_ctx ctx;
    uint32_t mic;
    
    b[0] = 0x49U;
    b[1] = 0U;
    b[2] = 0U;
    b[3] = 0U;
    b[4] = 0U;
    b[5] = (LDL_Frame_isUpstream(type) ? 0U : 1U);
    b[6] = (uint8_t)devAddr;
    b[7] = (uint8_t)(devAddr >> 8);
    b[8] = (uint8_t)(devAddr >> 16);
    b[9] = (uint8_t)(devAddr >> 24);
    b[10] = (uint8_t)counter;
    b[11] = (uint8_t)(counter >> 8);
    b[12] = 0U;
    b[13] = 0U;
    b[14] = 0U;
    b[15] = (uint8_t)len;

    LDL_AES_init(&aes_ctx, key);
    LDL_CMAC_init(&ctx, &aes_ctx);
    LDL_CMAC_update(&ctx, b, sizeof(b));
    LDL_CMAC_update(&ctx, msg, len);
    LDL_CMAC_finish(&ctx, b, sizeof(mic));
    
    (void)getU32(b, sizeof(mic), &mic);
    
    return mic;
}

static uint32_t cmacJoin(const uint8_t *key, const uint8_t *msg, size_t len)
{
    struct lora_aes_ctx aes_ctx;
    struct lora_cmac_ctx ctx;    
    uint32_t mic;
    uint8_t b[sizeof(mic)];

    LDL_AES_init(&aes_ctx, key);
    LDL_CMAC_init(&ctx, &aes_ctx);
    LDL_CMAC_update(&ctx, msg, len);
    LDL_CMAC_finish(&ctx, b, sizeof(mic));
    
    (void)getU32(b, sizeof(mic), &mic);
    
    return mic;
}

static void xor128(uint8_t *acc, const uint8_t *op)
{
    acc[0] ^= op[0];
    acc[1] ^= op[1];
    acc[2] ^= op[2];
    acc[3] ^= op[3];
    acc[4] ^= op[4];
    acc[5] ^= op[5];
    acc[6] ^= op[6];
    acc[7] ^= op[7];
    acc[8] ^= op[8];
    acc[9] ^= op[9];
    acc[10] ^= op[10];
    acc[11] ^= op[11];
    acc[12] ^= op[12];
    acc[13] ^= op[13];
    acc[14] ^= op[14];
    acc[15] ^= op[15];
}

static size_t putU8(uint8_t *out, size_t max, uint8_t value)
{
    size_t retval = 0U;
    
    if(max >= sizeof(value)){
        
        out[0] = value;        
        retval = sizeof(value);
    }
    
    return retval;
}


static size_t putU16(uint8_t *out, size_t max, uint16_t value)
{
    size_t retval = 0U;
    
    if(max >= sizeof(value)){
        
        out[0] = (uint8_t)value;        
        out[1] = (uint8_t)(value >> 8);        
        retval = sizeof(value);
    }
    
    return retval;
}

#ifndef LORA_DISABLE_FULL_CODEC
static size_t putU24(uint8_t *out, size_t max, uint32_t value)
{
    size_t retval = 0U;
    
    if(max >= 3U){
        
        out[0] = (uint8_t)value;        
        out[1] = (uint8_t)(value >> 8);        
        out[2] = (uint8_t)(value >> 16);        
        retval = 3U;
    }
    
    return retval;
}
#endif

static size_t putU32(uint8_t *out, size_t max, uint32_t value)
{
    size_t retval = 0U;
    
    if(max >= sizeof(value)){
        
        out[0] = (uint8_t)value;        
        out[1] = (uint8_t)(value >> 8);        
        out[2] = (uint8_t)(value >> 16);        
        out[3] = (uint8_t)(value >> 24);        
        retval = sizeof(value);
    }
    
    return retval;
}


static size_t putEUI(uint8_t *out, size_t max, const uint8_t *value)
{
    size_t retval = 0U;
    
    if(max >= 8U){
        
        out[0] = value[7];
        out[1] = value[6];
        out[2] = value[5];
        out[3] = value[4];
        out[4] = value[3];
        out[5] = value[2];
        out[6] = value[1];
        out[7] = value[0];
        
        retval = 8U;
    }
    
    return retval;
}


static size_t getU8(const uint8_t *in, size_t max, uint8_t *value)
{
    size_t retval = 0U;
    
    if(max >= sizeof(*value)){
        
        *value = in[0];
        retval = sizeof(*value);
    }
    
    return retval;
}

static size_t getU16(const uint8_t *in, size_t max, uint16_t *value)
{
    size_t retval = 0U;

    if(max >= sizeof(*value)){
        
        *value = in[1];
        *value <<= 8;
        *value |= in[0];
        
        retval = sizeof(*value);
    }
    
    return retval;
}


static size_t getU24(const uint8_t *in, size_t max, uint32_t *value)
{
    size_t retval = 0U;
    
    if(max >= 3U){
        
        *value = in[2];
        *value <<= 8;
        *value |= in[1];
        *value <<= 8;
        *value |= in[0];
        
        retval = 3U;
    }
    
    return retval;
}

static size_t getU32(const uint8_t *in, size_t max, uint32_t *value)
{
    size_t retval = 0U;
    
    if(max >= sizeof(*value)){
        
        *value = in[3];
        *value <<= 8;
        *value |= in[2];
        *value <<= 8;
        *value |= in[1];
        *value <<= 8;
        *value |= in[0];
        
        retval = sizeof(*value);
    }
    
    return retval;
}

#ifndef LORA_DISABLE_FULL_CODEC
static size_t getEUI(const uint8_t *in, size_t max, uint8_t *value)
{
    size_t retval = 0U;
    
    if(max >= 8U){
        
        value[0] = in[7];
        value[1] = in[6];
        value[2] = in[5];
        value[3] = in[4];
        value[4] = in[3];
        value[5] = in[2];
        value[6] = in[1];
        value[7] = in[0];
        
        retval = 8U;
    }
    
    return retval;
}
#endif
