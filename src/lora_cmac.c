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

/* includes ***********************************************************/

#ifndef LORA_ENABLE_PLATFORM_CMAC

#include "lora_aes.h"
#include "lora_cmac.h"
#include "lora_debug.h"

#include <string.h>
#include <stdio.h>

/* defines ************************************************************/

#define BLOCK_SIZE  16U
#define LSB         0x01U
#define MSB         0x80U

/* static function prototypes *****************************************/

/**
 * XOR an aligned AES block (may be aliased)
 *
 * @param[out] acc accumulator
 * @param[in] mask XORed with accumulator
 *
 * */
static void xor128(uint8_t *acc, const uint8_t *mask);

/**
 * left shift (by one bit) a 128bit vector
 *
 * @param[in/out] v vector to shift
 *
 * */
static void leftShift128(uint8_t *v);

/* functions  *********************************************************/

void LDL_CMAC_init(struct lora_cmac_ctx *ctx, const struct lora_aes_ctx *aes_ctx)
{    
    LORA_PEDANTIC(ctx != NULL)
    LORA_PEDANTIC(aes_ctx != NULL)

    (void)memset(ctx, 0, sizeof(*ctx));
    ctx->aes_ctx = aes_ctx;
}

void LDL_CMAC_update(struct lora_cmac_ctx *ctx, const void *data, uint8_t len)
{
    LORA_PEDANTIC(ctx != NULL)
    LORA_PEDANTIC((len == 0U) || (data != NULL))

    size_t part;
    size_t i;
    size_t blocks;
    size_t pos = 0U;
    const uint8_t *in = (const uint8_t *)data;    
    
    part =  ctx->size % sizeof(ctx->m);
    
    if(len > 0U){

        if(((part == 0U) && (ctx->size > 0U)) || ((part + len) > sizeof(ctx->m))){

            /* sometimes a whole extra block will already be cached, process it */
            if((part == 0U) && (ctx->size > 0U)){
                
                xor128(ctx->x, ctx->m);
                LDL_AES_encrypt(ctx->aes_ctx, ctx->x);
            }

            /* number of new blocks to process */
            blocks = (part + len) / sizeof(ctx->m);
                        
            /* do not process the last new block */
            if(((part + len) % sizeof(ctx->m)) == 0U){
            
                blocks -= 1U;
            }
            
            /* make up the first block to process */
            
            for(i=0U; i < blocks; i++){

                (void)memcpy(&ctx->m[part], &in[pos], sizeof(ctx->m) - part);
                pos += sizeof(ctx->m) - part;

                part = 0U;

                xor128(ctx->x, ctx->m);
                LDL_AES_encrypt(ctx->aes_ctx, ctx->x);                
            }
        }
                    
        (void)memcpy(&ctx->m[part], &in[pos], len - pos);
        ctx->size += len;        
    }
}

void LDL_CMAC_finish(const struct lora_cmac_ctx *ctx, void *out, uint8_t outMax)
{
    LORA_PEDANTIC(ctx != NULL)

    uint8_t k[BLOCK_SIZE];
    
    uint8_t k1[BLOCK_SIZE];
    uint8_t k2[BLOCK_SIZE];
    
    uint8_t m_last[BLOCK_SIZE];

    size_t part;

    /* generate subkeys */
    
    (void)memset(k, 0, sizeof(k));
    LDL_AES_encrypt(ctx->aes_ctx, k);
    
    (void)memcpy(k1, k, sizeof(k1));
    leftShift128(k1);

    if((k[0] & 0x80U) == 0x80U){

        k1[15] ^= 0x87U;
    }

    (void)memcpy(k2, k1, sizeof(k2));
    leftShift128(k2);

    if((k1[0] & 0x80U) == 0x80U){

        k2[15] ^= 0x87U;
    }

    /* process last block (m_last) */
    
    part = ctx->size % sizeof(ctx->m);

    (void)memset(m_last, 0, sizeof(m_last));
    
    if((ctx->size == 0U) || (part > 0U)){

        (void)memcpy(m_last, ctx->m, part);

        m_last[part] = 0x80U;
        xor128(m_last, k2);        
    }
    else{        

        (void)memcpy(m_last, ctx->m, sizeof(m_last));

        xor128(m_last, k1);
    }

    xor128(m_last, ctx->x);

    LDL_AES_encrypt(ctx->aes_ctx, m_last);

    (void)memcpy(out, m_last, (outMax > sizeof(m_last)) ? sizeof(m_last) : outMax);
}

/* static functions  **************************************************/

static void leftShift128(uint8_t *v)
{
    uint8_t t;
    uint8_t tt;
    uint8_t carry;
    uint8_t i;
    
    carry = 0U;

    for(i=16U; i > 0U; i--){

        t = v[i-1U];

        tt = t;
        tt <<= 1;
        tt |= carry;

        carry = ((t & MSB) == MSB) ? LSB : 0x0U;
        v[i-1U] = tt;
    }
}

static void xor128(uint8_t *acc, const uint8_t *mask)
{
    acc[0] ^= mask[0];
    acc[1] ^= mask[1];
    acc[2] ^= mask[2];
    acc[3] ^= mask[3];
    acc[4] ^= mask[4];
    acc[5] ^= mask[5];
    acc[6] ^= mask[6];
    acc[7] ^= mask[7];
    acc[8] ^= mask[8];
    acc[9] ^= mask[9];
    acc[10] ^= mask[10];
    acc[11] ^= mask[11];
    acc[12] ^= mask[12];
    acc[13] ^= mask[13];
    acc[14] ^= mask[14];
    acc[15] ^= mask[15];        
}

#endif
