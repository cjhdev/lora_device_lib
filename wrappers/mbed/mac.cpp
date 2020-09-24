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

MAC::MAC(EventQueue& event, Store &store, SM &sm, Radio &radio) :
    signal(A0, 0),
    radio(radio),
    sm(sm),
    store(store),
    event(event),
    semaphore(1)
{
    next_event_handler = 0;
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
    return to_obj(app)->event.tick();
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

    static const char *bw[] = {
        "125",
        "250",
        "500"
    };

    uint32_t timestamp = MAC::_ticks(app);

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

    switch(type){
    case LDL_MAC_STARTUP:
        tr_info("[%" PRIu32 "]STARTUP: ENTROPY=%u",
            timestamp,
            arg->startup.entropy
        );
        break;
    case LDL_MAC_LINK_STATUS:
        tr_info("[%" PRIu32 "]LINK_STATUS: M=%u GW=%u",
            timestamp,
            arg->link_status.margin,
            arg->link_status.gwCount
        );
        break;
    case LDL_MAC_CHIP_ERROR:
        tr_info("[%" PRIu32 "]CHIP_ERROR", timestamp);
        break;
    case LDL_MAC_RESET:
        tr_info("[%" PRIu32 "]RESET", timestamp);
        break;
    case LDL_MAC_TX_BEGIN:
        tr_info("[%" PRIu32 "]TX_BEGIN: SZ=%u F=%" PRIu32 " SF=%u BW=%s P=%u",
            timestamp,
            arg->tx_begin.size,
            arg->tx_begin.freq,
            arg->tx_begin.sf,
            bw[arg->tx_begin.bw],
            arg->tx_begin.power
        );
        break;
    case LDL_MAC_TX_COMPLETE:
        self->signal = 1;
        tr_info("[%" PRIu32 "]TX_COMPLETE", timestamp);
        break;
    case LDL_MAC_RX1_SLOT:
    case LDL_MAC_RX2_SLOT:
        self->signal = 0;
        tr_info("[%" PRIu32 "]%s: F=%" PRIu32 " SF=%u BW=%s E=%" PRIu32 " M=%" PRIu32 " SYM=%" PRIu32,
            timestamp,
            (type == LDL_MAC_RX1_SLOT) ? "RX1_SLOT" : "RX2_SLOT",
            arg->rx_slot.freq,
            arg->rx_slot.sf,
            bw[arg->rx_slot.bw],
            arg->rx_slot.error,
            arg->rx_slot.margin,
            arg->rx_slot.timeout
        );
        break;
    case LDL_MAC_DOWNSTREAM:
        tr_info("[%" PRIu32 "]DOWNSTREAM: SZ=%u RSSI=%" PRIu16 " SNR=%" PRIu16,
            timestamp,
            arg->downstream.size,
            arg->downstream.rssi,
            arg->downstream.snr
        );
        break;
    case LDL_MAC_JOIN_COMPLETE:
        tr_info("[%" PRIu32 "]JOIN_COMPLETE: JN=%" PRIu32 " NDN=%" PRIu16 " NETID=%" PRIu32 " DEVADDR=%" PRIu32,
            timestamp,
            arg->join_complete.joinNonce,
            arg->join_complete.nextDevNonce,
            arg->join_complete.netID,
            arg->join_complete.devAddr
        );
        break;
    case LDL_MAC_JOIN_TIMEOUT:
        tr_info("[%" PRIu32 "]JOIN_TIMEOUT", timestamp);
        break;
    case LDL_MAC_RX:
        tr_info("[%" PRIu32 "]RX: PORT=%u COUNT=%" PRIu16 " SIZE=%u",
            timestamp,
            arg->rx.port,
            arg->rx.counter,
            arg->rx.size
        );
        break;
    case LDL_MAC_DATA_COMPLETE:
        tr_info("[%" PRIu32 "]DATA_COMPLETE", timestamp);
        break;
    case LDL_MAC_DATA_TIMEOUT:
        tr_info("[%" PRIu32 "]DATA_TIMEOUT", timestamp);
        break;
    case LDL_MAC_DATA_NAK:
        tr_info("[%" PRIu32 "]DATA_NAK", timestamp);
        break;
    case LDL_MAC_SESSION_UPDATED:
        tr_info("[%" PRIu32 "]SESSION_UPDATED", timestamp);
        break;
    case LDL_MAC_DEVICE_TIME:
        tr_info("[%" PRIu32 "]DEVICE_TIME_ANS: SEC=%" PRIu32 " FRAC=%u",
            timestamp,
            arg->device_time.seconds,
            arg->device_time.fractions
        );
        break;
    default:
        break;
    }

    if(self->event_cb){

        self->event_cb(type, arg);
    }
}

/* protected **********************************************************/

void
MAC::do_handle_radio_event(enum ldl_radio_event event, uint32_t ticks)
{
    LDL_MAC_radioEventWithTicks(&mac, event, ticks);
    semaphore.release();
}

void
MAC::handle_radio_event(enum ldl_radio_event event)
{
    if(semaphore.try_acquire()){

        /* we don't assert on the return value
         * since not having memory here will show up later as
         * "chip error" when the interrupt never arrives.
         *
         * */
        this->event.call(callback(this, &MAC::do_handle_radio_event), event, this->event.tick());
    }
}

/* public methods *****************************************************/

void
MAC::do_process()
{
    uint32_t next;

    do{

        LDL_MAC_process(&mac);
        next = LDL_MAC_ticksUntilNextEvent(&mac);
    }
    while(next == 0U);

    next = (next == UINT32_MAX) ? 60000UL : next;

    event.cancel(next_event_handler);

    next_event_handler = event.call_in(std::chrono::milliseconds(next), callback(this, &MAC::do_process));

    /* LoRaWAN will grind to a halt if there is no memory to enqueue */
    MBED_ASSERT(next_event_handler != 0);
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

    // clock source is the millisecond ticker in EventQueue
    arg.ticks = _ticks;
    arg.tps = 1000UL;
    arg.a = 0UL;
    arg.b = 4UL;
    arg.advance = 0UL;

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

    radio.set_event_handler(callback(this, &MAC::handle_radio_event));

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

    run_state = ON;

    do_process();

    return true;
}

void
MAC::stop()
{
    run_state = OFF;
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
    event.dispatch(0);
}
