LDL: A LoRaWAN Device Library
=============================

[![Build Status](https://travis-ci.org/cjhdev/lora_device_lib.svg?branch=master)](https://travis-ci.org/cjhdev/lora_device_lib)

LDL is a [LoRaWAN](https://en.wikipedia.org/wiki/LoRa#LoRaWAN) implementation for devices.

Below is a partially complete example showing how to:

- initialise the library (for L2 specification 1.0.4)
- activate over the air
- send an empty data frame periodically
- limit the rate of send via global duty cycle setting
- allow the server to adjust the data rate and power setting

~~~ C
#include "ldl_mac.h"
#include "ldl_radio.h"

extern const void *app_key_ptr;

extern const void *dev_eui_ptr;
extern const void *join_eui_ptr;

extern void chip_set_mode(void *self, enum ldl_chip_mode mode);
extern bool chip_read(void *self, const void *opcode, size_t opcode_size, void *data, size_t size);
extern bool chip_write(void *self, const void *opcode, size_t opcode_size, const void *data, size_t size);

extern void your_app_handler(void *app, enum ldl_mac_response_type type, const union ldl_mac_response_arg *arg);
extern uint32_t your_ticks(void *app);
extern uint32_t your_rand(void *app);

struct ldl_sm sm;
struct ldl_radio radio;
struct ldl_mac mac;

void main(void)
{
    /* init the security module */
    LDL_SM_init(&sm, app_key_ptr);

    /* init the radio */
    {
        struct ldl_sx127x_init_arg arg = {0};

        arg.chip_set_mode = chip_set_mode;
        arg.chip_write = chip_write;
        arg.chip_read = chip_read;

        LDL_SX1272_init(&radio, &arg);
    }

    /* init the mac */
    {
        struct ldl_mac_init_arg arg = {0};

        arg.ticks = your_ticks;
        arg.tps = 32768;
        arg.a = 20;
        arg.rand = your_rand;

        arg.radio = &radio;
        arg.radio_interface = LDL_Radio_getInterface(&radio);

        arg.sm = &sm;
        arg.sm_interface = LDL_SM_getInterface();

        arg.handler = your_app_handler;

        arg.joinEUI = join_eui_ptr;
        arg.devEUI = dev_eui_ptr;

        LDL_MAC_init(&mac, LDL_EU_863_870, &arg);

        LDL_Radio_setEventCallback(&radio, &mac, LDL_MAC_radioEvent);

        LDL_MAC_setMaxDCycle(&mac, 12);

        LDL_MAC_entropy(&mac);
    }

    __enable_irq();

    for(;;){

        if(LDL_MAC_ready(&mac)){

            if(LDL_MAC_joined(&mac)){

                LDL_MAC_unconfirmedData(&mac, 1, NULL, 0, NULL);
            }
            else{

                LDL_MAC_otaa(&mac);
            }
        }

        LDL_MAC_process(&mac);
    }
}
~~~

Everything marked extern needs to be implemented somewhere. There
are more details in the [porting guide](porting.md).

The fastest way to get up and running is to use the [MBED wrapper](wrappers/mbed) and
copy one of the examples.

It is important to keep in mind that LDL is experimental. This means that things may not work properly and that
interfaces may change. Use one of the [tagged](https://github.com/cjhdev/lora_device_lib/releases) commits for best results
and read [history.md](history.md) if updating from an older version.

## Compiling

- define preprocessor symbols as needed ([see build options](https://ldl.readthedocs.io/en/latest/group__ldl__build__options.html))
- add `include` folder to include search path
- build all sources in `src`

## Features

- Small memory footprint
- L2 Support
    - 1.0.3
    - 1.0.4
    - 1.1
- Class A
- OTAA
- ADR
- Region Support (RP002-1.0.1)
    - EU_868_870
    - EU_433
    - US_902_928
    - AU_915_928
- Radio Drivers
    - SX1272
    - SX1276
    - SX1261
    - SX1262
- Linted to MISRA 2012
- Examples
    - [readme example](examples/doxygen/example.c)
    - [rtos mbed example](examples/mbed/rtos)
    - [bare-metal mbed example](examples/mbed/bare_mbed)
    - [chip interface example](examples/chip_interface)
    - [ruby example](examples/ruby)
    - [avr-gcc example](examples/avr)

## Limitations

- Class B and C not supported
- FSK modulation not supported
- ABP not supported
- 1.1 Rejoin not supported
- **Experimental**

## Documentation

- [porting guide](porting.md)
- [interface documentation](https://ldl.readthedocs.io/en/latest/)
- [history](history.md)
- [design goals](design_goals.md)
- [todo list](todo.md)
- [contribution guidelines](contributing.md)

## See Also

- [LoRaMAC-Node](https://github.com/Lora-net/LoRaMac-node) (reference implementation)
- [LDL vs. LoRaMac-Node: size and complexity](https://cjh.id.au/2019/12/11/comparing-ldl-to-loramac.html)

## License

MIT
