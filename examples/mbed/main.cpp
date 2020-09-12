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

SPI spi(D11, D12, D13);

LDL::SX1272 radio(
    spi,
    D10,    // nss
    A0,     // reset    
    D2, D3  // DIO0, DIO1
);

LDL::DefaultSM sm(app_key, nwk_key);
__attribute__ ((section (".noinit"))) LDL::DefaultStore store(dev_eui, join_eui);
LDL::Device device(store, sm, radio);

int main()
{
    mbed_trace_init();

    device.start(LDL_EU_863_870);

    for(;;){

        if(device.ready()){

            if(device.joined()){

                const char msg[] = "hello world";

                (void)device.unconfirmed(1, msg, strlen(msg));
            }
            else{
            
                (void)device.otaa();
            }
        }
    }
    
    return 0;
}
