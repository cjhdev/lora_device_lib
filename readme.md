LDL: A LoRaWAN Device Library
=============================

[![Build Status](https://travis-ci.org/cjhdev/lora_device_lib.svg?branch=master)](https://travis-ci.org/cjhdev/lora_device_lib)

LDL is an easy to use [LoRaWAN](https://en.wikipedia.org/wiki/LoRa#LoRaWAN) implementation for nodes/devices.

Below is an abridged example showing how to:

- initialise the library
- join a network
- send an empty data frame periodically

~~~ C
#include "ldl_mac.h"
#include "ldl_radio.h"

extern const void *app_key_ptr;
extern const void *nwk_key_ptr;

extern const void *dev_eui_ptr;
extern const void *join_eui_ptr;

extern void chip_set_mode(void *self, enum ldl_chip_mode mode);
extern void chip_read(void *self, uint8_t addr, void *data, uint8_t size);
extern void chip_write(void *self, uint8_t addr, const void *data, uint8_t size);

extern void your_app_handler(void *app, enum ldl_mac_response_type type, const union ldl_mac_response_arg *arg);
extern uint32_t your_ticks(void *app);
extern uint32_t your_rand(void *app);

struct ldl_sm sm;
struct ldl_radio radio;
struct ldl_mac mac;

void main(void)
{
    LDL_SM_init(&sm, app_key_ptr, nwk_key_ptr);

    /* init the radio */
    {
        struct ldl_radio_init_arg arg = {0};

        arg.type = LDL_RADIO_SX1272;

        arg.chip_set_mode = chip_set_mode;
        arg.chip_write = chip_write;
        arg.chip_read = chip_read;

        LDL_Radio_init(&radio, &arg);
    }

    /* init the mac */
    {
        struct ldl_mac_init_arg arg = {0};

        arg.ticks = your_ticks;
        arg.tps = 32768UL;
        arg.a = 20UL;
        arg.rand = your_rand;

        arg.radio = &radio;
        arg.handler = your_app_handler;
        arg.sm = &sm;

        arg.joinEUI = join_eui_ptr;
        arg.devEUI = dev_eui_ptr;

        LDL_MAC_init(&mac, LDL_EU_863_870, &arg);

        LDL_Radio_setEventCallback(&radio, &mac, LDL_MAC_radioEvent);

        LDL_MAC_setMaxDCycle(&mac, 12U);
    }

    __enable_irq();

    for(;;){

        if(LDL_MAC_ready(&mac)){

            if(LDL_MAC_joined(&mac)){

                LDL_MAC_unconfirmedData(&mac, 1U, NULL, 0U, NULL);
            }
            else{

                LDL_MAC_otaa(&mac);
            }
        }

        LDL_MAC_process(&mac);
    }
}
~~~

Behind the scenes you will need to implement:

- [chip interface](https://cjhdev.github.io/lora_device_lib_api/group__ldl__chip__interface.html): `chip_set_mode()`, `chip_write()`, `chip_read()`
- keys: `app_key_ptr`, `nwk_key_ptr`, `dev_eui_ptr`, `join_eui_ptr`
- system interfaces: `your_ticks()`, `your_rand()`
- `your_app_handler()`

The fastest way to get started with LDL is to use the [MBED wrapper](wrappers/mbed).
This project repository can be imported directly into an MBED project and
the MBED tooling will find the wrapper.

It is important to keep in mind that LDL is still experimental. This means that things may not work properly and that
interfaces may change. Use one of the [tagged](https://github.com/cjhdev/lora_device_lib/releases) commits for best results
and read [history.md](history.md) if updating from an older version.

## Features

- Small memory footprint*
- LoRaWAN 1.1 subset
- Class A
- OTAA
    - frequency and mask CFLists
- Confirmed and Unconfirmed Data
    - per invocation options (overriding global settings)
        - redundancy (nbTrans)
        - piggy-back LinkCheckReq
        - piggy-back DeviceTimeReq
        - transmit start time dither
    - deferred duty cycle limit for redundant unconfirmed data
- ADR
- Supported MAC commands
    - LinkCheckReq/Ans
    - LinkADRReq/Ans
    - DutyCycleReq/Ans
    - RXParamSetupReq/Ans
    - DevStatusReq/Ans
    - NewChannelReq/Ans
    - RXTimingSetupReq/Ans
    - RXParamSetupReq/Ans
    - DlChannelReq/Ans
    - DeviceTimeReq/Ans
    - ADRParamSetupReq/Ans
    - RekeyInd/Conf
- Supported regions (run-time option)
    - EU_868_870
    - EU_433
    - US_902_928
    - AU_915_928
- Non-blocking radio driver with run-time configuration
    - SX1272
    - SX1276
- Linted to MISRA 2012
- [Interface documentation](https://cjhdev.github.io/lora_device_lib_api/)
- [Build options](https://cjhdev.github.io/lora_device_lib_api/group__ldl__build__options.html)
- Examples
    - [rtos mbed example](examples/mbed)
    - [bare-metal mbed example](examples/bare_mbed)
    - [documentation example](examples/doxygen/example.c)
    - [chip interface example](examples/chip_interface)

*compared to LoRaMAC-Node.

## Limitations

- Class B and C not supported
- FSK modulation not supported
- ABP not supported
- 1.1 Rejoin not supported
- **Experimental**

## Documentation

- [porting guide](porting.md)
- [interface documentation](https://cjhdev.github.io/lora_device_lib_api/)
- [history](history.md)
- [design goals](design_goals.md)
- [todo list](todo.md)

## See Also

- [LoRaMAC-Node](https://github.com/Lora-net/LoRaMac-node) (Semtech reference implementation)
- [LDL vs. LoRaMac-Node: size and complexity](https://cjh.id.au/2019/12/11/comparing-ldl-to-loramac.html)

## License

MIT
