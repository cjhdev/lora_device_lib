LDL
===

LDL is a LoRaWAN implementation for devices.

Be sure to use tagged releases.

[![Build Status](https://travis-ci.org/cjhdev/lora_device_lib.svg?branch=master)](https://travis-ci.org/cjhdev/lora_device_lib)

## Features

- Compact and portable
- LoRaWAN 1.1
- Class A
- OTAA
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
- Rejoin not supported yet
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
- open doxygen/output/index.html

Alternatively just read the [header files](include).

## Commercial Support

Commercial support is available from the author.

contact@stackmechanic.com

## See Also

- [LoRaMAC-Node](https://github.com/Lora-net/LoRaMac-node): Semtech reference implementation

## License

MIT
