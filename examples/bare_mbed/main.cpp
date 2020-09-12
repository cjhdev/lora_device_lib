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

/* pin mapping for the DISCO_L072CZ_LRWAN1 */
SPI spi(PA_7, PA_6, PB_3);

LDL::CMWX1ZZABZ radio(
    spi,   
    PA_15,      // nss
    PC_0,       // reset    
    PB_4, PB_1, // DIO0, DIO1    
    PC_1,       // enable_boost
    PC_2,       // enable_rfo
    PA_1,       // enable_rfi    
    PA_12       // enable_tcxo
);

EventQueue event;

LDL::DefaultSM sm(app_key, nwk_key);

__attribute__ ((section (".noinit"))) LDL::DefaultStore store(dev_eui, join_eui);

LDL::MAC mac(event, store, sm, radio);

int main()
{
    mbed_trace_init();

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
