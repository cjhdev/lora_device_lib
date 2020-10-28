This example runs shows how to run LDL::MAC on MBED with the bare metal
profile.

The target in this case is expected to be the DISCO_L072CZ_LRWAN1 which
features the Murata ABZ module.

The wrapper for this module takes care of all the connections for you,
the only option left to the integrator is which pin to use to switch
TCXO. On the DISCO_L072CZ_LRWAN1 the default pin for controlling TCXO is PA_12
but this can be moved using jumper wire.

If you are using this module on bespoke hardware, make sure you have completed
the pin map correctly and used the STM32 names (e.g. PA_12).




