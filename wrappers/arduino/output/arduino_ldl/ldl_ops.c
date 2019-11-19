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

#include "ldl_ops.h"
#include "ldl_sm_internal.h"
#include "ldl_mac.h"
#include "ldl_system.h"
#include "ldl_frame.h"
#include "ldl_debug.h"
#include <string.h>

struct ldl_block {
    
    uint8_t value[16U];
};

/* static function prototypes *****************************************/

static void initA(struct ldl_block *a, uint32_t devAddr, bool up, uint32_t counter);
static void initB(struct ldl_block *b, uint16_t confirmCounter, uint8_t rate, uint8_t chIndex, bool up, uint32_t devAddr, uint32_t upCounter, uint8_t len);

static uint32_t deriveDownCounter(struct ldl_mac *self, uint8_t port, uint16_t counter);

static uint8_t putU8(uint8_t *buf, uint8_t value);
static uint8_t putU16(uint8_t *buf, uint16_t value);
static uint8_t putU24(uint8_t *buf, uint32_t value);
static uint8_t putU32(uint8_t *buf, uint32_t value);
static uint8_t putEUI(uint8_t *buf, const uint8_t *value);

/* functions **********************************************************/

void LDL_OPS_syncDownCounter(struct ldl_mac *self, uint8_t port, uint16_t counter)
{
    LDL_PEDANTIC(self != NULL)
    
    uint32_t derived;
    
    derived = deriveDownCounter(self, port, counter);
    
    if((self->ctx.version > 0U) && (port == 0U)){
        
        self->ctx.nwkDown = (uint16_t)(derived >> 16);
    }
    else{
        
        self->ctx.appDown = (uint16_t)(derived >> 16);
    }    
}
                  
void LDL_OPS_deriveKeys(struct ldl_mac *self)
{
    LDL_PEDANTIC(self != NULL)
    
    struct ldl_block iv;
    uint8_t *ptr;
    uint8_t pos;
    
    ptr = iv.value;
    
    (void)memset(&iv, 0, sizeof(iv));
    
    LDL_SM_beginUpdateSessionKey(self->sm); 
    {
        if(self->ctx.version == 0U){
            
            /* ptr[0] below */    
            pos = 1U;
            pos += putU24(&ptr[pos], self->joinNonce);
            pos += putU24(&ptr[pos], self->ctx.netID);
            (void)putU16(&ptr[pos], self->devNonce);
            
            ptr[0] = 2U;
            LDL_SM_updateSessionKey(self->sm, LDL_SM_KEY_APPS, LDL_SM_KEY_NWK, &iv);
            
            ptr[0] = 1U;
            LDL_SM_updateSessionKey(self->sm, LDL_SM_KEY_FNWKSINT, LDL_SM_KEY_NWK, &iv);    
            LDL_SM_updateSessionKey(self->sm, LDL_SM_KEY_SNWKSINT, LDL_SM_KEY_NWK, &iv);
            LDL_SM_updateSessionKey(self->sm, LDL_SM_KEY_NWKSENC, LDL_SM_KEY_NWK, &iv);
        }
        else{
            
            /* ptr[0] below */ 
            (void)putEUI(&ptr[1U], self->devEUI);
            
            ptr[0] = 5U;
            LDL_SM_updateSessionKey(self->sm, LDL_SM_KEY_JSENC, LDL_SM_KEY_NWK, &iv);                
            
            ptr[0] = 6U;
            LDL_SM_updateSessionKey(self->sm, LDL_SM_KEY_JSINT, LDL_SM_KEY_NWK, &iv); 
            
            /* ptr[0] below */ 
            pos = 1U;
            pos += putU24(&ptr[pos], self->joinNonce);
            pos += putEUI(&ptr[pos], self->joinEUI);
            (void)putU16(&ptr[pos], self->devNonce);
               
            ptr[0] = 1U;
            LDL_SM_updateSessionKey(self->sm, LDL_SM_KEY_FNWKSINT, LDL_SM_KEY_NWK, &iv);    
            
            ptr[0] = 2U;
            LDL_SM_updateSessionKey(self->sm, LDL_SM_KEY_APPS, LDL_SM_KEY_NWK, &iv);
            
            ptr[0] = 3U;
            LDL_SM_updateSessionKey(self->sm, LDL_SM_KEY_SNWKSINT, LDL_SM_KEY_NWK, &iv);
            
            ptr[0] = 4U;
            LDL_SM_updateSessionKey(self->sm, LDL_SM_KEY_NWKSENC, LDL_SM_KEY_NWK, &iv);                               
        }        
    }    
    LDL_SM_endUpdateSessionKey(self->sm);    
}

uint8_t LDL_OPS_prepareData(struct ldl_mac *self, const struct ldl_frame_data *f, uint8_t *out, uint8_t max)
{
    LDL_PEDANTIC(self != NULL)
    
    struct ldl_frame_data_offset off;
    uint8_t retval = 0U;
    
    retval = LDL_Frame_putData(f, out, max, &off);

    if(retval > 0U){

        /* encrypt */
        {
            struct ldl_block A;
            
            initA(&A, f->devAddr, true, f->counter);

            /* encrypt fopt (LoRaWAN 1.1) */
            if(self->ctx.version == 1U){
                
                LDL_SM_ctr(self->sm, LDL_SM_KEY_NWKSENC, &A, &out[off.opts], f->optsLen);            
            }
            
            /* encrypt data */
            LDL_SM_ctr(self->sm, (f->port == 0U) ? LDL_SM_KEY_NWKSENC : LDL_SM_KEY_APPS, &A, &out[off.data], f->dataLen);                
        }

        /* produce MIC*/
        {
            struct ldl_block B0;
            struct ldl_block B1;            
            uint32_t micS;
            uint32_t micF;
            
            initB(&B0, 0U, 0U, 0U, true, f->devAddr, f->counter, retval - sizeof(micF)); 
            initB(&B1, 0U, self->tx.rate, self->tx.chIndex, true, f->devAddr, f->counter, retval - sizeof(micS));

            micF = LDL_SM_mic(self->sm, LDL_SM_KEY_FNWKSINT, &B0, sizeof(B0.value), out, retval - sizeof(micF));       

            if(self->ctx.version == 1U){
            
                micS = LDL_SM_mic(self->sm, LDL_SM_KEY_SNWKSINT, &B1, sizeof(B1.value), out, retval - sizeof(micS));
                
                LDL_Frame_updateMIC(out, retval, ((micF << 16) | (micS & 0xffffUL))); 
            } 
            else{
                
                LDL_Frame_updateMIC(out, retval, micF); 
            }
        }
    }
     
    return retval;
}

uint8_t LDL_OPS_prepareJoinRequest(struct ldl_mac *self, const struct ldl_frame_join_request *f, uint8_t *out, uint8_t max)
{
    uint32_t mic;
    uint8_t retval;
    
    retval = LDL_Frame_putJoinRequest(f, out, max);
    
    mic = LDL_SM_mic(self->sm, LDL_SM_KEY_NWK, NULL, 0U, out, retval-sizeof(mic));
    
    LDL_Frame_updateMIC(out, retval, mic);
    
    return retval;
}

bool LDL_OPS_receiveFrame(struct ldl_mac *self, struct ldl_frame_down *f, uint8_t *in, uint8_t len)
{
    bool retval;
    uint32_t mic;
        
    retval = false;
    
    if(LDL_Frame_decode(f, in, len)){
        
        switch(f->type){
        default:
            break;
        
        case FRAME_TYPE_JOIN_ACCEPT:
        
            if((self->op == LDL_OP_JOINING) || (self->op == LDL_OP_REJOINING)){
                
                enum ldl_sm_key key;
                
                key = (self->op == LDL_OP_JOINING) ? LDL_SM_KEY_APP : LDL_SM_KEY_JSENC;
                
                LDL_SM_ecb(self->sm, key, &in[1U]);
        
                if(len == LDL_Frame_sizeofJoinAccept(true)){
                    
                    LDL_SM_ecb(self->sm, key, &in[LDL_Frame_sizeofJoinAccept(false)]);
                }
                
                if(LDL_Frame_decode(f, in, len)){
                    
                    if(f->optNeg){
                        
                        if(f->joinNonce > self->joinNonce){
                        
                            struct ldl_block hdr;
                            uint8_t pos;
                            
                            pos = 0U;
                            
                            switch(self->op){
                            default:
                            case LDL_OP_JOINING:
                                pos += putU8(&hdr.value[pos], 0xffU);
                                break;
                            case LDL_OP_REJOINING:                        
                                pos += putU8(&hdr.value[pos], 2U);
                                break;
                            }
                            
                            pos += putEUI(&hdr.value[pos], self->joinEUI);
                            pos += putU16(&hdr.value[pos], self->devNonce);
                            
                            mic = LDL_SM_mic(self->sm, LDL_SM_KEY_JSINT, &hdr, pos, in, len-sizeof(mic));
                            
                            if(f->mic == mic){
                            
                                retval = true;                    
                            }
                            else{
                                
                                LDL_DEBUG(self->app, "joinAccept MIC failed")
                            }                           
                        }
                        else{
                            
                            LDL_DEBUG(self->app, "invalid joinNonce")
                        }
                    }
                    else{
                        
                        mic = LDL_SM_mic(self->sm, LDL_SM_KEY_NWK, NULL, 0U, in, len-sizeof(mic));                        
                        
                        if(f->mic == mic){
                        
                            retval = true;                    
                        }
                        else{
                            
                            LDL_DEBUG(self->app, "joinAccept MIC failed")
                        }   
                    }                     
                }                                
            }
            else{
                
                LDL_DEBUG(self->app, "unexpected frame type")
            }
            break;
        
        case FRAME_TYPE_DATA_UNCONFIRMED_DOWN:
        case FRAME_TYPE_DATA_CONFIRMED_DOWN:
        
            if(
                ((self->ctx.version > 0) && (self->op == LDL_OP_REJOINING))
                ||
                (self->op  == LDL_OP_DATA_UNCONFIRMED)
                ||
                (self->op == LDL_OP_DATA_CONFIRMED)
            ){
            
                if(self->ctx.devAddr == f->devAddr){
                    
                    uint32_t counter;
                    
                    counter = deriveDownCounter(self, f->port, f->counter);

                    struct ldl_block B;
                    struct ldl_block A;

                    if((self->ctx.version == 1U) && f->ack){
                    
                        initB(&B, (self->ctx.up-1U), 0U, 0U, false, f->devAddr, counter, len-sizeof(mic));
                    }                    
                    else{
                        
                        initB(&B, 0U, 0U, 0U, false, f->devAddr, counter, len-sizeof(mic));
                    }
                    
                    mic = LDL_SM_mic(self->sm, LDL_SM_KEY_SNWKSINT, &B, sizeof(B.value), in, len-sizeof(mic));
                    
                    if(mic == f->mic){
                        
                        initA(&A, f->devAddr, false, f->counter);

                        /* V1.1 encrypts the opts */
                        if(self->ctx.version == 1U){
                            
                            LDL_SM_ctr(self->sm, LDL_SM_KEY_NWKSENC, &A, f->opts, f->optsLen);    
                        }
                        
                        LDL_SM_ctr(self->sm, (f->port == 0U) ? LDL_SM_KEY_NWKSENC : LDL_SM_KEY_APPS, &A, f->data, f->dataLen);
                       
                        retval = true;
                    }
                    else{
                                            
                        LDL_DEBUG(self->app, "mic failed")
                    }                           
                }
                else{
                    
                    LDL_DEBUG(self->app, "devaddr mismatch")        
                }                                                         
            }
            else{
                
                LDL_DEBUG(self->app, "unexpected frame type")
            }
        
            break;
        }    
    }
    else{
        
        LDL_DEBUG(self->app, "invalid frame")
    }
    
    return retval;
}

/* static functions ***************************************************/

static void initA(struct ldl_block *a, uint32_t devAddr, bool up, uint32_t counter)
{
    uint8_t pos = 0U;
    uint8_t *ptr = a->value;
    
    pos += putU8(&ptr[pos], 1U);
    pos += putU32(&ptr[pos], 0U);
    pos += putU8(&ptr[pos], up ? 0U : 1U);
    pos += putU32(&ptr[pos], devAddr);
    pos += putU32(&ptr[pos], counter);    
    (void)putU16(&ptr[pos], 0U);    
}

static void initB(struct ldl_block *b, uint16_t confirmCounter, uint8_t rate, uint8_t chIndex, bool up, uint32_t devAddr, uint32_t upCounter, uint8_t len)
{    
    uint8_t pos = 0U;
    uint8_t *ptr = b->value;
    
    pos += putU8(&ptr[pos], 0x49U);
    pos += putU16(&ptr[pos], confirmCounter);
    pos += putU8(&ptr[pos], rate);
    pos += putU8(&ptr[pos], chIndex);
    pos += putU8(&ptr[pos], up ? 0U : 1U);
    pos += putU32(&ptr[pos], devAddr);
    pos += putU32(&ptr[pos], upCounter);
    pos += putU8(&ptr[pos], 0U);
    (void)putU8(&ptr[pos], len);
}

static uint32_t deriveDownCounter(struct ldl_mac *self, uint8_t port, uint16_t counter)
{
    uint32_t mine = ((self->ctx.version > 0U) && (port == 0U)) ? (uint32_t)self->ctx.nwkDown : (uint32_t)self->ctx.appDown;
    
    mine = mine << 16;
    
    if((uint32_t)counter < mine){
        
        mine = mine + 0x10000UL + (uint32_t)counter;
    }
    else{
        
        mine = mine + (uint32_t)counter;
    }    
    
    return mine;
}

static uint8_t putEUI(uint8_t *buf, const uint8_t *value)
{
    buf[0] = value[7];
    buf[1] = value[6];
    buf[2] = value[5];
    buf[3] = value[4];
    buf[4] = value[3];
    buf[5] = value[2];
    buf[6] = value[1];
    buf[7] = value[0];
    
    return 8U;
}

static uint8_t putU8(uint8_t *buf, uint8_t value)
{
    buf[0] = value;
    
    return 1U;
}

static uint8_t putU16(uint8_t *buf, uint16_t value)
{
    buf[0] = value;
    buf[1] = value >> 8;
    
    return 2U;
}

static uint8_t putU24(uint8_t *buf, uint32_t value)
{
    buf[0] = value;
    buf[1] = value >> 8;
    buf[2] = value >> 16;
    
    return 3U;    
}

static uint8_t putU32(uint8_t *buf, uint32_t value)
{
    buf[0] = value;
    buf[1] = value >> 8;
    buf[2] = value >> 16;
    buf[3] = value >> 24;
    
    return 4U;        
}
