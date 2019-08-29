LoraDeviceLib
=============

An experimental LoRaWAN device implementation.

[![Build Status](https://travis-ci.org/cjhdev/lora_device_lib.svg?branch=master)](https://travis-ci.org/cjhdev/lora_device_lib)

## Highlights

- Class A only
- LoRa mode only (no GFSK)
- OTAA only (ABP not supported)
- ADR and other network configurable features
- Multiple region support (run-time option)
- Multiple radio support (run-time option)
- Extensive build-time options for optimisation and/or conformance to your project requirements
- [Arduino wrapper](bindings/arduino/output/arduino_ldl)
- [API documentation](https://cjhdev.github.io/lora_device_lib_api/)

## Building

- review [options](https://cjhdev.github.io/lora_device_lib_api/group__ldl__optional.html)
- review [system interfaces](https://cjhdev.github.io/lora_device_lib_api/group__ldl__system.html)
- add `include` folder to include search path
- build all sources in `src`

## See Also

- [LoRaMAC-Node](https://github.com/Lora-net/LoRaMac-node): Semtech reference implementation

## License

MIT
