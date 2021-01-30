#include "mbed_ldl.h"

#include "mbed_trace.h"

const uint8_t app_key[] = MBED_CONF_APP_APP_KEY;
const uint8_t nwk_key[] = MBED_CONF_APP_NWK_KEY;
const uint8_t dev_eui[] = MBED_CONF_APP_DEV_EUI;
const uint8_t join_eui[] = MBED_CONF_APP_JOIN_EUI;

int main()
{
    uint32_t entropy;
    const char msg[] = "hello world";

    mbed_trace_init();

    static LDL::DefaultSM sm(app_key, nwk_key);
    static LDL::DefaultStore store(dev_eui, join_eui);
    static LDL::HW::NucleoWL55JC radio;

    static LDL::Device device(store, sm, radio);

    device.start(LDL_EU_863_870);

    if(device.entropy(entropy) == LDL_STATUS_OK){

        srand(entropy);
    }

    device.otaa();

    for(;;){

        device.unconfirmed(1, msg, strlen(msg));

        ThisThread::sleep_for(10s);
    }

    return 0;
}
