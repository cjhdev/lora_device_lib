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

#ifndef MBED_LDL_SM_H
#define MBED_LDL_SM_H

#include "mbed.h"
#include "ldl_sm_internal.h"
#include "ldl_sm.h"

namespace LDL {

    /** Abstract Security Module
     *
     * Security Modules will likely differ in terms of:
     *
     * - which cryptographic implementations they use
     * - how they store the root keys
     * - how they handle the keys
     *
     * The session keys never need to be persisted in non-volatile memory
     * since LDL will always re-derive from from the root keys and restored
     * non-secret state.
     *
     * */
    class SM {

        protected:

            static SM *to_obj(void *self);

            static void _begin_update_session_key(struct ldl_sm *self);
            static void _end_update_session_key(struct ldl_sm *self);
            static void _update_session_key(struct ldl_sm *self, enum ldl_sm_key key_desc, enum ldl_sm_key root_desc, const void *iv);
            static uint32_t _mic(struct ldl_sm *self, enum ldl_sm_key desc, const void *hdr, uint8_t hdrLen, const void *data, uint8_t dataLen);
            static void _ecb(struct ldl_sm *self, enum ldl_sm_key desc, void *b);
            static void _ctr(struct ldl_sm *self, enum ldl_sm_key desc, const void *iv, void *data, uint8_t len);

            virtual void begin_update_session_key() = 0;
            virtual void end_update_session_key() = 0;
            virtual void update_session_key(enum ldl_sm_key key_desc, enum ldl_sm_key root_desc, const void *iv) = 0;
            virtual uint32_t mic(enum ldl_sm_key desc, const void *hdr, uint8_t hdrLen, const void *data, uint8_t dataLen) = 0;
            virtual void ecb(enum ldl_sm_key desc, void *b) = 0;
            virtual void ctr(enum ldl_sm_key desc, const void *iv, void *data, uint8_t len) = 0;

        public:

            static const struct ldl_sm_interface interface;
    };
};

#endif
