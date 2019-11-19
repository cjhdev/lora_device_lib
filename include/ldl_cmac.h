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
 
#ifndef LDL_CMAC_H
#define LDL_CMAC_H

/** @file */

/**
 * @addtogroup ldl_crypto
 * 
 * @{
 * */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

struct ldl_aes_ctx;

/** CMAC state */
struct ldl_cmac_ctx {

    const struct ldl_aes_ctx *aes_ctx;
    uint8_t m[16U];
    uint8_t x[16U];
    uint32_t size;
};

/** Initialise CMAC state
 * 
 * @param[in] ctx
 * @param[in] aes_ctx block cipher state
 * 
 * */
void LDL_CMAC_init(struct ldl_cmac_ctx *ctx, const struct ldl_aes_ctx *aes_ctx);

/** Update CMAC state
 * 
 * @param[in] ctx
 * @param[in] data
 * @param[in] len
 * 
 * */
void LDL_CMAC_update(struct ldl_cmac_ctx *ctx, const void *data, uint8_t len);

/** Produce CMAC output from current state
 * 
 * @param[in]   ctx
 * @param[out]  out
 * @param[in]   outMax 
 * 
 * */
void LDL_CMAC_finish(const struct ldl_cmac_ctx *ctx, void *out, uint8_t outMax);

#ifdef __cplusplus
}
#endif

/** @} */
#endif
