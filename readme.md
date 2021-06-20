LDL: A LoRaWAN Device Library
=============================

[![Build Status](https://travis-ci.org/cjhdev/lora_device_lib.svg?branch=master)](https://travis-ci.org/cjhdev/lora_device_lib)

LDL is a [LoRaWAN](https://en.wikipedia.org/wiki/LoRa#LoRaWAN) implementation for devices.

Use one of the [releases](https://github.com/cjhdev/lora_device_lib/releases)
for best results and read [history.md](history.md) if updating from an old version.

## Examples

- [MBED rtos profile](examples/mbed/rtos)
- [MBED bare-metal profile](examples/mbed/bare_metal)
- [generic mainloop](examples/doxygen/example.c)
- [generic chip interface](examples/chip_interface)
- [ruby virtual hardware](examples/ruby)

## Features

- Small memory footprint
- L2 Support
    - 1.0.3 (recommended)
    - 1.0.4 (recommended)
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
    - STM32WL55
- Linted to MISRA 2012

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

- [LoRaMAC-Node](https://github.com/Lora-net/LoRaMac-node)

## License

MIT
