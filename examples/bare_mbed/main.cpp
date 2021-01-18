#include "mbed_ldl.h"

#include "mbed_trace.h"

const uint8_t app_key[] = MBED_CONF_APP_APP_KEY;
const uint8_t nwk_key[] = MBED_CONF_APP_NWK_KEY;
const uint8_t dev_eui[] = MBED_CONF_APP_DEV_EUI;
const uint8_t join_eui[] = MBED_CONF_APP_JOIN_EUI;

LDL::DefaultSM sm(app_key, nwk_key);
LDL::DefaultStore store(dev_eui, join_eui);

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

int main()
{
    mbed_trace_init();

    // init after trace so we get logging
    static LDL::HW::CMWX1ZZABZ radio;
    static LDL::MAC mac(store, sm, radio);

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
    }

    return 0;
}
