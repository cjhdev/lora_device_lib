#include "mbed_ldl.h"

#include "mbed_trace.h"

const uint8_t app_key[] = MBED_CONF_APP_APP_KEY;
const uint8_t nwk_key[] = MBED_CONF_APP_NWK_KEY;
const uint8_t dev_eui[] = MBED_CONF_APP_DEV_EUI;
const uint8_t join_eui[] = MBED_CONF_APP_JOIN_EUI;

LDL::DefaultSM sm(app_key, nwk_key);
LDL::DefaultStore store(dev_eui, join_eui);

void handle_rx(uint8_t port, const void *data, uint8_t size)
{
}

void handle_link_status(uint8_t margin, uint8_t gwcount)
{
}

void handle_device_time(uint32_t seconds, uint8_t fractions)
{
}

int main()
{
    uint32_t entropy;

    mbed_trace_init();

    static LDL::HW::NucleoWL55JCX radio;

    static LDL::Device device(store, sm, radio);

    device.set_rx_cb(callback(handle_rx));
    device.set_link_status_cb(callback(handle_link_status));
    device.set_device_time_cb(callback(handle_device_time));

    device.start(LDL_EU_863_870);

    if(device.entropy(entropy) == LDL_STATUS_OK){

        srand(entropy);
    }

    device.otaa();

    for(;;){

        const char msg[] = "hello world";

        // will block until channel available
        device.unconfirmed(1, msg, strlen(msg));

        ThisThread::sleep_for(10s);
    }

    return 0;
}
