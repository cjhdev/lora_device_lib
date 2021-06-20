Porting Guide
=============

## General

- LDL interfaces, except those marked as interrupt-safe, must be accessed from a single thread of execution.
- If interrupt-safe interfaces are accessed from ISRs
    - LDL_SYSTEM_ENTER_CRITICAL() and LDL_SYSTEM_LEAVE_CRITICAL() must be defined
    - the ISR must have a higher priority than the non-interrupt thread of execution
    - bear in mind that interrupt-safe interfaces never block and return as quickly as possible
- Check the interface documentation
- Porting is difficult

## Basic Steps

1. Select L2 Specification via build options.

    Define LDL_LL_VERSION as one of:

    - LDL_LL_VERSION_1_0_3 (recommended)
    - LDL_LL_VERSION_1_0_4 (recommended)
    - LDL_LL_VERSION_1_1

2. Implement the following system functions

    - `ldl_mac_init_arg.ticks()`
    - `ldl_mac_init_arg.rand()`

3. Tell LDL the rate at which `ldl_mac_init_arg.ticks()` increments
   by setting `ldl_mac_init_arg.tps` to that rate

4. Define the following macros (if interrupt-safe functions are called from ISRs)

    - LDL_SYSTEM_ENTER_CRITICAL()
    - LDL_SYSTEM_LEAVE_CRITICAL()

5. Enable at least one region via build options

    - LDL_ENABLE_EU_863_870
    - LDL_ENABLE_US_902_928
    - LDL_ENABLE_AU_915_928
    - LDL_ENABLE_EU_433

6. Enable at least one radio driver via build options

    - LDL_ENABLE_SX1272
    - LDL_ENABLE_SX1276
    - LDL_ENABLE_SX1261
    - LDL_ENABLE_SX1262
    - LDL_ENABLE_WL55

7. Implement the chip interface to reach radio chip

    - remember to call LDL_Radio_handleInterrupt() from DIO rising edge interrupt

8. Initialise in correct order

    MAC depends on Radio to be initialised first, and Radio should be
    set to callback to MAC only after MAC has been initialised.

    In other words:

    1. LDL_SX12xx_init() and LDL_SM_init()
    2. LDL_MAC_init()
    3. LDL_Radio_setEventCallback()

9. Poll LDL_MAC_process() to make it run

## Building Source

- define preprocessor symbols as needed ([see build options](https://ldl.readthedocs.io/en/latest/group__ldl__build__options.html))
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
arg.tps = 1000000;    // 1MHz

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

- delay_seconds is the delay from the end of TX to the beginning of RX (anywhere in the range 1 to 7 seconds)
- `ldl_mac_init_arg.a` is clock error per second measured in ticks
- `ldl_mac_init_arg.b` is a constant value

If `ldl_mac_init_arg.tps` is faster than 1KHz, it may be possible to measure
uncertainty in whole ticks. Compensation values may look like this:

| type    | value  |
| ------- | ------ |
| a       | 10     |
| b       | 0      |

If `ldl_mac_init_arg.tps` is around 1KHz, the error in ticks will be a fraction of 1
and so `ldl_mac_init_arg.a` will be set to 0.
You could round `ldl_mac_init_arg.a` up to 1, but this would overcompensate
for windows that open later than one second. In this situation it makes
sense to set a constant error correction using `ldl_mac_init_arg.b`.

For example, you might compensate for `ldl_mac_init_arg.tps` of 1KHz
using the following setting:

| type    | value  |
| ------- | ------ |
| a       | 0      |
| b       | 2      |


### Shifting Run-Time Configuration to Compile Time

Some configuration items can be defined at compile time if that
better suits the application:

- LDL_PARAM_TPS (replaces `ldl_mac_init_arg.tps`)
- LDL_PARAM_A (replaces `ldl_mac_init_arg.a`)
- LDL_PARAM_B (replaces `ldl_mac_init_arg.b`)
- LDL_PARAM_ADVANCE (replaces `ldl_mac_init_arg.advance`)
- LDL_PARAM_BEACON_INTERVAL (replaces `ldl_mac_init_arg.beaconInterval`)

### Managing Device Nonce (devNonce)

LDL will push 'nextDevNonce' to the application as argument to the LDL_MAC_DEV_NONCE_UPDATED event.
The application can restore this value as 'devNonce' argument to LDL_MAC_init().

DevNonce is incremented for each join request frame sent during OTAA. The
server will ignore frames containing DevNonce values it has seen before.

LDL cannot restore DevNonce from session state since it is longer lived
than session state.

### Managing Join Nonce (joinNonce)


LDL will push 'joinNonce' to the application as argument to the LDL_MAC_JOIN_COMPLETE event.
The applciation can restore this value as 'joinNonce' argument to LDL_MAC_init().

LDL will reject join accept messages that have a joinNonce field less than the joinNonce
stored in memory.

LDL cannot restore JoinNonce from session state since it is longer lived than session
state.

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

Note that:

- session state does not contain the keys (i.e. nwkKey and/or appKey)
- session state and keys are used together to derive the session keys
- loading invalid/corrupt session state may result in undefined behaviour

LDL does not implement migration of session state between versions. This means if you update
the firmware on a device, it is best practice to ensure old session state is discarded.

### Sleep Mode

LDL is designed to work with applications that use sleep mode.

- ensure that LDL_System_ticks() is incremented by a time source
that keeps working in sleep mode
- use LDL_MAC_ticksUntilNextEvent() to set a wakeup timer before entering sleep mode
- use external interrupts to call LDL_Radio_interrupt()
- use LDL_MAC_priority() to determine if the MCU can enter a deep sleep
  that would require many milliseconds to wake from since a slow
  wakeup may cause you to be late to handling an event

### Co-operative Scheduling

LDL is designed to share a single thread of execution with other tasks.

LDL works by scheduling time based events. Some of these events can be handled
late without affecting the LoRaWAN, while others will cause serious problems like
not being able to receive frames.

LDL provides the LDL_MAC_priority() interface so that a co-operative scheduler can
check if another tasking running for the next CEIL(n) seconds will cause
a problem for LDL.

### Reducing/Changing Memory Use

Flash memory usage can be reduced by:

- not enabling radio drivers which are not needed
- not enabling regions which are not needed
- reimplementing the default Security Module ([ldl_sm.c](src/ldl_sm.c)) to use a hardware peripheral
- not defining LDL_INFO, LDL_DEBUG, LDL_ERROR, LDL_TRACE_*
- shifting run-time configuration to compile-time [see above](#shifting-run-time-configuration-to-compile-time)

Static RAM usage can be reduced by:

- using a smaller frame buffer by redefining LDL_MAX_PACKET
    - default is 255 bytes
    - an investigation is required to determine safe minimums for your region

Automatic RAM usage can be reduced by:

- shifting the frame receive buffer from stack to bss by defining LDL_ENABLE_STATIC_RX_BUFFER
    - this will reduce stack usage during LDL_MAC_process()

### Compensating Antenna Gain

See radio driver configuration.

### Configuring Radio Driver

The radio driver needs to be configured to suit your hardware. A range
of configuration options exist depending on the driver.

These options are documented in the radio driver interface documentation.

### Debugging Radio Driver

Not a porting issue, but if you need to see what is being written/read to
the transceiver you can see this information printed by defining:

- LDL_ENABLE_RADIO_DEBUG
- LDL_TRACE_*

Enabling this feature will increase RAM and flash usage. The RAM is used
to buffer register accesses so that printing doesn't affect time
sensitive operations but this can still cause timing problems.
Enabling this logging for releases is not recommended.

### LoRaWAN 1.1 Errata

There is an errata dated 26 Jan 2018 that changes the way Fopts are
encrypted in 1.1 mode. This document is public but is still marked as
proposed.

By default this change is not applied to the source, but it can be enabled
by defining:

- LDL_ENABLE_ERRATA_A1




