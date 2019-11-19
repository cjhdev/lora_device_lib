Porting Guide
=============

## General

- LDL interfaces, except those marked as interrupt-safe, must be accessed from a single thread of execution. 
- If interrupt-safe interfaces are accessed from ISRs
    - LDL_SYSTEM_ENTER_CRITICAL() and LDL_SYSTEM_LEAVE_CRITICAL() must be defined
    - the ISR must have a higher priority than the non-interrupt thread of execution
    - bear in mind that interrupt-safe interfaces never block and return as quickly as possible

## Basic Steps

1. Review Doxygen groups

    - [system](https://cjhdev.github.io/lora_device_lib_api/group__ldl__system.html)
    - [radio connector](https://cjhdev.github.io/lora_device_lib_api/group__ldl__radio__connector.html)
    - [build options](https://cjhdev.github.io/lora_device_lib_api/group__ldl__build__options.html)

2. Implement the following system functions

    - LDL_System_ticks()
    - LDL_System_tps()
    - LDL_System_eps()

3. Define the following macros (if interrupt-safe functions are called from ISRs)

    - LDL_SYSTEM_ENTER_CRITICAL()
    - LDL_SYSTEM_LEAVE_CRITICAL()

4. Define at least one region via build options

    - LDL_ENABLE_EU_863_870
    - LDL_ENABLE_US_902_928
    - LDL_ENABLE_AU_915_928
    - LDL_ENABLE_EU_433

5. Define at least one radio driver via build options

    - LDL_ENABLE_SX1272
    - LDL_ENABLE_SX1276

6. Implement the radio connector

    - See [radio connector](https://cjhdev.github.io/lora_device_lib_api/group__ldl__radio__connector.html)

7. Integrate with your application

    - See [main()](examples/doxygen/example.c)

## Building Source

- define preprocessor symbols as needed ([see build options](https://cjhdev.github.io/lora_device_lib_api/group__ldl__build__options.html))
- add `include` folder to include search path
- build all sources in `src`

## Advanced Topics

### Managing Device Nonce (devNonce)

LoRaWAN 1.1 redefined devNonce to be a 16 bit counter from zero where previously it had been
a random number. The counter increments after each successful OTAA. A LoRaWAN 1.1 server
will not accept a devNonce less than one it has seen before. LoRaWAN 1.1 implementations must therefore maintain this 
counter over the lifetime of the device if there is an expectation to enter join mode
again before the root keys and/or joinEUI are refreshed.

LDL will push nextDevNonce to the application as an argument to the LDL_MAC_JOIN_COMPLETE event. A cached
value can be restored by passing it as an argument to LDL_MAC_init().

LDL does not keep this counter with session state since it is longer lived than session state.

### Managing Join Nonce (joinNonce)

LoRaWAN 1.1 renamed appNonce to joinNonce and declared it to be a counter of the number of successful joins maintained
by the server.

When the joinAccept indicates that LoRaWAN 1.1 is in use, LDL will only accept the joinAccept if joinNonce is
greater than the last cached joinNonce. 

LDL will push the updated value to the application as an argument to the LDL_MAC_JOIN_COMPLETE event. A cached
value can be restored by passing it as an argument to LDL_MAC_init().

LDL does not keep this counter with session state since it is longer lived than session state.

Caching/restoring this value is optional since resetting it to zero at LDL_MAC_init() will not
stop joinAccept from being accepted, but it will leave LDL open to replay attacks during OTAA.

### Modifying the Security Module

The default cryptographic implementations can be replaced with hardened implementations.

LDL depends on the following modes which are integrated within the default Security Module ([ldl_sm.c](src/ldl_sm.c)):

- AES128-ECB (default: [lora_aes.c](src/ldl_aes.c))
- AES128-CTR (default: [lora_ctr.c](src/ldl_aes.c))
- AES128-CMAC (default: [lora_cmac.c](src/ldl_aes.c))

If your toolchain supports weak symbols, replacing some or all of the
default implementations is as simple as re-implementing the function
somewhere else.

If you don't want to use weak symbols, remove ldl_sm.c
from the build and re-implement the whole thing somewhere else. 
The same applies to situations where a more robust security 
module (i.e. HSM) is required.

### Persistent Sessions

To implement persistent sessions the application must:

- be able to cache session state passed with the LDL_MAC_SESSION_UPDATED event
- be able to cache session keys updated as part of the LDL_MAC_JOIN_COMPLETE event
- be able to restore session keys prior to calling LDL_MAC_init()
- be able to restore session state prior to calling LDL_MAC_init()
- be able to pass a pointer to the restored session state to LDL_MAC_init()
- ensure the integrity of state and keys
    - this also means ensuring that session keys are actually valid for session state

Note that:

- session state does not contain sensitive information
- session state is best treated as opaque data
- loading invalid/corrupt session state may result in undefined behaviour

### Sleep Mode

LDL is designed to work with applications that use sleep mode.

- ensure that LDL_System_ticks() is incremented by a time source
that keeps working in sleep mode
- use LDL_MAC_ticksUntilNextEvent() to set a wakeup timer before entering sleep mode
- use external interrupts to call LDL_Radio_interrupt()

### Co-operative Scheduling

LDL is designed to share a single thread of execution with other tasks.

LDL works by scheduling time based events. Some of these events can be handled
late without affecting the LoRaWAN, while others will cause serious problems like 
missing frames.

LDL provides the LDL_MAC_priority() interface so that a co-operative scheduler can
check if another tasking running for the next CEIL(n) seconds will cause
a problem for LDL. 

### Reducing/Changing Memory Use

Flash memory usage can be reduced by:

- not enabling radio drivers which are not needed
- not enabling regions which are not needed
- disabling unhandled events (LDL_DISABLE_*_EVENT)
- modifying the default Security Module ([ldl_sm.c](src/ldl_sm.c)) to use a hardware peripheral 

Static RAM usage can be reduced by:

- using a smaller frame buffer by redefining LDL_MAX_PACKET
    - default is 255 bytes
    - an investigation is required to determine safe minimums for your region

Automatic RAM usage can be reduced by:

- shifting the frame receive buffer from stack to bss by defining LDL_ENABLE_STATIC_RX_BUFFER
    - this will reduce stack usage during LDL_MAC_process()

### Compensating For Gain

Gain can be compensated by setting the ldl_mac_init_arg.gain prior to calling LDL_MAC_init().
This value will be added to the dBm setting requested from the Radio at transmit time.
