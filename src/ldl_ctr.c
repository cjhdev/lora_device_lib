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

#include "ldl_ctr.h"
#include "ldl_debug.h"

#include <string.h>

/* static function prototypes *****************************************/

static void xor128(uint8_t *acc, const uint8_t *op);

/* functions **********************************************************/

void LDL_CTR_encrypt(struct ldl_aes_ctx *ctx, const void *iv, const void *in, void *out, uint8_t len)
{
    LDL_PEDANTIC(ctx != NULL)
    
    uint8_t a[16U];
    uint8_t s[16U];
    uint8_t pld[16U];
    uint8_t k;
    uint8_t i;
    uint8_t pos;
    uint8_t size;
    const uint8_t *ptr_in;
    uint8_t *ptr_out;

    /* number of blocks */
    k = (len / 16U) + (((len % 16U) != 0U) ? 1U : 0U);
    
    pos = 0U;

    ptr_in = (const uint8_t *)in;
    ptr_out = (uint8_t *)out;

    (void)memcpy(a, iv, sizeof(a)); 

    for(i=0U; i < k; i++){

        size = ((len - pos) >= (uint8_t)sizeof(a)) ? (uint8_t)sizeof(a) : (len - pos);
        
        (void)memset(pld, 0, sizeof(pld));
        
        (void)memcpy(pld, &ptr_in[pos], size);
        
        a[15U] = i + 1U;

        (void)memcpy(s, a, sizeof(s));
        
        LDL_AES_encrypt(ctx, s);

        xor128(pld, s);

        (void)memcpy(&ptr_out[pos], pld, size);
        
        pos += (uint8_t)sizeof(a);
    }
}

/* static functions ***************************************************/

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
