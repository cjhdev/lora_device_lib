/* Copyright (c) 2020 Cameron Harper
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

#include "sm.h"

using namespace LDL;

const struct ldl_sm_interface SM::interface = {
    .update_session_key = SM::_update_session_key,
    .begin_update_session_key = SM::_begin_update_session_key,
    .end_update_session_key = SM::_end_update_session_key,
    .mic = SM::_mic,
    .ecb = SM::_ecb,
    .ctr = SM::_ctr
};

/* constructors *******************************************************/

/* protected static ***************************************************/

SM *
SM::to_obj(void *self)
{
    return static_cast<SM *>(self);
}

void
SM::_begin_update_session_key(struct ldl_sm *self)
{
    to_obj(self)->begin_update_session_key();
}

void
SM::_end_update_session_key(struct ldl_sm *self)
{
    to_obj(self)->end_update_session_key();
}

void
SM::_update_session_key(struct ldl_sm *self, enum ldl_sm_key key_desc, enum ldl_sm_key root_desc, const void *iv)
{
    to_obj(self)->update_session_key(key_desc, root_desc, iv);
}

uint32_t
SM::_mic(struct ldl_sm *self, enum ldl_sm_key desc, const void *hdr, uint8_t hdrLen, const void *data, uint8_t dataLen)
{
    return to_obj(self)->mic(desc, hdr, hdrLen, data, dataLen);
}

void
SM::_ecb(struct ldl_sm *self, enum ldl_sm_key desc, void *b)
{
    to_obj(self)->ecb(desc, b);
}

void
SM::_ctr(struct ldl_sm *self, enum ldl_sm_key desc, const void *iv, void *data, uint8_t len)
{
    to_obj(self)->ctr(desc, iv, data, len);
}

/* protected **********************************************************/
