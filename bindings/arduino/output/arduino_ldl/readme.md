ArduinoLDL
==========

An Arduino library that wraps [LoraDeviceLib](https://github.com/cjhdev/lora_device_lib).

## Supported Targets

- ATMEGA328p based boards with ceramic resonators and crystal oscillators

## Optimisation

ArduinoLDL can be optimised and adapted by changing the definitions in [platform.h](platform.h).

## Interface

See [arduino_ldl.h](arduino_ldl.h).

Note that `DEBUG_LEVEL` is optional.

- `DEBUG_LEVEL 1` will print summary status information to Serial
- `DEBUG_LEVEL 2` will print all status information to Serial

Reset, select, and dio pins can be moved to suit your hardware. All
of these connections are required for correct operation.

## Examples

See [examples](examples).

## License

MIT for the wrapper, example sketches, and core LoraDeviceLib code.

Some of the examples depend on third-party libraries. These libraries are 
kept in separate folders and have their own license T&Cs.
