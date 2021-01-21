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

#ifndef MBED_LDL_STORE_H
#define MBED_LDL_STORE_H

#include <string.h>
#include <stdint.h>

namespace LDL {

    class Store {

        public:

            struct init_params {

                uint8_t dev_eui[8];
                uint8_t join_eui[8];
                uint32_t join_nonce;
                uint16_t dev_nonce;
            };

            /** Get the set of parameters required for LDL_MAC_init()
             *  except for session.
             *
             * @param[out] params   #init_params
             *
             * */
            virtual void get_init_params(struct init_params *params);

            /** Read session from cache
             *
             * @param[out] data     destination buffer
             * @param[in] max       maximum size of destination buffer
             *
             * @retval  bytes written to data
             *
             * @retval 0    no session OR buffer is too small
             *
             * The max parameter will be set by LDL. If the saved session
             * is larger than expected then LDL must have been downgraded
             * relative to the session data, and therefore, LDL won't be
             * able to understand it.
             *
             * */
            virtual size_t get_session(void *data, size_t max);

            /** These values will be needed the next time an OTAA occurs
             *
             * @param[in] join_nonce
             * @param[in] next_dev_nonce
             *
             * */
            virtual void save_join_accept(uint32_t join_nonce, uint16_t next_dev_nonce);

            /** Save session data
             *
             * @param[in] data
             * @param[in] size  size of data
             *
             * */
            virtual void save_session(const void *data, size_t size);
    };
};

#endif
