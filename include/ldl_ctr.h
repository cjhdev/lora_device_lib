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

#ifndef LDL_CTR_H
#define LDL_CTR_H

/** @file */

/**
 * @addtogroup ldl_crypto
 * 
 * @{
 * */

#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"
#include "ldl_aes.h"

struct ldl_aes_ctx;

/** Counter mode encryption
 * 
 * @param[in] ctx   #ldl_aes_ctx
 * @param[in] iv    16 byte initial value
 * @param[in] in    input buffer to encrypt
 * @param[out] out  output buffer (can be same as in)
 * @param[in] len   size of input
 * 
 * */
void LDL_CTR_encrypt(struct ldl_aes_ctx *ctx, const void *iv, const void *in, void *out, uint8_t len);

#ifdef __cplusplus
}
#endif

/** @} */

#endif
