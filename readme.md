LDL: A LoRaWAN Device Library
=============================

[![Build Status](https://travis-ci.org/cjhdev/lora_device_lib.svg?branch=master)](https://travis-ci.org/cjhdev/lora_device_lib)

LDL is an easy to use [LoRaWAN](https://en.wikipedia.org/wiki/LoRa#LoRaWAN) implementation for nodes/devices.

Below is an abridged example showing how to:

- initialise the library
- join a network
- send an empty data frame periodically

~~~ C
extern const void *app_key_ptr;
extern const void *nwk_key_ptr;

extern void your_app_handler(void *app, enum ldl_mac_response_type type, const union ldl_mac_response_arg *arg);

struct ldl_sm sm;
struct ldl_radio radio;
struct ldl_mac mac;

void main(void)
{
    LDL_SM_init(&sm, app_key_ptr, nwk_key_ptr);
    
    LDL_Radio_init(&radio, LDL_RADIO_SX1272, NULL);
    LDL_Radio_setPA(&radio, LDL_RADIO_PA_RFO);
    
    struct ldl_mac_init_arg arg = {0};
    
    arg.radio = &radio;
    arg.handler = your_app_handler;    
    arg.sm = &sm;
    
    LDL_MAC_init(&mac, LDL_EU_863_870, &arg);
    
    LDL_MAC_setMaxDCycle(&mac, 12U);
    
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

Behind the scenes you will need to implement the [radio connector](https://cjhdev.github.io/lora_device_lib_api/group__ldl__radio__connector.html), 
find somewhere to keep your root keys, and implement `your_app_handler()`. 
More information can be found in the [porting guide](porting.md).

It is important to keep in mind that LDL is still experimental. This means that things may not work properly and that
interfaces may change. Use one of the [tagged](https://github.com/cjhdev/lora_device_lib/releases) commits for best results.

## Features

- LoRaWAN 1.1
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
- Supported radios (run-time option)
    - SX1272
    - SX1276
- [Build options](https://cjhdev.github.io/lora_device_lib_api/group__ldl__build__options.html)
- [Interface documentation](https://cjhdev.github.io/lora_device_lib_api/)
- Examples
    - [Arduino wrapper](wrappers/arduino/output/arduino_ldl)
    - [documentation example](examples/doxygen/example.c)
    
## Limitations

- Class B and C not supported
- FSK modulation not supported
- ABP not supported
- Rejoin not supported
- **Experimental**

## Documentation

- [porting guide](porting.md)
- [interface documentation](https://cjhdev.github.io/lora_device_lib_api/)
- [history](history.md)
- [design goals](design_goals.md)
- [todo list](todo.md)

## Building Interface Documentation

- have doxygen and make installed
- `cd doxygen && make`
- open doxygen/html/index.html

Alternatively just read the [header files](include).

## Commercial Support

Commercial support is available from the author.

contact@stackmechanic.com

## See Also

- [LoRaMAC-Node](https://github.com/Lora-net/LoRaMac-node): Semtech reference implementation

## License

MIT
