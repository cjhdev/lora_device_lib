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

#ifndef LDL_SM_H
#define LDL_SM_H

/** @file */

#ifdef __cplusplus
extern "C" {
#endif

#include "ldl_platform.h"
#include "ldl_sm_internal.h"

#include <stdint.h>

#define LDL_KEY_SIZE 16U

struct ldl_key {

    uint8_t value[LDL_KEY_SIZE];
};

/** default in-memory security module state */
struct ldl_sm {
#ifdef LDL_DISABLE_POINTONE
    struct ldl_key keys[3U];
#else
    struct ldl_key keys[8U];
#endif
};

#ifdef LDL_DISABLE_POINTONE
/**
 * Initialise Default Security Module with root key
 *
 * @param[in] self      #ldl_sm
 * @param[in] appKey    pointer to 16 byte field
 *
 * */
void LDL_SM_init(struct ldl_sm *self, const void *appKey);
#else
/**
 * Initialise Default Security Module with root keys
 *
 * @param[in] self      #ldl_sm
 * @param[in] appKey    pointer to 16 byte field
 * @param[in] nwkKey    pointer to 16 byte field
 *
 * */
void LDL_SM_init(struct ldl_sm *self, const void *appKey, const void *nwkKey);
#endif

#ifdef __cplusplus
}
#endif

/** @} */

#endif
