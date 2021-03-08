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

#ifndef MBED_LDL_DEFAULT_STORE_H
#define MBED_LDL_DEFAULT_STORE_H

#include "store.h"

namespace LDL {

    /** Stores all state in volatile memory.
     *
     * Suitable for demonstrations.
     *
     * */
    class DefaultStore : public Store {

        private:

            const void *dev_eui;
            const void *join_eui;

            uint16_t next_dev_nonce;
            uint32_t join_nonce;

        public:

            DefaultStore(const void *dev_eui, const void *join_eui)
                :
                Store(),
                dev_eui(dev_eui),
                join_eui(join_eui),
                next_dev_nonce(0),
                join_nonce(0)
            {
            }

            void get_init_params(struct init_params *params)
            {
                params->dev_nonce = next_dev_nonce;
                params->join_nonce = join_nonce;
                (void)memcpy(params->dev_eui, dev_eui, sizeof(params->dev_eui));
                (void)memcpy(params->join_eui, join_eui, sizeof(params->join_eui));
            }

            size_t get_session(void *data, size_t max)
            {
                return 0U;
            }

            void save_next_dev_nonce(uint16_t next_dev_nonce)
            {
                this->next_dev_nonce = next_dev_nonce;
            }

            void save_join_accept(uint32_t join_nonce)
            {
                this->join_nonce = join_nonce;
            }

            void save_session(const void *data, size_t size)
            {
            }

            void reset()
            {
                next_dev_nonce = 0U;
                join_nonce = 0U;
            }
    };
};

#endif
