LoraDeviceLib
=============

An experimental LoRaWAN 1.0.3 device implementation.

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

- add `include` folder to include search path
- build all sources in `src`
- if your toolchain doesn't support weak symbols you will need to omit `src/lora_system.c`

## LoRaWAN Conformance

This implementation has never been conformance tested since the test
specification is not open.

It should be more or less aligned to version 1.0.3 of the specification.

## License

MIT
