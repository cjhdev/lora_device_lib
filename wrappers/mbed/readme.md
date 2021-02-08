LDL MBED Wrapper
================

This wrapper exists for quick evaluation and regression testing.

- LDL::Device is a thread-safe wrapper that runs LDL within its
  own thread. This works on targets with >20K of RAM.

- LDL::MAC is a minimal wrapper around LDL. It can be made to work
  in RTOS and bare-metal profiles. This works on targets with <20K of RAM.

- LDL::HW::* are a collection of pre-configured radios/modules/shields. If you
  are running on your own hardware you can either define your pin-mapping to line
  up with one of these variants or create your own class.

## Example

This example requires a development kit with >20K RAM connected to a SX126XMB2XAS shield.

The app:

- activates over the air in LDL_EU_863_870 region
- sends hello world as often as possible
- ignores any downlink frames

~~~ c++
#include "mbed_ldl.h"

const uint8_t app_key[] = MBED_CONF_APP_APP_KEY;
const uint8_t nwk_key[] = MBED_CONF_APP_NWK_KEY;
const uint8_t dev_eui[] = MBED_CONF_APP_DEV_EUI;
const uint8_t join_eui[] = MBED_CONF_APP_JOIN_EUI;

LDL::DefaultSM sm(app_key, nwk_key);
LDL::DefaultStore store(dev_eui, join_eui);
LDL::HW::SX126XMB2XAS radio();
LDL::Device device(store, sm, radio);

int main()
{
    device.start(LDL_EU_863_870);

    device.otaa();

    for(;;){

        const char msg[] = "hello world";

        device.unconfirmed(1, msg, strlen(msg));
    }

    return 0;
}
~~~

This example and more are in [examples/mbed](../../examples/mbed).

## FYI

### ADR

The Adaptive Data Rate feature is enabled by default in LDL.
You should disable this when your device is not stationary.

### SF12

If you want your app to use SF12 based data rates you need to override
the `ldl.disable-sf12` compile time setting:

~~~
"target_overrides": {
    "*" : {
        "ldl.disable-sf12" : null
    }
}
~~~

`ldl.disable-sf12` is enabled by default because some radio hardware
cannot work reliably at SF12.

### DevNonce

If `ldl.l2-version` > `LDL_L2_VERSION_1_0_3` DevNonce is a counter that
increments on each successful OTAA.

LDL::DefaultStore stores this counter in volatile memory. If you power
cycle the target, the server will likely stop accepting OTAA since it has already
seen the devNonce.

The workaround here is to implement persistence as a subclass of LDL::Store. Alternatively you can
reset the counter on the server side.

### LowPowerTimer and LowPowerTimeout

The wrapper depends on LowPowerTimer and LowPowerTimer.
There is an expectation that these are clocked by a crystal or TCXO.

### Logging

You can turn on/off logging by changing mbed_app.json:

disable logging:

~~~
{
    "target_overrides": {
        "*": {
            "mbed-trace.enable": null,
        }
    }
}
~~~

enable info level:

~~~
{
    "target_overrides": {
        "*": {
            "platform.stdio-baud-rate": 115200,
            "platform.default-serial-baud-rate": 115200,
            "mbed-trace.enable": 1,
            "mbed-trace.max-level" : "TRACE_LEVEL_INFO"
        }
    }
}
~~~

enable debug level:

~~~
{
    "target_overrides": {
        "*": {
            "platform.stdio-baud-rate": 115200,
            "platform.default-serial-baud-rate": 115200,
            "mbed-trace.enable": 1,
            "mbed-trace.max-level" : "TRACE_LEVEL_DEBUG"
        }
    }
}
~~~

enable trace level:

~~~
{
    "target_overrides": {
        "*": {
            "platform.stdio-baud-rate": 115200,
            "platform.default-serial-baud-rate": 115200,
            "mbed-trace.enable": 1,
            "mbed-trace.max-level" : "TRACE_LEVEL_DEBUG",
            "ldl.enable-verbose-debug" : 1
        }
    }
}
~~~

enable trace level with extra messages from radio driver:

~~~
{
    "target_overrides": {
        "*": {
            "platform.stdio-baud-rate": 115200,
            "platform.default-serial-baud-rate": 115200,
            "mbed-trace.enable": 1,
            "mbed-trace.max-level" : "TRACE_LEVEL_DEBUG",
            "ldl.enable-verbose-debug" : 1,
            "ldl.enable-radio-debug" : 1
        }
    }
}
~~~

Note that verbose logging has the potential to upset timing. If your
app can't receive downlinks, try turning off logging.

