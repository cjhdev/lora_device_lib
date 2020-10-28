#include "mbed_ldl.h"

#include "mbed_trace.h"

/* Change these values either here or in mbed_app.json
 *
 * - keys must be 16 bytes
 * - EUIs must be 8 bytes
 *
 * */
const uint8_t app_key[] = MBED_CONF_APP_APP_KEY;
const uint8_t nwk_key[] = MBED_CONF_APP_NWK_KEY;
const uint8_t dev_eui[] = MBED_CONF_APP_DEV_EUI;
const uint8_t join_eui[] = MBED_CONF_APP_JOIN_EUI;

LDL::DefaultSM sm(app_key, nwk_key);

LDL::DefaultStore store(dev_eui, join_eui);

int main()
{
    mbed_trace_init();

    /* PA_12 is where the reference design puts the TCXO on/off
     * control line, and also where you will find it on the LRWAN
     * kit.
     *
     * */
    static LDL::CMWX1ZZABZ radio(
        PA_12       // enable_tcxo
    );

    static LDL::MAC mac(store, sm, radio);

    mac.start(LDL_EU_863_870);

    for(;;){

        if(mac.ready()){

            if(mac.joined()){

                const char msg[] = "hello world";

                (void)mac.unconfirmed(1, msg, strlen(msg));
            }
            else{

                (void)mac.otaa();
            }
        }

        mac.process();
    }

    return 0;
}
