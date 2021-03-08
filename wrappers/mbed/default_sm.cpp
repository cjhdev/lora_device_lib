/* Copyright (c) 2021 Cameron Harper
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

#include "default_sm.h"

using namespace LDL;

/* constructors *******************************************************/


DefaultSM::DefaultSM(const void *app_key)
{
#if defined(LDL_ENABLE_L2_1_1)
    LDL_SM_init(&state, app_key, app_key);
#else
    LDL_SM_init(&state, app_key);
#endif
}

DefaultSM::DefaultSM(const void *app_key, const void *nwk_key)
{
#if defined(LDL_ENABLE_L2_1_1)
    LDL_SM_init(&state, app_key, nwk_key);
#else
    LDL_SM_init(&state, nwk_key);
#endif
}

/* protected static ***************************************************/


/* protected **********************************************************/

void
DefaultSM::update_session_key(enum ldl_sm_key key_desc, enum ldl_sm_key root_desc, const void *iv)
{
    LDL_SM_updateSessionKey(&state, key_desc, root_desc, iv);
}

uint32_t
DefaultSM::mic(enum ldl_sm_key desc, const void *hdr, uint8_t hdrLen, const void *data, uint8_t dataLen)
{
    return LDL_SM_mic(&state, desc, hdr, hdrLen, data, dataLen);
}

void
DefaultSM::ecb(enum ldl_sm_key desc, void *b)
{
    LDL_SM_ecb(&state, desc, b);
}

void
DefaultSM::ctr(enum ldl_sm_key desc, const void *iv, void *data, uint8_t len)
{
    LDL_SM_ctr(&state, desc, iv, data, len);
}

