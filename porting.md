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

    - `ldl_mac_init_arg.ticks()`
    - `ldl_mac_init_arg.rand()`

3. Tell LDL the rate at which `ldl_mac_init_arg.ticks()` increments
   by setting `ldl_mac_init_arg.tps` to that rate

4. Define the following macros (if interrupt-safe functions are called from ISRs)

    - LDL_SYSTEM_ENTER_CRITICAL()
    - LDL_SYSTEM_LEAVE_CRITICAL()

5. Define at least one region via build options

    - LDL_ENABLE_EU_863_870
    - LDL_ENABLE_US_902_928
    - LDL_ENABLE_AU_915_928
    - LDL_ENABLE_EU_433

6. Define at least one radio driver via build options

    - LDL_ENABLE_SX1272
    - LDL_ENABLE_SX1276

7. Implement the chip interface

    - See [chip interface](https://cjhdev.github.io/lora_device_lib_api/group__ldl__chip__interfaces.html)

8. Initialise in correct order

    MAC depends on Radio to be initialised first, and Radio should be
    set to callback to MAC only after MAC has been initialised.

    In other words:

    1. LDL_Radio_init() and LDL_SM_init()
    2. LDL_MAC_init()
    3. LDL_Radio_setEventCallback()

9. Integrate with your application

    - See [main()](examples/doxygen/example.c)

## Building Source

- define preprocessor symbols as needed ([see build options](https://cjhdev.github.io/lora_device_lib_api/group__ldl__build__options.html))
- add `include` folder to include search path
- build all sources in `src`

## Advanced Topics

### Timing

LDL tracks time by calling `ldl_mac_init_arg.ticks()`. The value returned is a 32 bit integer that
increments at a rate `ldl_mac_init_arg.tps` ticks per second (i.e. Hz).

e.g.

~~~
extern uint32_t your_system_ticks(void *app);

struct ldl_mac_init_arg arg = {0};

arg.ticks = your_system_ticks;
arg.tps = 1000000UL;    // 1MHz

// ...

LDL_MAC_init(&mac, LDL_EU_863_870, &arg);
~~~

### Compensating for Timing Error

LDL tracks time by calling `ldl_mac_init_arg.ticks()`. This value will be
incremented by some clock source on the target, and that clock
source will have a +/- uncertainty (error).

If the uncertainty is significant (i.e. large enough to cause LDL to miss RX windows) then
LDL must compensate by opening RX windows ealier and increasing the symbol timeout.

LDL calculates the amount of compensation using this formula:

~~~
compensation = (delay_seconds * ldl_mac_init_arg.a * 2UL) + ldl_mac_init_arg.b;
~~~

- delay seconds is the delay from the end of TX to the beginning of RX (anywhere in the range 1 to 7 seconds)
- `ldl_mac_init_arg.a` is clock error per second measured in ticks
- `ldl_mac_init_arg.b` is a constant value

`ldl_mac_init_arg.tps` is faster than 1KHz (e.g. 1MHz) it may be possible to measure
uncertainty in whole ticks. Compensation values may look like this:

| type    | value  |
| ------- | ------ |
| a       | 10     |
| b       | 0      |

If `ldl_mac_init_arg.tps` is around 1KHz, `ldl_mac_init_arg.a` will be a fraction of a tick.
At 1KHz there will be a constant error of +/- 500us at the point where the timestamp
is recorded and again where the scheduler opens the window.

You could round `ldl_mac_init_arg.a` up to 1, but this would overcompensate
for windows that open later than one second. In this situation it makes
sense to set `ldl_mac_init_arg.b` like so:

| type    | value  |
| ------- | ------ |
| a       | 0      |
| b       | 2      |


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

There are several ways to augment or replace the default security module and
cipher modes.

One option might be to replace the default modes with hardened implementations.
This would be achived by removing the files containing the default modes
from the build and reimplementing the interfaces.
The defalt modes are implemented in the following files:

- AES128-ECB (default: [ldl_aes.c](src/ldl_aes.c))
- AES128-CTR (default: [ldl_ctr.c](src/ldl_aes.c))
- AES128-CMAC (default: [ldl_cmac.c](src/ldl_aes.c))

The same approach can be applied to the default security module ([ldl_sm.c](src/ldl_sm.c))
code which calls into these implementations.

It is also possible to redirect to a different security module at run-time by changing
the ldl_mac_init_arg.sm_interface structure which MAC uses to call into SM.

This feature was added to make it possible to upgrade the SM by subclassing
in C++ projects. An example of this can be seen in the [MBED wrapper](wrappers/mbed).

### Persistent Sessions

To implement persistent sessions the application must:

- be able to cache session state passed with the LDL_MAC_SESSION_UPDATED event
- be able to restore session state prior to calling LDL_MAC_init()
- be able to pass a pointer to the restored session state to LDL_MAC_init()
- ensure the integrity of state and root keys

Note that:

- session state does not contain sensitive information
- session state is best treated as opaque data
- loading invalid/corrupt session state may result in undefined behaviour
- LDL is able to detect and reset incompatible state moving between firmware versions

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
- reimplementing the default Security Module ([ldl_sm.c](src/ldl_sm.c)) to use a hardware peripheral
- not defining LDL_INFO, LDL_DEBUG, LDL_TRACE_*

Static RAM usage can be reduced by:

- using a smaller frame buffer by redefining LDL_MAX_PACKET
    - default is 255 bytes
    - an investigation is required to determine safe minimums for your region

Automatic RAM usage can be reduced by:

- shifting the frame receive buffer from stack to bss by defining LDL_ENABLE_STATIC_RX_BUFFER
    - this will reduce stack usage during LDL_MAC_process()

### Compensating For Antenna Gain

Initialise Radio with ldl_radio_init_arg.tx_gain set to a value you wish to boost
or attenuate by.

Note the value here is (dB x 10-2) so 2.4dB would be 240.

### Configuring Radio Driver

The radio driver needs to be configured to suit your hardware. The following
configuration options exist:

- Transceiver type (ldl_radio_init_arg.type)

- Antenna transmit gain compensation (ldl_radio_init_arg.tx_gain)

  Note that the value here is (dB x 10^-2) so 2.4dB would be 240.

- PA amp selection (ldl_radio_init_arg.pa)

  With SX1272/6 transceiver you need to tell the driver if it can use one
  or both power amplifier circuits. This is because your hardware might not
  implement switching required to use both.

- XTAL selection (ldl_radio_init_arg.xtal)

  The driver needs to know if the transceiver is clocked by a crystal or
  external oscillator.

- XTAL startup delay in milliseconds (ldl_radio_init_arg.xtal_delay)

  A crystal will stabilise in less than one millisecond, but an external
  oscillator might take several milliseconds. This value is returned
  to MAC and used to set a delay between turning on the transceiver
  and performing an operation.


The rest should be explained in the chip interface documentation.

### Debugging Radio Driver

Not a porting issue, but if you need to see what is being written/read to
the transceiver you can see this information printed by defining:

- LDL_ENABLE_RADIO_DEBUG
- LDL_TRACE_*

Enabling this feature will increase RAM and flash usage. The RAM is used
to buffer register accesses so that printing doesn't affect time
sensitive operations. You probably shouldn't leave this on in releases
since it is very verbose.

### LoRaWAN 1.1 Errata

There is an errata dated 26 Jan 2018 that changes the way Fopts are
encrypted in 1.1 mode. This document is public but is still marked as
proposed.

By default this change is not applied to the source, but it can be enabled
by defining:

- LDL_ENABLE_POINTONE_ERRATA_A1


