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

#ifndef MBED_LDL_MAC_H
#define MBED_LDL_MAC_H

#include "mbed.h"

#include "ldl_mac.h"
#include "ldl_system.h"

#include "sm.h"
#include "radio.h"
#include "store.h"

namespace LDL {

    class MAC {

        protected:

            enum mac_run_state {

                OFF,
                ON

            } run_state;

            DigitalOut signal;

            struct ldl_mac mac;
            Radio& radio;
            SM& sm;
            Store& store;
            LowPowerTimer timer;

            Callback<void(enum ldl_mac_response_type, const union ldl_mac_response_arg *)> event_cb;
            Callback<void(void)> wakeup_cb;

            static MAC *to_obj(void *ptr);

            static void app_handler(void *app, enum ldl_mac_response_type type, const union ldl_mac_response_arg *arg);

            static uint32_t _ticks(void *app);
            static uint32_t _rand(void *app);
            static uint8_t _get_battery_level(void *app);

        public:

            /* called by radio in ISR */
            void handle_radio_event();

            MAC(Store& store, SM& sm, LDL::Radio& radio);

            bool start(enum ldl_region region);

            enum ldl_mac_status unconfirmed(uint8_t port, const void *data, uint8_t len, const struct ldl_mac_data_opts *opts = NULL);
            enum ldl_mac_status confirmed(uint8_t port, const void *data, uint8_t len, const struct ldl_mac_data_opts *opts = NULL);

            enum ldl_mac_status otaa();

            void forget();
            void cancel();

            enum ldl_mac_status entropy();

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

            uint32_t ticks_until_next_event();

            void process();

            void set_event_cb(Callback<void(enum ldl_mac_response_type, const union ldl_mac_response_arg *)> cb)
            {
                event_cb = cb;
            }

            void set_wakeup_cb(Callback<void(void)> cb)
            {
                wakeup_cb = cb;
            }
    };

};

#endif
