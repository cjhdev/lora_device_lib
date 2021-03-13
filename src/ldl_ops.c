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

#include "ldl_ops.h"
#include "ldl_sm_internal.h"
#include "ldl_mac.h"
#include "ldl_system.h"
#include "ldl_frame.h"
#include "ldl_debug.h"
#include "ldl_internal.h"
#include "ldl_stream.h"
#include <string.h>

struct ldl_block {

    uint8_t value[16];
};

/* static function prototypes *****************************************/

static void initA(struct ldl_block *a, uint32_t c, uint32_t devAddr, bool up, uint32_t counter, uint8_t i);
static void initB(struct ldl_block *b, uint16_t confirmCounter, uint8_t rate, uint8_t chIndex, bool up, uint32_t devAddr, uint32_t upCounter, uint8_t len);

static uint32_t deriveDownCounter(struct ldl_mac *self, uint8_t port, uint16_t counter);

/* functions **********************************************************/

void LDL_OPS_syncDownCounter(struct ldl_mac *self, uint8_t port, uint16_t counter)
{
    LDL_PEDANTIC(self != NULL)

    uint32_t derived;

    derived = deriveDownCounter(self, port, counter);

    if((SESS_VERSION(self->ctx) > 0U) && (port == 0U)){

        self->ctx.nwkDown = U16(derived >> 16);
    }
    else{

        self->ctx.appDown = U16(derived >> 16);
    }
}

void LDL_OPS_deriveKeys(struct ldl_mac *self)
{
    LDL_PEDANTIC(self != NULL)

    struct ldl_block iv = {0};
    struct ldl_stream s;

    LDL_Stream_init(&s, &iv, sizeof(iv));

    if(SESS_VERSION(self->ctx) == 0U){

        (void)LDL_Stream_putU8(&s, 1);
        (void)LDL_Stream_putU24(&s, self->ctx.joinNonce);
        (void)LDL_Stream_putU24(&s, self->ctx.netID);
        (void)LDL_Stream_putU16(&s, self->ctx.devNonce);

        self->sm_interface->update_session_key(self->sm, LDL_SM_KEY_FNWKSINT, LDL_SM_KEY_NWK, &iv);
        self->sm_interface->update_session_key(self->sm, LDL_SM_KEY_SNWKSINT, LDL_SM_KEY_NWK, &iv);
        self->sm_interface->update_session_key(self->sm, LDL_SM_KEY_NWKSENC, LDL_SM_KEY_NWK, &iv);

        LDL_Stream_rewind(&s);
        (void)LDL_Stream_putU8(&s, 2);
        self->sm_interface->update_session_key(self->sm, LDL_SM_KEY_APPS, LDL_SM_KEY_NWK, &iv);

    }
    else{

        (void)LDL_Stream_putU8(&s, 1);
        (void)LDL_Stream_putU24(&s, self->ctx.joinNonce);
        (void)LDL_Stream_putEUI(&s, self->joinEUI);
        (void)LDL_Stream_putU16(&s, self->ctx.devNonce);

        self->sm_interface->update_session_key(self->sm, LDL_SM_KEY_FNWKSINT, LDL_SM_KEY_NWK, &iv);

        LDL_Stream_rewind(&s);
        (void)LDL_Stream_putU8(&s, 2);
        self->sm_interface->update_session_key(self->sm, LDL_SM_KEY_APPS, LDL_SM_KEY_APP, &iv);

        LDL_Stream_rewind(&s);
        (void)LDL_Stream_putU8(&s, 3);
        self->sm_interface->update_session_key(self->sm, LDL_SM_KEY_SNWKSINT, LDL_SM_KEY_NWK, &iv);

        LDL_Stream_rewind(&s);
        (void)LDL_Stream_putU8(&s, 4);
        self->sm_interface->update_session_key(self->sm, LDL_SM_KEY_NWKSENC, LDL_SM_KEY_NWK, &iv);
    }
}

#if defined(LDL_ENABLE_L2_1_1)
void LDL_OPS_deriveJoinKeys(struct ldl_mac *self)
{
    LDL_PEDANTIC(self != NULL)

    struct ldl_block iv = {0};
    struct ldl_stream s;

    LDL_Stream_init(&s, &iv, sizeof(iv));

    (void)LDL_Stream_putU8(&s, 5);
    (void)LDL_Stream_putEUI(&s, self->devEUI);
    self->sm_interface->update_session_key(self->sm, LDL_SM_KEY_JSENC, LDL_SM_KEY_NWK, &iv);

    LDL_Stream_rewind(&s);
    (void)LDL_Stream_putU8(&s, 6);
    self->sm_interface->update_session_key(self->sm, LDL_SM_KEY_JSINT, LDL_SM_KEY_NWK, &iv);
}
#endif

uint8_t LDL_OPS_prepareData(struct ldl_mac *self, const struct ldl_frame_data *f, uint8_t *out, uint8_t max)
{
    LDL_PEDANTIC(self != NULL)

    struct ldl_frame_data_offset off;
    uint8_t retval = 0;

    retval = LDL_Frame_putData(f, out, max, &off);

    if(retval > 0U){

        struct ldl_block A;

#if defined(LDL_ENABLE_L2_1_1)
        /* encrypt fopt (LoRaWAN 1.1) */
        if(SESS_VERSION(self->ctx) == 1U){

#ifdef LDL_ENABLE_ERRATA_A1
            /* as per errata 26 Jan 2018 */
            initA(&A, 1, f->devAddr, true, f->counter, 1);
#else
            /* as per 1.1 spec */
            initA(&A, 0, f->devAddr, true, f->counter, 0);
#endif
            self->sm_interface->ctr(self->sm, LDL_SM_KEY_NWKSENC, &A, &out[off.opts], f->optsLen);
        }
#endif

        initA(&A, 0, f->devAddr, true, f->counter, 1);

        /* encrypt data */
        self->sm_interface->ctr(self->sm, (f->port == 0U) ? LDL_SM_KEY_NWKSENC : LDL_SM_KEY_APPS, &A, &out[off.data], f->dataLen);

    }

    return retval;
}

void LDL_OPS_micDataFrame(struct ldl_mac *self, void *buffer, uint8_t size)
{
    struct ldl_block B0;
    struct ldl_block B1;
    uint32_t micS;
    uint32_t micF;

    initB(&B0, 0U, 0U, 0U, true, self->ctx.devAddr, self->tx.counter, size - U8(sizeof(micF)));
    initB(&B1, 0U, self->tx.rate, self->tx.chIndex, true, self->ctx.devAddr, self->tx.counter, size - U8(sizeof(micS)));

    micF = self->sm_interface->mic(self->sm, LDL_SM_KEY_FNWKSINT, &B0, U8(sizeof(B0.value)), buffer, size - U8(sizeof(micF)));

    if(SESS_VERSION(self->ctx) == 1U){

        micS = self->sm_interface->mic(self->sm, LDL_SM_KEY_SNWKSINT, &B1, U8(sizeof(B1.value)), buffer, size - U8(sizeof(micS)));

        LDL_Frame_updateMIC(buffer, size, ((micF << 16) | (micS & U32(0xffff))));
    }
    else{

        LDL_Frame_updateMIC(buffer, size, micF);
    }
}

uint8_t LDL_OPS_prepareJoinRequest(struct ldl_mac *self, const struct ldl_frame_join_request *f, uint8_t *out, uint8_t max)
{
    uint32_t mic;
    uint8_t retval;

    retval = LDL_Frame_putJoinRequest(f, out, max);

    mic = self->sm_interface->mic(self->sm, LDL_SM_KEY_NWK, NULL, 0U, out, retval - U8(sizeof(mic)));

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
            /* impossible */
            break;

        case FRAME_TYPE_JOIN_ACCEPT:

            if((self->op == LDL_OP_JOINING) || (self->op == LDL_OP_REJOINING)){

                enum ldl_sm_key key;

                key = (self->op == LDL_OP_JOINING) ? LDL_SM_KEY_NWK : LDL_SM_KEY_JSENC;

                self->sm_interface->ecb(self->sm, key, &in[1U]);

                if(len == LDL_Frame_sizeofJoinAccept(true)){

                    self->sm_interface->ecb(self->sm, key, &in[LDL_Frame_sizeofJoinAccept(false)]);
                }

                if(LDL_Frame_decode(f, in, len)){

#if defined(LDL_ENABLE_L2_1_1) || defined(LDL_ENABLE_L2_1_0_4)
                    if(f->joinNonce < self->joinNonce){

                        /* invalid joinNonce */
                        LDL_DEBUG("invalid joinNonce")
                    }
                    else
#endif
                    {
#if defined(LDL_ENABLE_L2_1_1)
                        if(f->optNeg){

                            struct ldl_block hdr = {0};
                            struct ldl_stream s;

                            LDL_Stream_init(&s, &hdr, sizeof(hdr));

                            switch(self->op){
                            default:
                            case LDL_OP_JOINING:
                                (void)LDL_Stream_putU8(&s, 0xff);
                                break;
                            case LDL_OP_REJOINING:
                                (void)LDL_Stream_putU8(&s, 2);
                                break;
                            }

                            (void)LDL_Stream_putEUI(&s, self->joinEUI);
                            (void)LDL_Stream_putU16(&s, self->ctx.devNonce);

                            mic = self->sm_interface->mic(self->sm, LDL_SM_KEY_JSINT, &hdr, LDL_Stream_tell(&s), in, len - U8(sizeof(mic)));

                            if(f->mic == mic){

                                retval = true;
                            }
                            else{

                                /* MIC failed */
                                LDL_DEBUG("joinAccept MIC failed")
                            }
                        }
                        else
#endif
                        {
                            mic = self->sm_interface->mic(self->sm, LDL_SM_KEY_NWK, NULL, 0U, in, len - U8(sizeof(mic)));

                            if(f->mic == mic){

                                retval = true;
                            }
                            else{

                                /* MIC failed */
                                LDL_DEBUG("joinAccept MIC failed")
                            }
                        }
                    }
                }
                else{

                    /* shouldn't happen */
                    LDL_DEBUG("LDL_Frame_decode() (JoinAccept after decrypt)")
                }
            }
            else{

                /* unexpected frame type */
                LDL_DEBUG("unexpected frame type")
            }
            break;

        case FRAME_TYPE_DATA_UNCONFIRMED_DOWN:
        case FRAME_TYPE_DATA_CONFIRMED_DOWN:

            if(
                (self->op == LDL_OP_REJOINING)
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

                    if((SESS_VERSION(self->ctx) == 1U) && f->ack){

                        initB(&B, U16(self->ctx.up-1U), 0U, 0U, false, f->devAddr, counter, len - U8(sizeof(mic)));
                    }
                    else{

                        initB(&B, 0U, 0U, 0U, false, f->devAddr, counter, len - U8(sizeof(mic)));
                    }

                    mic = self->sm_interface->mic(self->sm, LDL_SM_KEY_SNWKSINT, &B, U8(sizeof(B.value)), in, len - U8(sizeof(mic)));

                    if(mic == f->mic){

#if defined(LDL_ENABLE_L2_1_1)
                        /* V1.1 encrypts the opts */
                        if(SESS_VERSION(self->ctx) == 1U){
#ifdef LDL_ENABLE_ERRATA_A1
                            /* as per errata 26 Jan 2018 */
                            initA(&A, f->dataPresent ? 2U : 1U, f->devAddr, false, f->counter, 0U);
#else
                            /* as per 1.1 spec */
                            initA(&A, 0U, f->devAddr, false, f->counter, 0U);
#endif
                            self->sm_interface->ctr(self->sm, LDL_SM_KEY_NWKSENC, &A, f->opts, f->optsLen);
                        }
#endif
                        initA(&A, 0U, f->devAddr, false, f->counter, 1U);

                        self->sm_interface->ctr(self->sm, (f->port == 0U) ? LDL_SM_KEY_NWKSENC : LDL_SM_KEY_APPS, &A, f->data, f->dataLen);

                        retval = true;
                    }
                    else{

                        /* MIC failed */
                        LDL_DEBUG("data MIC failed")
                    }
                }
                else{

                    /* devaddr */
                    LDL_DEBUG("devaddr mismatch")
                }
            }
            else{

                /* unexpected frame type */
                LDL_DEBUG("unexpected frame type")
            }

            break;
        }
    }
    else{

        /* invalid frame */
        LDL_DEBUG("invalid frame")
    }

    return retval;
}

/* static functions ***************************************************/

static void initA(struct ldl_block *a, uint32_t c, uint32_t devAddr, bool up, uint32_t counter, uint8_t i)
{
    struct ldl_stream s;

    LDL_Stream_init(&s, a, sizeof(*a));

    (void)LDL_Stream_putU8(&s, 1);
    (void)LDL_Stream_putU32(&s, c);
    (void)LDL_Stream_putU8(&s, up ? 0U : 1U);
    (void)LDL_Stream_putU32(&s, devAddr);
    (void)LDL_Stream_putU32(&s, counter);
    (void)LDL_Stream_putU8(&s, 0);
    (void)LDL_Stream_putU8(&s, i);
}

static void initB(struct ldl_block *b, uint16_t confirmCounter, uint8_t rate, uint8_t chIndex, bool up, uint32_t devAddr, uint32_t upCounter, uint8_t len)
{
    struct ldl_stream s;

    LDL_Stream_init(&s, b, sizeof(*b));

    (void)LDL_Stream_putU8(&s, 0x49);
    (void)LDL_Stream_putU16(&s, confirmCounter);
    (void)LDL_Stream_putU8(&s, rate);
    (void)LDL_Stream_putU8(&s, chIndex);
    (void)LDL_Stream_putU8(&s, up ? 0U : 1U);
    (void)LDL_Stream_putU32(&s, devAddr);
    (void)LDL_Stream_putU32(&s, upCounter);
    (void)LDL_Stream_putU8(&s, 0);
    (void)LDL_Stream_putU8(&s, len);
}

static uint32_t deriveDownCounter(struct ldl_mac *self, uint8_t port, uint16_t counter)
{
    uint32_t mine = ((SESS_VERSION(self->ctx) > 0U) && (port == 0U)) ? U32(self->ctx.nwkDown) : U32(self->ctx.appDown);

    mine = mine << 16;

    if(U32(counter) < mine){

        mine = mine + U32(0x10000) + U32(counter);
    }
    else{

        mine = mine + U32(counter);
    }

    return mine;
}
