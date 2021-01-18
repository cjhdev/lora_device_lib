LDL Interface Documentation
===========================

Interface documentation for LDL.

## Interface Groups

- [MAC](@ref ldl_mac)
- [Radio Driver](@ref ldl_radio)
- [Chip Interface](@ref ldl_chip_interface)
- [Security Module](@ref ldl_tsm)
- [Default Cryptography](@ref ldl_crypto)
- [Build Options](@ref ldl_build_options)
- [System](@ref ldl_system)

## Useful Links

- [**project repository**](https://github.com/cjhdev/lora_device_lib)
- [**issue tracker**](https://github.com/cjhdev/lora_device_lib/issues)

## Example

Below is an example of how to use the interfaces to activate a device
and send data.

Functions marked as "extern" have not been implemented for brevity.

This example would need the following @ref ldl_build_options to be defined:

- #LDL_ENABLE_SX1272
- #LDL_ENABLE_EU_863_870

@include examples/doxygen/example.c
