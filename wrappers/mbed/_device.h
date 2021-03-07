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

    /**
     * LDL::Device is the thread-safe interface for the MBED LDL wrapper.
     *
     * A lot of the methods are named the same as with LDL::MAC, the
     * main difference being the operation methods will block until
     * they are complete.
     *
     * This class is not compiled if you are building for the bare metal
     * profile.
     *
     * */
    class Device {

        protected:

            EventQueue event;

            MAC mac;

            Thread worker_thread;

            Semaphore work_semaphore;       // makes worker_thread run
            Semaphore status_semaphore;     // indicates status is available
            Semaphore queue_semaphore;      // limits access to event queue
            Semaphore data_semaphore;       // limits access to data service

            Mutex cb_mutex;

            LowPowerTimeout timeout;
            Radio &radio;

            EventFlags op_flags;
            EventFlags band_flags;

            uint32_t entropy_value;

            Callback<void(uint8_t, const void *, uint8_t)> rx_cb;
            Callback<void(uint8_t, uint8_t)> link_status_cb;
            Callback<void(uint32_t, uint8_t)> device_time_cb;
            Callback<void(enum ldl_mac_response_type, const union ldl_mac_response_arg *)> event_cb;

            /* run by worker_thread */
            void worker();

            /* called by mac */
            void handle_mac_event(enum ldl_mac_response_type type, const union ldl_mac_response_arg *arg);

            /* called by mac in ISR */
            void handle_wakeup();

            struct data_arg {

                ldl_mac_operation op;
                uint8_t port;
                const void *data;
                uint8_t len;
                const struct ldl_mac_data_opts *opts;
            };

            /* executed from the event queue */
            void do_data(enum ldl_mac_status *retval, const struct data_arg *arg);
            void do_otaa(enum ldl_mac_status *retval);
#ifdef LDL_ENABLE_ABP
            void do_abp(enum ldl_mac_status *retval, uint32_t devAddr);
#endif
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
            void do_entropy(enum ldl_mac_status *retval);
            void do_cancel();
            void do_get_f_pending(bool *retval);
            void do_get_ack_pending(bool *retval);

            /* used to release work_semaphore in future */
            void do_work();

            enum ldl_mac_status data_service(const struct data_arg *arg, Kernel::Clock::duration timeout);

            /* used by getter/setter methods */
            template<typename F, typename ...Args>
            void accessor(F f, Args ...args)
            {
                queue_semaphore.acquire();

                (void)event.call(f, args...);

                work_semaphore.release();

                status_semaphore.acquire();

                queue_semaphore.release();
            }

        public:

            /**
             * Create a Device.
             *
             * @param[in] store     responsible for handling non-secrets
             * @param[in] sm        responsible for handling secrets
             * @param[in] radio
             * @param[in] prio      priority of the worker
             *
             * */
            Device(Store& store, SM& sm, LDL::Radio& radio, osPriority prio = osPriorityNormal);

            /**
             * Device must be started in a specific region before
             * the methods work.
             *
             * @param[in] region #ldl_region
             *
             * @retval true     Device started
             * @retval false    Device did not start
             *
             * */
            bool start(enum ldl_region region);

            /**
             * Initiate over the air activation
             *
             * This method block forever until:
             *
             * - device is activated by server
             * - application calls cancel()
             * - application calls forget()
             * - more than 65535 join request messages have been sent
             *
             * @return #ldl_mac_status
             *
             * */
            enum ldl_mac_status otaa(Kernel::Clock::duration timeout=Kernel::Clock::duration::max());

#ifdef LDL_ENABLE_ABP
            enum ldl_mac_status abp(uint32_t devAddr);
#endif

            /**
             * Send data using an unconfirmed service.
             *
             * This method will block until a radio band is available. Once
             * a band becomes available it will continue to block until the
             * service has completed the full tx-rx1-rx2 cycle.
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
             * Send data using a confirmed service.
             *
             * This method will block until a radio band is available. Once
             * a band becomes available it will continue to block until the
             * service has completed the full tx-rx1-rx2 cycle.
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
             * Forget the network
             *
             * */
            void forget();

            /**
             * Cancel the current operation
             *
             * */
            void cancel();

            /**
             * Get entropy from the radio
             *
             * @param[out] value    random number from radio
             *
             * @return #ldl_mac_status
             *
             * */
            enum ldl_mac_status entropy(uint32_t &value);

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
             * Set an event callback handler
             *
             * @param[in] cb
             *
             * */
            void set_event_cb(Callback<void(enum ldl_mac_response_type, const union ldl_mac_response_arg *)> cb)
            {
                cb_mutex.lock();
                {
                    event_cb = cb;
                }
                cb_mutex.unlock();
            }

            /**
             * Set an downlink callback handler.
             *
             * This is how the application receives messages from server
             * to device.
             *
             * @param[in] cb
             *
             * */
            void set_rx_cb(Callback<void(uint8_t, const void *, uint8_t)> cb)
            {
                cb_mutex.lock();
                {
                    rx_cb = cb;
                }
                cb_mutex.unlock();
            }

            /**
             * Set link status answer callback handler
             *
             * @param[in] cb
             *
             * */
            void set_link_status_cb(Callback<void(uint8_t, uint8_t)> cb)
            {
                cb_mutex.lock();
                {
                    link_status_cb = cb;
                }
                cb_mutex.unlock();
            }

            /**
             * Set device time answer callback handler
             *
             * @param[in] cb
             *
             * */
            void set_device_time_cb(Callback<void(uint32_t, uint8_t)> cb)
            {
                cb_mutex.lock();
                {
                    device_time_cb = cb;
                }
                cb_mutex.unlock();
            }

            bool get_f_pending();
            bool get_ack_pending();
    };

};

#endif

#endif
