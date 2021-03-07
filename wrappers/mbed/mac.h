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

    /**
     * LDL::MAC is minimal C++ wrapper around #ldl_mac.
     *
     * The methods are not thread-safe. LDL::MAC is useful
     * for resource constrained targets and/or targets compiling
     * for the bare metal profile.
     *
     * Consider using LDL::Device if you require thread-safe interfaces.
     *
     * */
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

            /**
             * Create a MAC
             *
             * @param[in] store     responsible for handling non-secrets
             * @param[in] sm        responsible for handling secrets
             * @param[in] radio
             *
             * */
            MAC(Store& store, SM& sm, LDL::Radio& radio);

            /**
             * MAC must be started in a specific region before
             * the methods work.
             *
             * @param[in] region #ldl_region
             *
             * @retval true     MAC started
             * @retval false    MAC did not start
             *
             * */
            bool start(enum ldl_region region);

            /**
             * Send data using unconfirmed service
             *
             * @param[in] port
             * @param[in] data
             * @param[in] len
             * @param[in] opts
             *
             * @return #ldl_mac_status
             *
             * */
            enum ldl_mac_status unconfirmed(uint8_t port, const void *data, uint8_t len, const struct ldl_mac_data_opts *opts = NULL);

            /**
             * Send data using unconfirmed service
             *
             * @param[in] port
             * @param[in] data
             * @param[in] len
             * @param[in] opts
             *
             * @return #ldl_mac_status
             *
             * */
            enum ldl_mac_status confirmed(uint8_t port, const void *data, uint8_t len, const struct ldl_mac_data_opts *opts = NULL);

            /**
             * Initiate over the air activation
             *
             * This will continue to run forever until:
             *
             * - device is activated by server
             * - application calls cancel()
             * - application calls forget()
             * - devNonce has incremented to UINT16_MAX
             *
             * @return #ldl_mac_status
             *
             * */
            enum ldl_mac_status otaa();

#ifdef LDL_ENABLE_ABP
            enum ldl_mac_status abp(uint32_t devAddr);
#endif

            /**
             * Forget the network and return to an inactive state
             *
             * */
            void forget();

            /**
             * Cancel the current operation
             *
             * */
            void cancel();

            /**
             * Initiate entropy operation
             *
             * @return #ldl_mac_status
             *
             * */
            enum ldl_mac_status entropy();

            /**
             * Set data rate
             *
             * @return #ldl_mac_status
             *
             * */
            enum ldl_mac_status set_rate(uint8_t value);

            /**
             * Get data rate
             *
             * @return rate
             *
             * */
            uint8_t get_rate();

            /**
             * Set transmit power
             *
             * @return @ldl_mac_status
             *
             * */
            enum ldl_mac_status set_power(uint8_t value);

            /**
             * Get transmit power
             *
             * @return power
             *
             * */
            uint8_t get_power();

            /**
             * Set ADR mode
             *
             * @param[in] value
             *
             * */
            void set_adr(bool value);

            /**
             * Get ADR mode
             *
             * @retval true     ADR is enabled
             * @retval false    ADR is disabled
             *
             * */
            bool get_adr();

            /**
             * Get joined status
             *
             * @retval true MAC is joined to a network
             * @retval false MAC is not joined
             *
             * */
            bool joined();

            /**
             * Get ready status
             *
             * @retval true MAC is ready to transmit
             * @retval false MAC is not ready
             *
             * */
            bool ready();

            /**
             * Set maximum duty cycle
             *
             * @param[in] value
             *
             * */
            void set_max_dcycle(uint8_t value);

            /**
             * Get maximum duty cycle setting
             *
             * @return maximum duty cycle
             *
             * */
            uint8_t get_max_dcycle();

            /**
             * Get the number of ticks until the next scheduled event
             *
             * @return ticks
             *
             * */
            uint32_t ticks_until_next_event();

            /**
             * Call to process events
             *
             * */
            void process();

            /**
             * Set event callback
             *
             * @param[in] callback
             *
             * */
            void set_event_cb(Callback<void(enum ldl_mac_response_type, const union ldl_mac_response_arg *)> cb)
            {
                event_cb = cb;
            }

            /**
             * Set wakeup callback
             *
             * This callback is called when that radio driver
             * receives an interrupt and can be used to ensure
             * the app wakes up from whatever it might be blocking
             * on.
             *
             * @param[in] callback
             *
             * */
            void set_wakeup_cb(Callback<void(void)> cb)
            {
                wakeup_cb = cb;
            }

            bool get_f_pending();
            bool get_ack_pending();
    };

};

#endif
