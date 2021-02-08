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

#ifndef MBED_LDL_DEFAULT_SM_H
#define MBED_LDL_DEFAULT_SM_H

#include "sm.h"

namespace LDL {

    /**
     * A default Security Module suitable for demo apps.
     *
     * Uses the default LDL cryptographic implementation and takes
     * pointers to the root keys on construction.
     *
     * */
    class DefaultSM : public SM {

        protected:

            struct ldl_sm state;

            void begin_update_session_key();
            void end_update_session_key();
            void update_session_key(enum ldl_sm_key key_desc, enum ldl_sm_key root_desc, const void *iv);
            uint32_t mic(enum ldl_sm_key desc, const void *hdr, uint8_t hdrLen, const void *data, uint8_t dataLen);
            void ecb(enum ldl_sm_key desc, void *b);
            void ctr(enum ldl_sm_key desc, const void *iv, void *data, uint8_t len);

        public:

            DefaultSM(const void *app_key);
            DefaultSM(const void *app_key, const void *nwk_key);
    };

};

#endif
