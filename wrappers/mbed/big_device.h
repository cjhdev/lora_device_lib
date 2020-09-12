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

#ifndef MBED_LDL_DEVICE_H
#define MBED_LDL_DEVICE_H

#include "mbed.h"
#include "mac.h"

#if MBED_CONF_RTOS_PRESENT

namespace LDL {

    class Device {

        protected:

            EventQueue event;

            MAC mac;

            Thread event_thread;

            volatile bool done;
            Semaphore api;
            Mutex mutex;

            void begin_api();
            void wait_until_api_done();
            void notify_api();

            /* executed from the event queue */
            void do_unconfirmed(enum ldl_mac_status *retval, uint8_t port, const void *data, uint8_t len, const struct ldl_mac_data_opts *opts);
            void do_confirmed(enum ldl_mac_status *retval, uint8_t port, const void *data, uint8_t len, const struct ldl_mac_data_opts *opts);
            void do_otaa(enum ldl_mac_status *retval);
            void do_forget();
            void do_set_rate(enum ldl_mac_status *retval, uint8_t value);
            void do_get_rate(uint8_t *retval);
            void do_set_power(enum ldl_mac_status *retval, uint8_t value);
            void do_get_power(uint8_t *retval);
            void do_set_adr(bool value);
            void do_get_adr(bool *retval);
            void do_joined(bool *retval);
            void do_ready(bool *retval);
            void do_set_max_dcycle(uint8_t value);
            void do_get_max_dcycle(uint8_t *retval);

        public:

            Device(Store& store, SM& sm, Radio& radio);

            bool start(enum ldl_region region);
            void stop();

            enum ldl_mac_status unconfirmed(uint8_t port, const void *data, uint8_t len, const struct ldl_mac_data_opts *opts = NULL);
            enum ldl_mac_status confirmed(uint8_t port, const void *data, uint8_t len, const struct ldl_mac_data_opts *opts = NULL);

            enum ldl_mac_status otaa();

            void forget();

            enum ldl_mac_status set_rate(uint8_t value);
            uint8_t get_rate();

            enum ldl_mac_status set_power(uint8_t value);
            uint8_t get_power();

            void set_adr(bool value);
            bool get_adr();

            bool joined();
            bool ready();

            void set_max_dcycle(uint8_t value);
            uint8_t get_max_dcycle();

            void set_event_cb(Callback<void(enum ldl_mac_response_type, const union ldl_mac_response_arg *)> cb)
            {
                mac.set_event_cb(cb);
            }

            void set_entropy_cb(Callback<void(unsigned int)> cb)
            {
                mac.set_entropy_cb(cb);
            }

            void set_data_cb(Callback<void(uint8_t, const void *, uint8_t)> cb)
            {
                mac.set_data_cb(cb);
            }
    };

};

#endif

#endif
