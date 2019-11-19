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

#ifndef LDL_AES_H
#define LDL_AES_H

/** @file */

/**
 * @defgroup ldl_crypto Default Crypto
 * @ingroup ldl
 * 
 * # Default Crypto Interface
 * 
 * The implementations for these interfaces are used by the default
 * @ref ldl_tsm. They are documented here to assist integrators that
 * choose to reimplement @ref ldl_tsm.
 * 
 * These implementations are provided for the purpose of evaluating LDL
 * and no effort has been made to ensure they are hardened against
 * attacks.
 * 
 * @{
 * */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/** AES state */
struct ldl_aes_ctx {

    uint8_t k[240U];
    uint8_t r;
};

/** Initialise AES block cipher
 * 
 * @param[in] ctx
 * @param[in] key pointer to 16 byte key
 * 
 * */
void LDL_AES_init(struct ldl_aes_ctx *ctx, const void *key);

/** Encrypt a block of data
 * 
 * @param[in] ctx state previously initialised by LDL_AES_init()
 * @param[in] s pointer to 16 byte block of data (any alignment)
 * 
 * */
void LDL_AES_encrypt(const struct ldl_aes_ctx *ctx, void *s);

/** Decrypt a block of data
 * 
 * @param[in] ctx state previously initialised by LDL_AES_init()
 * @param[in] s pointer to 16 byte block of data (any alignment)
 * 
 * */
void LDL_AES_decrypt(const struct ldl_aes_ctx *ctx, void *s);

#ifdef __cplusplus
}
#endif

/** @} */
#endif
