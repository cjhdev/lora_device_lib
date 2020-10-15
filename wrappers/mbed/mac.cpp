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

#include "mac.h"
#include "mbed_trace.h"

#include <inttypes.h>

using namespace LDL;

/* constructors *******************************************************/

MAC::MAC(Store &store, SM &sm, Radio &radio) :
    signal(A0, 0),
    radio(radio),
    sm(sm),
    store(store)
{
    timer.start();
}

/* protected static  **************************************************/

MAC *
MAC::to_obj(void *self)
{
    return static_cast<MAC *>(self);
}

uint32_t
MAC::_ticks(void *app)
{
    return (uint32_t)to_obj(app)->timer.elapsed_time().count();
}

uint32_t
MAC::_rand(void *app)
{
    return rand();
}

uint8_t
MAC::_get_battery_level(void *app)
{
    return 255U;
}

void
MAC::app_handler(void *app, enum ldl_mac_response_type type, const union ldl_mac_response_arg *arg)
{
    MAC *self = to_obj(app);

    switch(type){
    case LDL_MAC_STARTUP:
        if(self->entropy_cb){

            self->entropy_cb(arg->startup.entropy);
        }
        else{

            srand(arg->startup.entropy);
        }
        break;
    case LDL_MAC_JOIN_COMPLETE:
        self->store.save_join_accept(arg->join_complete.joinNonce, arg->join_complete.nextDevNonce);
        break;
    case LDL_MAC_SESSION_UPDATED:
        self->store.save_session(arg->session_updated.session, sizeof(*arg->session_updated.session));
        break;
    case LDL_MAC_RX:
        self->data_cb(arg->rx.port, arg->rx.data, arg->rx.size);
        break;

    default:
        break;
    }

    if(self->event_cb){

        self->event_cb(type, arg);
    }
}

/* public methods *****************************************************/

void
MAC::handle_radio_event(enum ldl_radio_event event)
{
    LDL_MAC_radioEvent(&mac, event);
}

bool
MAC::start(enum ldl_region region)
{
    if(run_state == ON){

        return false;
    }

    Store::init_params store_params;
    struct ldl_mac_init_arg arg = {};

    // this will grow the stack!
    struct ldl_mac_session session;
    size_t session_size;

    store.get_init_params(&store_params);
    session_size = store.get_session(&session, sizeof(session));

    arg.ticks = _ticks;

    arg.rand = _rand;
    arg.get_battery_level = _get_battery_level;

    arg.app = this;
    arg.handler = app_handler;

    arg.radio = (struct ldl_radio *)(&radio);
    arg.radio_interface = &radio.interface;

    arg.sm = (struct ldl_sm *)(&sm);
    arg.sm_interface = &sm.interface;

    arg.devEUI = store_params.dev_eui;
    arg.joinEUI = store_params.join_eui;
    arg.joinNonce = store_params.join_nonce;
    arg.devNonce = store_params.dev_nonce;

    arg.session = (session_size == sizeof(session)) ? &session : NULL;

    LDL_MAC_init(&mac, region, &arg);

    /* apply TTN fair access policy
     *
     * ~30s per day
     *
     * 30 / (60*60*24)  = 0.000347222
     *
     * 1 / (2 ^ 11)     = 0.000488281
     * 1 / (2 ^ 12)     = 0.000244141
     *
     * */
    LDL_MAC_setMaxDCycle(&mac, 12U);

    radio.set_event_handler(callback(this, &MAC::handle_radio_event));

    run_state = ON;

    return true;
}

enum ldl_mac_status
MAC::unconfirmed(uint8_t port, const void *data, uint8_t len, const struct ldl_mac_data_opts *opts)
{
    return LDL_MAC_unconfirmedData(&mac, port, data, len, opts);
}

enum ldl_mac_status
MAC::confirmed(uint8_t port, const void *data, uint8_t len, const struct ldl_mac_data_opts *opts)
{
    return LDL_MAC_confirmedData(&mac, port, data, len, opts);
}

enum ldl_mac_status
MAC::otaa()
{
    return LDL_MAC_otaa(&mac);
}

void
MAC::forget()
{
    LDL_MAC_forget(&mac);
}

enum ldl_mac_status
MAC::set_rate(uint8_t value)
{
    return LDL_MAC_setRate(&mac, value);
}

uint8_t
MAC::get_rate()
{
    return LDL_MAC_getRate(&mac);
}

enum ldl_mac_status
MAC::set_power(uint8_t value)
{
    return LDL_MAC_setPower(&mac, value);
}

uint8_t
MAC::get_power()
{
    return LDL_MAC_getPower(&mac);
}

bool
MAC::joined()
{
    return LDL_MAC_joined(&mac);
}

bool
MAC::ready()
{
    return LDL_MAC_ready(&mac);
}

void
MAC::set_adr(bool value)
{
    LDL_MAC_setADR(&mac, value);
}

bool
MAC::get_adr()
{
    return LDL_MAC_getADR(&mac);
}

void
MAC::set_max_dcycle(uint8_t value)
{
    LDL_MAC_setMaxDCycle(&mac, value);
}

uint8_t
MAC::get_max_dcycle()
{
    return LDL_MAC_getMaxDCycle(&mac);
}

uint32_t
MAC::ticks_until_next_event()
{
    return LDL_MAC_ticksUntilNextEvent(&mac);
}

void
MAC::process()
{
    LDL_MAC_process(&mac);
}
