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

#include "big_device.h"
#include <inttypes.h>

#if MBED_CONF_RTOS_PRESENT

using namespace LDL;

/* constructors *******************************************************/

Device::Device(Store &store, SM &sm, Radio &radio) :
    mac(store, sm, radio),
    worker_thread(1024U)
{
}

/* protected **********************************************************/

void
Device::do_work()
{
    work.release();
}

void
Device::worker()
{
    uint32_t next;

    for(;;){

        work.acquire();

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

            timeout.attach(callback(this, &Device::do_work), std::chrono::microseconds(next));
        }
    }
}

void
Device::do_unconfirmed(enum ldl_mac_status *retval, uint8_t port, const void *data, uint8_t len, const struct ldl_mac_data_opts *opts)
{
    *retval = mac.unconfirmed(port, data, len, opts);
    notify_api();
}

void
Device::do_confirmed(enum ldl_mac_status *retval, uint8_t port, const void *data, uint8_t len, const struct ldl_mac_data_opts *opts)
{
    *retval = mac.confirmed(port, data, len, opts);
    notify_api();
}

void
Device::do_otaa(enum ldl_mac_status *retval)
{
    *retval = mac.otaa();
    notify_api();
}

void
Device::do_forget()
{
    mac.forget();
    notify_api();
}

void
Device::do_set_rate(enum ldl_mac_status *retval, uint8_t value)
{
    *retval = mac.set_rate(value);
    notify_api();
}

void
Device::do_get_rate(uint8_t *retval)
{
    *retval = mac.get_rate();
    notify_api();
}

void
Device::do_set_power(enum ldl_mac_status *retval, uint8_t value)
{
    *retval = mac.set_power(value);
    notify_api();
}

void
Device::do_get_power(uint8_t *retval)
{
    *retval = mac.get_power();
    notify_api();
}

void
Device::do_set_adr(bool value)
{
    mac.set_adr(value);
    notify_api();
}

void
Device::do_get_adr(bool *retval)
{
    *retval = mac.get_adr();
    notify_api();
}

void
Device::do_joined(bool *retval)
{
    *retval = mac.joined();
    notify_api();
}

void
Device::do_ready(bool *retval)
{
    *retval = mac.ready();
    notify_api();
}

void
Device::do_set_max_dcycle(uint8_t value)
{
    mac.set_max_dcycle(value);
    notify_api();
}

void
Device::do_get_max_dcycle(uint8_t *retval)
{
    *retval = mac.get_max_dcycle();
    notify_api();
}

void
Device::begin_api()
{
    mutex.lock();
    done = false;
}

void
Device::wait_until_api_done()
{
    do_work();

    while(!done){

        api.acquire();
    }

    mutex.unlock();
}

void
Device::notify_api()
{
    done = true;
    api.release();
}

/* public methods *****************************************************/

bool
Device::start(enum ldl_region region)
{
    bool retval = false;

    mutex.lock();

    if(mac.start(region)){

        worker_thread.start(callback(this,&Device::worker));
        retval = true;
    }

    mutex.unlock();

    return retval;
}

enum ldl_mac_status
Device::unconfirmed(uint8_t port, const void *data, uint8_t len, const struct ldl_mac_data_opts *opts)
{
    enum ldl_mac_status retval;
    int evt;

    begin_api();

    evt = event.call(callback(this, &Device::do_unconfirmed), &retval, port, data, len, opts);

    MBED_ASSERT(evt != 0);

    wait_until_api_done();

    return retval;
}

enum ldl_mac_status
Device::confirmed(uint8_t port, const void *data, uint8_t len, const struct ldl_mac_data_opts *opts)
{
    enum ldl_mac_status retval;
    int evt;

    begin_api();

    evt = event.call(callback(this, &Device::do_confirmed), &retval, port, data, len, opts);

    MBED_ASSERT(evt != 0);

    wait_until_api_done();

    return retval;
}

enum ldl_mac_status
Device::otaa()
{
    enum ldl_mac_status retval;
    int evt;

    begin_api();

    evt = event.call(callback(this, &Device::do_otaa), &retval);

    MBED_ASSERT(evt != 0);

    wait_until_api_done();

    return retval;
}

void
Device::forget()
{
    int evt;

    begin_api();

    evt = event.call(callback(this, &Device::do_forget));

    MBED_ASSERT(evt != 0);

    wait_until_api_done();
}

enum ldl_mac_status
Device::set_rate(uint8_t value)
{
    enum ldl_mac_status retval;
    int evt;

    begin_api();

    evt = event.call(callback(this, &Device::do_set_rate), &retval, value);

    MBED_ASSERT(evt != 0);

    wait_until_api_done();

    return retval;
}

uint8_t
Device::get_rate()
{
    uint8_t retval;
    int evt;

    begin_api();

    evt = event.call(callback(this, &Device::do_get_rate), &retval);

    MBED_ASSERT(evt != 0);

    wait_until_api_done();

    return retval;
}

enum ldl_mac_status
Device::set_power(uint8_t value)
{
    enum ldl_mac_status retval;
    int evt;

    begin_api();

    evt = event.call(callback(this, &Device::do_set_power), &retval, value);

    MBED_ASSERT(evt != 0);

    wait_until_api_done();

    return retval;
}

uint8_t
Device::get_power()
{
    uint8_t retval;
    int evt;

    begin_api();

    evt = event.call(callback(this, &Device::do_get_power), &retval);

    MBED_ASSERT(evt != 0);

    wait_until_api_done();

    return retval;
}

bool
Device::joined()
{
    bool retval;
    int evt;

    begin_api();

    evt = event.call(callback(this, &Device::do_joined), &retval);

    MBED_ASSERT(evt != 0);

    wait_until_api_done();

    return retval;
}

bool
Device::ready()
{
    bool retval;
    int evt;

    begin_api();

    evt = event.call(callback(this, &Device::do_ready), &retval);

    MBED_ASSERT(evt != 0);

    wait_until_api_done();

    return retval;
}

void
Device::set_adr(bool value)
{
    int evt;

    begin_api();

    evt = event.call(callback(this, &Device::do_set_adr), value);

    MBED_ASSERT(evt != 0);

    wait_until_api_done();
}

bool
Device::get_adr()
{
    bool retval;
    int evt;

    begin_api();

    evt = event.call(callback(this, &Device::do_get_adr), &retval);

    MBED_ASSERT(evt != 0);

    wait_until_api_done();

    return retval;
}

void
Device::set_max_dcycle(uint8_t value)
{
    int evt;

    begin_api();

    evt = event.call(callback(this, &Device::do_set_max_dcycle), value);

    MBED_ASSERT(evt != 0);

    wait_until_api_done();
}

uint8_t
Device::get_max_dcycle()
{
    uint8_t retval;
    int evt;

    begin_api();

    evt = event.call(callback(this, &Device::do_get_max_dcycle), &retval);

    MBED_ASSERT(evt != 0);

    wait_until_api_done();

    return retval;
}

#endif
