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

#ifndef LORA_AES_H
#define LORA_AES_H

/** @file */

/**
 * @addtogroup ldl_optional
 * 
 * @{
 * */

#ifdef __cplusplus
extern "C" {
#endif

#include "lora_platform.h"
#include <stdint.h>

#ifdef LORA_ENABLE_PLATFORM_AES

struct lora_aes_ctx;

#else

/** AES state */
struct lora_aes_ctx {

    uint8_t k[240U];
    uint8_t r;
};

#endif

#ifndef AES_BLOCK_SIZE
    #define AES_BLOCK_SIZE 16U
#endif

/**
 * 
 * This interface is called by LDL to initialise the AES block cipher
 * prior to calling LDL_AES_encrypt() and LDL_AES_decrypt().
 * 
 * @param[in] ctx
 * @param[in] key pointer to 16 byte key
 * 
 * */
void LDL_AES_init(struct lora_aes_ctx *ctx, const void *key);

/** Encrypt a block of data
 * 
 * @param[in] ctx state previously initialised by LDL_AES_init()
 * @param[in] s pointer to 16 byte block of data (any alignment)
 * 
 * */
void LDL_AES_encrypt(const struct lora_aes_ctx *ctx, void *s);

/** Decrypt a block of data
 * 
 * @param[in] ctx state previously initialised by LDL_AES_init()
 * @param[in] s pointer to 16 byte block of data (any alignment)
 * 
 * */
void LDL_AES_decrypt(const struct lora_aes_ctx *ctx, void *s);

#ifdef __cplusplus
}
#endif

/** @} */
#endif
