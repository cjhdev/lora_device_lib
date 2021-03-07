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

#include "_device.h"
#include <inttypes.h>

#if MBED_CONF_RTOS_PRESENT

using namespace LDL;

/* constructors *******************************************************/

Device::Device(Store &store, SM &sm, Radio &radio, osPriority prio) :
    mac(store, sm, radio),
    worker_thread(prio, 2048),
    queue_semaphore(1),
    data_semaphore(1),
    radio(radio)
{
    mac.set_event_cb(callback(this, &Device::handle_mac_event));
    mac.set_wakeup_cb(callback(this, &Device::handle_wakeup));
}

/* protected **********************************************************/

void
Device::do_work()
{
    work_semaphore.release();
}

void
Device::worker()
{
    uint32_t next = 0U;

    for(;;){

        if(next < 100000){

            sleep_manager_lock_deep_sleep();
        }

        work_semaphore.acquire();

        if(next < 100000){

            sleep_manager_unlock_deep_sleep();
        }

        do{

            mac.process();
            next = mac.ticks_until_next_event();
        }
        while(next == 0U);

        event.dispatch(0);

        next = mac.ticks_until_next_event();

        if(next == 0U){

            do_work();
        }
        else{

            if(next < UINT32_MAX){

                timeout.attach(callback(this, &Device::do_work), std::chrono::microseconds(next));
            }
            else{

                timeout.detach();
            }
        }
    }
}

void
Device::do_data(enum ldl_mac_status *retval, const struct data_arg *arg)
{
    if(arg->op == LDL_OP_DATA_CONFIRMED){

        *retval = mac.confirmed(arg->port, arg->data, arg->len, arg->opts);
    }
    else{

        *retval = mac.unconfirmed(arg->port, arg->data, arg->len, arg->opts);
    }

    switch(*retval){
    case LDL_STATUS_OK:
        op_flags.clear();
        break;
    case LDL_STATUS_NOCHANNEL:
        band_flags.clear();
        break;
    default:
        break;
    }

    status_semaphore.release();
}

void
Device::do_otaa(enum ldl_mac_status *retval)
{
    *retval = mac.otaa();

    switch(*retval){
    case LDL_STATUS_OK:
        op_flags.clear();
        break;
    default:
        break;
    }

    status_semaphore.release();
}

void
Device::do_forget()
{
    mac.forget();
    status_semaphore.release();
}

void
Device::do_cancel()
{
    mac.forget();
    status_semaphore.release();
}

void
Device::do_set_rate(enum ldl_mac_status *retval, uint8_t value)
{
    *retval = mac.set_rate(value);
    status_semaphore.release();
}

void
Device::do_get_rate(uint8_t *retval)
{
    *retval = mac.get_rate();
    status_semaphore.release();
}

void
Device::do_set_power(enum ldl_mac_status *retval, uint8_t value)
{
    *retval = mac.set_power(value);
    status_semaphore.release();
}

void
Device::do_get_power(uint8_t *retval)
{
    *retval = mac.get_power();
    status_semaphore.release();
}

void
Device::do_set_adr(bool value)
{
    mac.set_adr(value);
    status_semaphore.release();
}

void
Device::do_get_adr(bool *retval)
{
    *retval = mac.get_adr();
    status_semaphore.release();
}

void
Device::do_joined(bool *retval)
{
    *retval = mac.joined();
    status_semaphore.release();
}

void
Device::do_ready(bool *retval)
{
    *retval = mac.ready();
    status_semaphore.release();
}

void
Device::do_set_max_dcycle(uint8_t value)
{
    mac.set_max_dcycle(value);
    status_semaphore.release();
}

void
Device::do_get_max_dcycle(uint8_t *retval)
{
    *retval = mac.get_max_dcycle();
    status_semaphore.release();
}

void
Device::do_get_fpending(bool *retval)
{
    *retval = mac.get_fpending();
    status_semaphore.release();
}

void
Device::do_entropy(enum ldl_mac_status *retval)
{
    *retval = mac.entropy();
    status_semaphore.release();
}

void
Device::handle_mac_event(enum ldl_mac_response_type type, const union ldl_mac_response_arg *arg)
{
    LDL_DEBUG("%s: event: %i", __FUNCTION__, type);

    switch(type){
    default:
        break;

    case LDL_MAC_RX:
        cb_mutex.lock();
        {
            if(rx_cb){

                rx_cb(arg->rx.port, arg->rx.data, arg->rx.size);
            }
        }
        cb_mutex.unlock();
        break;

    case LDL_MAC_LINK_STATUS:
        cb_mutex.lock();
        {
            if(link_status_cb){

                link_status_cb(arg->link_status.margin, arg->link_status.gwCount);
            }
        }
        cb_mutex.unlock();
        break;

    case LDL_MAC_DEVICE_TIME:
        cb_mutex.lock();
        {
            if(device_time_cb){

                device_time_cb(arg->device_time.seconds, arg->device_time.fractions);
            }
        }
        cb_mutex.unlock();
        break;

    case LDL_MAC_CHANNEL_READY:
        band_flags.set(1);
        break;

    case LDL_MAC_ENTROPY:
        entropy_value = arg->entropy.value;
        op_flags.set(1 << type);
        break;

    case LDL_MAC_OP_CANCELLED:
    case LDL_MAC_OP_ERROR:
    case LDL_MAC_JOIN_COMPLETE:
    case LDL_MAC_DATA_COMPLETE:
    case LDL_MAC_DATA_TIMEOUT:
    case LDL_MAC_JOIN_EXHAUSTED:
        op_flags.set(1 << type);
        break;
    }


    cb_mutex.lock();
    {
        if(event_cb){

            event_cb(type, arg);
        }
    }
    cb_mutex.unlock();
}

void
Device::handle_wakeup()
{
    work_semaphore.release();
}

enum ldl_mac_status
Device::data_service(const struct data_arg *arg, Kernel::Clock::duration timeout)
{
    enum ldl_mac_status retval;
    uint32_t flags;
    Kernel::Clock::time_point ready_before;
    bool exit_loop;

    /* one data service at a time to prevent
     * one thread stealing bands from another thread */
    if(data_semaphore.try_acquire_until(ready_before)){

        if(timeout == Kernel::Clock::duration::max()){

            ready_before = Kernel::Clock::time_point::max();
        }
        else{

            ready_before = Kernel::Clock::now() + timeout;

            if(ready_before < Kernel::Clock::now()){

                ready_before = Kernel::Clock::time_point::max();
            }
        }

        do{

            exit_loop = true;

            queue_semaphore.acquire();

            (void)event.call(callback(this, &Device::do_data), &retval, arg);

            work_semaphore.release();

            status_semaphore.acquire();

            queue_semaphore.release();

            switch(retval){
            case LDL_STATUS_OK:

                flags = op_flags.wait_any(0U
                    | (1 << LDL_MAC_DATA_COMPLETE)
                    | (1 << LDL_MAC_DATA_TIMEOUT)
                    | (1 << LDL_MAC_OP_CANCELLED)
                    | (1 << LDL_MAC_OP_ERROR)
                );

                switch(flags){
                case (1 << LDL_MAC_DATA_COMPLETE):
                    break;
                case (1 << LDL_MAC_DATA_TIMEOUT):
                    retval = LDL_STATUS_NOACK;
                    break;
                case (1 << LDL_MAC_OP_CANCELLED):
                    retval = LDL_STATUS_CANCELLED;
                    break;
                case (1 << LDL_MAC_OP_ERROR):
                default:
                    retval = LDL_STATUS_ERROR;
                    break;
                }
                break;

            case LDL_STATUS_MACPRIORITY:

                flags = op_flags.wait_any(0U
                    | (1 << LDL_MAC_DATA_COMPLETE)
                    | (1 << LDL_MAC_DATA_TIMEOUT)
                    | (1 << LDL_MAC_OP_CANCELLED)
                    | (1 << LDL_MAC_OP_ERROR)
                );

                switch(flags){
                case (1 << LDL_MAC_DATA_COMPLETE):
                    exit_loop = false;   // now try to send user data (again)
                    break;
                case (1 << LDL_MAC_OP_CANCELLED):
                    retval = LDL_STATUS_CANCELLED;
                    break;
                case (1 << LDL_MAC_OP_ERROR):
                default:
                    retval = LDL_STATUS_ERROR;
                    break;
                }
                break;

            case LDL_STATUS_NOCHANNEL:

                if(!band_flags.wait_any_until(1, ready_before)){

                    retval = LDL_STATUS_TIMEOUT;
                    exit_loop = false;
                }
                break;

            default:
                break;
            }
        }
        while(!exit_loop);

        data_semaphore.release();
    }
    else{

        retval = LDL_STATUS_TIMEOUT;
    }

    return retval;
}

/* public methods *****************************************************/

bool
Device::start(enum ldl_region region)
{
    bool retval = false;

    if(mac.start(region)){

        worker_thread.start(callback(this, &Device::worker));

        work_semaphore.release();

        retval = true;
    }

    return retval;
}

enum ldl_mac_status
Device::otaa(Kernel::Clock::duration timeout)
{
    enum ldl_mac_status retval;
    uint32_t flags;
    Kernel::Clock::time_point ready_before;

    if(timeout == Kernel::Clock::duration::max()){

        ready_before = Kernel::Clock::time_point::max();
    }
    else{

        ready_before = Kernel::Clock::now() + timeout;

        if(ready_before < Kernel::Clock::now()){

            ready_before = Kernel::Clock::time_point::max();
        }
    }

    queue_semaphore.acquire();

    event.call(callback(this, &Device::do_otaa), &retval);

    work_semaphore.release();

    status_semaphore.acquire();

    queue_semaphore.release();

    if(retval == LDL_STATUS_OK){

        flags = op_flags.wait_any_until(0U
            | (1 << LDL_MAC_JOIN_COMPLETE)
            | (1 << LDL_MAC_OP_CANCELLED)
            | (1 << LDL_MAC_JOIN_EXHAUSTED),
            ready_before
        );

        switch(flags){
        case (1 << LDL_MAC_JOIN_COMPLETE):
            break;
        case (1 << LDL_MAC_OP_CANCELLED):
            retval = LDL_STATUS_CANCELLED;
            break;
        case (1 << LDL_MAC_JOIN_EXHAUSTED):
            retval = LDL_STATUS_DEVNONCE;
            break;
        default:
            forget();
            retval = LDL_STATUS_TIMEOUT;
            break;
        }
    }

    return retval;
}

enum ldl_mac_status
Device::unconfirmed(uint8_t port, const void *data, uint8_t len, const struct ldl_mac_data_opts *opts)
{
    struct data_arg arg = {
        .op = LDL_OP_DATA_UNCONFIRMED,
        .port = port,
        .data = data,
        .len = len,
        .opts = opts
    };

    return data_service(&arg, Kernel::Clock::duration::max());
}

enum ldl_mac_status
Device::confirmed(uint8_t port, const void *data, uint8_t len, const struct ldl_mac_data_opts *opts)
{
    struct data_arg arg = {
        .op = LDL_OP_DATA_CONFIRMED,
        .port = port,
        .data = data,
        .len = len,
        .opts = opts
    };

    return data_service(&arg, Kernel::Clock::duration::max());
}

enum ldl_mac_status
Device::entropy(uint32_t &value)
{
    enum ldl_mac_status retval;
    uint32_t flags;

    value = 0;

    queue_semaphore.acquire();

    (void)event.call(callback(this, &Device::do_entropy), &retval);

    work_semaphore.release();

    status_semaphore.acquire();

    queue_semaphore.release();

    if(retval == LDL_STATUS_OK){

        flags = op_flags.wait_any(0U
            | (1 << LDL_MAC_ENTROPY)
            | (1 << LDL_MAC_OP_ERROR)
            | (1 << LDL_MAC_OP_CANCELLED)
        );

        switch(flags){
        case (1 << LDL_MAC_ENTROPY):
            value = entropy_value;
            break;
        case (1 << LDL_MAC_OP_CANCELLED):
            retval = LDL_STATUS_CANCELLED;
            break;
        case (1 << LDL_MAC_OP_ERROR):
        default:
            retval = LDL_STATUS_ERROR;
            break;
        }
    }

    return retval;
}

void
Device::forget()
{
    accessor(callback(this, &Device::do_forget));
}

enum ldl_mac_status
Device::set_rate(uint8_t value)
{
    enum ldl_mac_status retval;

    accessor(callback(this, &Device::do_set_rate), &retval, value);

    return retval;
}

uint8_t
Device::get_rate()
{
    uint8_t retval;

    accessor(callback(this, &Device::do_get_rate), &retval);

    return retval;
}

enum ldl_mac_status
Device::set_power(uint8_t value)
{
    enum ldl_mac_status retval;

    accessor(callback(this, &Device::do_set_power), &retval, value);

    return retval;
}

uint8_t
Device::get_power()
{
    uint8_t retval;

    accessor(callback(this, &Device::do_get_power), &retval);

    return retval;
}

bool
Device::joined()
{
    bool retval;

    accessor(callback(this, &Device::do_joined), &retval);

    return retval;
}

bool
Device::ready()
{
    bool retval;

    accessor(callback(this, &Device::do_ready), &retval);

    return retval;
}

void
Device::set_adr(bool value)
{
    accessor(callback(this, &Device::do_set_adr), value);
}

bool
Device::get_adr()
{
    bool retval;

    accessor(callback(this, &Device::do_get_adr), &retval);

    return retval;
}

void
Device::set_max_dcycle(uint8_t value)
{
    accessor(callback(this, &Device::do_set_max_dcycle), value);
}

uint8_t
Device::get_max_dcycle()
{
    uint8_t retval;

    accessor(callback(this, &Device::do_get_max_dcycle), &retval);

    return retval;
}

void
Device::cancel()
{
    accessor(callback(this, &Device::do_cancel));
}

bool
Device::get_fpending()
{
    bool retval;

    accessor(callback(this, &Device::do_get_fpending), &retval);

    return retval;
}

#endif
