LDL MBED Wrapper
================

This wrapper exists for quick evaluation and regression testing.

- LDL::Device is a thread-safe wrapper that runs LDL within its
  own thread. This needs >20K of RAM.

- LDL::MAC is a minimal wrapper around LDL. It can be made to work
  in RTOS and bare-metal profiles. This will work with less than 20K of RAM.

Usage:

~~~ c++

#include "mbed_ldl.h"

const uint8_t app_key[] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1};
const uint8_t nwk_key[] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1};
const uint8_t dev_eui[] = {0,0,0,0,0,0,0,1};
const uint8_t join_eui[] = {0,0,0,0,0,0,0,2};

SPI spi(D11, D12, D13);

LDL::SX1272 radio(
    spi,
    D10,    // nss
    A0,     // reset
    D2, D3  // DIO0, DIO1
);

LDL::DefaultSM sm(app_key, nwk_key);
LDL::DefaultStore store(dev_eui, join_eui);
LDL::Device device(store, sm, radio);

int main()
{
    device.start(LDL_EU_863_870);

    for(;;){

        if(device.ready()){

            if(device.joined()){

                const char msg[] = "hello world";

                device.unconfirmed(1, msg, strlen(msg));
            }
            else{

                device.otaa();
            }
        }
    }

    return 0;
}
~~~

## Gotchas

### DevNonce

DevNonce is implemented here as a counter that increments for every successful
OTAA.

LDL::DefaultStore stores this counter in volatile no-init memory. If you power
cycle the target, the server will likely stop accepting OTAA since it has already
seen the devNonce.

The workaround here is to implement persistence as a subclass of LDL::Store. Alternatively you can
reset the counter on the server side.




