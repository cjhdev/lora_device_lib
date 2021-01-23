#include "mbed_ldl.h"

const uint8_t app_key[] = MBED_CONF_APP_APP_KEY;
const uint8_t nwk_key[] = MBED_CONF_APP_NWK_KEY;
const uint8_t dev_eui[] = MBED_CONF_APP_DEV_EUI;
const uint8_t join_eui[] = MBED_CONF_APP_JOIN_EUI;

static Semaphore wakeup_flag(1);

void handle_mac_event(enum ldl_mac_response_type type, const union ldl_mac_response_arg *arg)
{
    switch(type){
    case LDL_MAC_ENTROPY:
        srand(arg->entropy.value);
        break;
    case LDL_MAC_RX:
        break;
    default:
        break;
    }
}

void wakeup_handler()
{
    wakeup_flag.release();
}

int main()
{
    mbed_trace_init();

    static LowPowerTicker ticker;
    static LowPowerTimeout timeout;
    static DeepSleepLock sleep_lock;

    static LDL::DefaultSM sm(app_key, nwk_key);
    static LDL::DefaultStore store(dev_eui, join_eui);
    static LDL::HW::CMWX1ZZABZ radio;
    static LDL::MAC mac(store, sm, radio);

    mac.set_event_cb(callback(handle_mac_event));

    /* wake every 10 seconds */
    ticker.attach(callback(wakeup_handler), std::chrono::microseconds(10000000));

    /* wake when MAC has an ISR */
    mac.set_event_cb(callback(handle_mac_event));

    mac.start(LDL_EU_863_870);

    /* seed random from radio */
    mac.entropy();

    /* slow the rate device can send messages */
    mac.set_max_dcycle(6);

    for(;;){

        if(mac.ready()){

            if(mac.joined()){

                const char msg[] = "hello world";

                mac.unconfirmed(1, msg, strlen(msg));
            }
            else{

                mac.otaa();
            }
        }

        mac.process();

        uint32_t next_event = mac.ticks_until_next_event();

        if(next_event > 0){

            timeout.attach(callback(wakeup_handler), std::chrono::microseconds(next_event));

            /* Deep sleep can be slow to wake from.
             *
             * Lock it out if there is an event that will
             * happen soon.
             *
             * */
            if(next_event < 100000){

                sleep_lock.lock();
            }

            wakeup_flag.acquire();

            if(next_event < 100000){

                sleep_lock.unlock();
            }
        }
    }

    return 0;
}
