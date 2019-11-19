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

#include "ldl_aes.h"
#include "ldl_debug.h"
#include <string.h>

/* defines ************************************************************/

#define AES_BLOCK_SIZE 16U

#define R1 0U   
#define R2 1U
#define R3 2U
#define R4 3U

#define C1 0U
#define C2 4U
#define C3 8U
#define C4 12U

#define GALOIS_MUL2(B) ((((B) & 0x80U) == 0x80U) ? (uint8_t)(((B) << 1U) ^ 0x1bU) : (uint8_t)((B) << 1U))

#ifdef LDL_ENABLE_AVR

    #include <avr/pgmspace.h>
    
    #define RSBOX(C) pgm_read_byte(&rsbox[(C)])
    #define SBOX(C) pgm_read_byte(&sbox[(C)])
    #define RCON(C) pgm_read_byte(&rcon[(C)])

#else
    
    #define PROGMEM
    
    #define SBOX(C) (sbox[(C)])
    #define RCON(C) (rcon[(C)])
    #define RSBOX(C) (rsbox[(C)])
    
#endif

enum aes_key_size {

    AES_KEY_128 = 16U,
    AES_KEY_192 = 24U,
    AES_KEY_256 = 32U
};

/* static variables ***************************************************/

static const uint8_t sbox[] PROGMEM = {
    0x63U, 0x7cU, 0x77U, 0x7bU, 0xf2U, 0x6bU, 0x6fU, 0xc5U,
    0x30U, 0x01U, 0x67U, 0x2bU, 0xfeU, 0xd7U, 0xabU, 0x76U,
    0xcaU, 0x82U, 0xc9U, 0x7dU, 0xfaU, 0x59U, 0x47U, 0xf0U,
    0xadU, 0xd4U, 0xa2U, 0xafU, 0x9cU, 0xa4U, 0x72U, 0xc0U,
    0xb7U, 0xfdU, 0x93U, 0x26U, 0x36U, 0x3fU, 0xf7U, 0xccU,
    0x34U, 0xa5U, 0xe5U, 0xf1U, 0x71U, 0xd8U, 0x31U, 0x15U,
    0x04U, 0xc7U, 0x23U, 0xc3U, 0x18U, 0x96U, 0x05U, 0x9aU,
    0x07U, 0x12U, 0x80U, 0xe2U, 0xebU, 0x27U, 0xb2U, 0x75U,
    0x09U, 0x83U, 0x2cU, 0x1aU, 0x1bU, 0x6eU, 0x5aU, 0xa0U,
    0x52U, 0x3bU, 0xd6U, 0xb3U, 0x29U, 0xe3U, 0x2fU, 0x84U,
    0x53U, 0xd1U, 0x00U, 0xedU, 0x20U, 0xfcU, 0xb1U, 0x5bU,
    0x6aU, 0xcbU, 0xbeU, 0x39U, 0x4aU, 0x4cU, 0x58U, 0xcfU,
    0xd0U, 0xefU, 0xaaU, 0xfbU, 0x43U, 0x4dU, 0x33U, 0x85U,
    0x45U, 0xf9U, 0x02U, 0x7fU, 0x50U, 0x3cU, 0x9fU, 0xa8U,
    0x51U, 0xa3U, 0x40U, 0x8fU, 0x92U, 0x9dU, 0x38U, 0xf5U,
    0xbcU, 0xb6U, 0xdaU, 0x21U, 0x10U, 0xffU, 0xf3U, 0xd2U,
    0xcdU, 0x0cU, 0x13U, 0xecU, 0x5fU, 0x97U, 0x44U, 0x17U,
    0xc4U, 0xa7U, 0x7eU, 0x3dU, 0x64U, 0x5dU, 0x19U, 0x73U,
    0x60U, 0x81U, 0x4fU, 0xdcU, 0x22U, 0x2aU, 0x90U, 0x88U,
    0x46U, 0xeeU, 0xb8U, 0x14U, 0xdeU, 0x5eU, 0x0bU, 0xdbU,
    0xe0U, 0x32U, 0x3aU, 0x0aU, 0x49U, 0x06U, 0x24U, 0x5cU,
    0xc2U, 0xd3U, 0xacU, 0x62U, 0x91U, 0x95U, 0xe4U, 0x79U,
    0xe7U, 0xc8U, 0x37U, 0x6dU, 0x8dU, 0xd5U, 0x4eU, 0xa9U,
    0x6cU, 0x56U, 0xf4U, 0xeaU, 0x65U, 0x7aU, 0xaeU, 0x08U,
    0xbaU, 0x78U, 0x25U, 0x2eU, 0x1cU, 0xa6U, 0xb4U, 0xc6U,
    0xe8U, 0xddU, 0x74U, 0x1fU, 0x4bU, 0xbdU, 0x8bU, 0x8aU,
    0x70U, 0x3eU, 0xb5U, 0x66U, 0x48U, 0x03U, 0xf6U, 0x0eU,
    0x61U, 0x35U, 0x57U, 0xb9U, 0x86U, 0xc1U, 0x1dU, 0x9eU,
    0xe1U, 0xf8U, 0x98U, 0x11U, 0x69U, 0xd9U, 0x8eU, 0x94U,
    0x9bU, 0x1eU, 0x87U, 0xe9U, 0xceU, 0x55U, 0x28U, 0xdfU,
    0x8cU, 0xa1U, 0x89U, 0x0dU, 0xbfU, 0xe6U, 0x42U, 0x68U,
    0x41U, 0x99U, 0x2dU, 0x0fU, 0xb0U, 0x54U, 0xbbU, 0x16U
};

/* functions **********************************************************/

void LDL_AES_init(struct ldl_aes_ctx *ctx, const void *key)
{
    uint8_t p;
    uint8_t j;
    uint8_t b = 0U;
    uint8_t swap;
    uint8_t *k;
    uint8_t ks;
    uint8_t i = 1U;
    
    static const uint8_t rcon[] PROGMEM = {
        0x8dU, 0x01U, 0x02U, 0x04U, 0x08U, 0x10U, 0x20U, 0x40U, 0x80U, 0x1bU, 0x36U
    };

    LDL_PEDANTIC(ctx != NULL)
    LDL_PEDANTIC(key != NULL)

    ctx->r = 10U;
    b = 176U;

    (void)memcpy(ctx->k, key, 16U);
    k = ctx->k;    
    ks = 16U;
    p = ks;

    /* Rijndael key schedule */
    while(p < b){
        
        swap = k[p - 4U];
        k[p     ] = SBOX( k[p - 3U] ) ^ k[p      - ks] ^ RCON(i);
        k[p + 1U] = SBOX( k[p - 2U] ) ^ k[p + 1U - ks];
        k[p + 2U] = SBOX( k[p - 1U] ) ^ k[p + 2U - ks];
        k[p + 3U] = SBOX( swap      ) ^ k[p + 3U - ks];
        p += 4U;

        for(j=0U; j < 12U; j++){

            k[p] = k[p - 4U] ^ k[p - ks];
            p++;
        }

        i++;
    }
}

void LDL_AES_encrypt(const struct ldl_aes_ctx *ctx, void *s)
{
    uint8_t *_s = s;
    uint8_t r;    
    uint8_t a;
    uint8_t b;
    uint8_t c;
    uint8_t d;    
    uint8_t i;
    uint8_t p = 0U;
    const uint8_t *k = ctx->k;

    /* add round key, sbox and shiftrows */
    for(r = 0U; r < ctx->r; r++){

        /* add round key, sbox, left shift row */

        /* row 1 */
        _s[C1] = SBOX( _s[C1] ^ k[p] );
        _s[C2] = SBOX( _s[C2] ^ k[p + C2] );
        _s[C3] = SBOX( _s[C3] ^ k[p + C3] );
        _s[C4] = SBOX( _s[C4] ^ k[p + C4] );

        /* row 2, left shift 1 */
        a = SBOX( _s[R2] ^ k[p + R2] );
        _s[R2     ] = SBOX( _s[R2 + C2] ^ k[p + R2 + C2] );
        _s[R2 + C2] = SBOX( _s[R2 + C3] ^ k[p + R2 + C3] );
        _s[R2 + C3] = SBOX( _s[R2 + C4] ^ k[p + R2 + C4] );
        _s[R2 + C4] = a;

        /* row 3, left shift 2 */
        a = SBOX( _s[R3     ] ^ k[p + R3] );
        b = SBOX( _s[R3 + C2] ^ k[p + R3 + C2] );
        _s[R3     ] = SBOX( _s[R3 + C3] ^ k[p + R3 + C3] );
        _s[R3 + C2] = SBOX( _s[R3 + C4] ^ k[p + R3 + C4] );
        _s[R3 + C3] = a;
        _s[R3 + C4] = b;

        /* row 4, left shift 3 */
        a = SBOX( _s[R4 + C4] ^ k[p + R4 + C4] );
        _s[R4 + C4] = SBOX( _s[R4 + C3] ^ k[p + R4 + C3] );
        _s[R4 + C3] = SBOX( _s[R4 + C2] ^ k[p + R4 + C2] );
        _s[R4 + C2] = SBOX( _s[R4     ] ^ k[p + R4     ] );
        _s[R4     ] = a;

        if((r+1U) == ctx->r){

            p += 16U;

            /* final add round key */
            for(i=0U; i < 16U; i++){

                _s[i] ^= ctx->k[p+i];
            }
            
            break;
        }
         
        /* mix columns */
        for(i=0U; i < 16U; i += 4U){

            a = _s[i     ];
            b = _s[i + 1U];
            c = _s[i + 2U];
            d = _s[i + 3U];

            /* 2a + 3b + 1c + 1d 
             * 1a + 2b + 3c + 1d
             * 1a + 1b + 2c + 3d
             * 3a + 1b + 1c + 2d
             *
             * */
            _s[i     ] ^= (a ^ b ^ c ^ d) ^ GALOIS_MUL2( (a ^ b) );
            _s[i + 1U] ^= (a ^ b ^ c ^ d) ^ GALOIS_MUL2( (b ^ c) );
            _s[i + 2U] ^= (a ^ b ^ c ^ d) ^ GALOIS_MUL2( (c ^ d) );
            _s[i + 3U] ^= (a ^ b ^ c ^ d) ^ GALOIS_MUL2( (d ^ a) );
        }

        p += 16U;
    }   
}
