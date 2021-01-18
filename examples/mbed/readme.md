LDL MBED Wrapper with Blocking Interfaces
=========================================

This example runs shows how to run LDL::Device on MBED with the RTOS
profile.

This will probably only work on targets with >20K of RAM.

## Changing the Radio

Two radio shields are in the example, simply adjust the comments
to enable the one you want to try.

The SX126X part is available in a few different variants with IO
lines that tell LDL which driver to use.


## Changing the Region

Region is specified as argument to LDL::Device.start().




