Release History
===============

Note that versions are only "released" when there is a git tag with the same name.
If you have checked out master, the top version listed here may be a
work in progress.

## 0.5.3

This release changes the way tx_gain is applied. Check your code before
updating.

- added support for STM32WL55x
- added MBED wrapper for Nucleo STM32WL55JC kit
- added antenna gain defaults to MBED wrapper
- changed the way tx_gain compensation so that a positive gain
  will subtract from the requested power setting (instead of adding to it)
- fixed bug where radio driver was not being put into sleep before setting
  RX mode for collecting entropy
- doubled stack size for LDL::Device worker thread

## 0.5.2

- added deep sleep locking to wrappers/mbed/device.cpp
- added sleep handling to examples/mbed/bare_metal
- refactored branching in processRX and processTX
- removed useless debug messages
- changed definitions for LDL_L2_VERSION_* to ensure unknown macros (which default to 0) are caught

## 0.5.1

- refactored examples/mbed to contain all mbed examples in subdirectories
- refactored wrappers/mbed to adhere to one class per file
- added one makefile for building all mbed examples in situ
- added Dockerfile for setting up mbed-cli environment
- changed documentation generator to include missing resources causing some page load errors
- changed Ruby wrapper so that gem version is read from version file
- fixed path problem causing Ruby wrapper to not compile when installed via Bundler

## 0.5.0

This release introduces breaking changes for those updating from 0.4.6 and earlier.
Read the porting notes section below if this affects you.

### new features

- added SX1261 and SX1262 radio drivers
- added LDL_MAC_CHANNEL_READY event to indicate when subbands become ready
- added LDL_MAC_OP_ERROR event to indicate when a radio error causes requested operation to fail
- added LDL_MAC_entropy() for applications to read radio entropy if it is required
- added blocking interfaces to ruby wrapper for otaa, confirmed, unconfirmed, entropy
- added radio model and feature for dropping frames to Ruby wrapper
- added Readthedocs integration
- added unlimited duty cycle test mode
- added max EIRP limiting and dwell time control for regions that require it

### changes

- changed ldl_chip_write_fn and ldl_chip_read_fn to support SX126X as well as older SX127X
- updated chip interface documentation and examples
- changed LDL_MAC_radioEvent interface so that radio driver no longer indicates a specific event (required for SX126X)
- added LDL_Radio_getEvent() which is used to read the specific event from the radio
- removed LDL_MAC_DATA_NAK event since it is not useful to the application
- changed next band timing feature to reduce spurious wakeups for sleepy devices
- changed confirmed data retry to use an exponential back-off (previously used OTAA retry algorithm)
- changed OTAA retry algorithm to make use of the LDL_BAND_GLOBAL down-counter instead of a dedicated down-counter
- changed MAC startup behavior so that radio entropy is no longer read automatically
- removed send time dither option from LDL_MAC_unconfirmedData() and LDL_MAC_confirmedData()
- changed OTAA dither time to be configurable at run-time or compile-time
- changed US/AU OTAA to scan all sub-bands before increasing spreading factor
- changed channel selection code to use rate setting as desired rate rather than actual rate
- change MBED LDL::MAC so that duty cycle limit is no longer applied by default
- changed LDL_MAC_init() so that ldl_mac_init_arg.radio_interface must be defined (do not leave NULL!)
- changed LDL_MAC_init() so that ldl_mac_init_arg.sm_interface must be defined (do not leave NULL!)
- removed LDL_Radio_init()
- added specialised radio init functions: LDL_SX1272_init(), LDL_SX1276_init(), LDL_SX1261_init(), LDL_SX1262_init()
- removed non-essential "toString" functions
- removed LDL_DEFAULT_RATE option (prints error if defined)
- added LDL_DISABLE_SF12 option
- added LDL_DISABLE_LINK_CHECK option to alias LDL_DISABLE_CHECK (prints warning if LDL_DISABLE_CHECK defined)
- added LDL_DISABLE_TX_PARAM_SETUP option
- changed LDL_MAC_cancel() behaviour so that radio is always reset when this function is called

### bugs

- fixed bug where cflist didn't apply channel mask for US/AU regions
- fixed bug where duty-cycle off-time was registered before TX meaning off-time was always slightly shorter than
  it should be, especially for large spreading factors

### Porting From 0.4.6

Changes to the chip interfaces were necessary to support both SX126x and SX127x types.

- "opcode" is changed from a single byte to a variable byte buffer
- the read/write functions now return true if they were successful or false if they fail
- SX127x drivers never fail at chip interface and always always return true
- SX126x drivers can fail at chip interface waiting for the busy line to clear
- refer to chip interface example code

MAC no longer reads random from radio on startup. Random must now be requested
using LDL_MAC_entropy() and read from the LDL_MAC_ENTROPY event.

The LDL_MAC_ENTROPY event has been added, the LDL_STARTUP event has been removed.

All MAC interfaces can now be used regardless of the reset state of the radio driver.

The struct ldl_mac_data_opts no longer has a member for timing dither.

ldl_mac_init_arg.radio_interface and ldl_mac_init_arg.sm_interface MUST be defined
for LDL_MAC_init() to be successful. Assertions will remind you if you forget, hence
it is recommneded to have these on at least for development.
This change necessary to ensure that unused code can be removed by the linker.

The radio driver must now be initialised using a specialised init function
for the driver you are initialising:

- LDL_SX1272_init()
- LDL_SX1276_init()
- LDL_SX1261_init()
- LDL_SX1262_init()

LDL_Radio_init() has been removed.

LDL_DEFAULT_RATE build option has been removed. It has been replaced by
LDL_DISABLE_SF12.

## 0.4.6

- added Ruby wrapper and example
- changed LDL_TIMER_BAND to interrupt less frequently

## 0.4.5

- changed LDL_DEBUG, LDL_INFO, LDL_ERROR to not insert __FUNCTION__ into format string
- changed LDL_DEBUG, LDL_INFO, LDL_ERROR to not take APP argument
- reworked log messages and levels
- added LDL_ENABLE_FAST_DEBUG build option which removes extra delays and dithering required
  for use in the field but which slow down development
- added 1.0.x memory saving enhancements (contributed by frbehrens)
- added confirmed downlink acknowledgement logic (contributed by frbehrens)
- fixed bug where data downlinks were not being passed to applications (contributed by frbehrens)
- added 1.0.x default key store optimisation
- changed band timers to use gps timebase (1/256Hz timebase)
- changed time tracker to use gps timebase
- removed logic for detecting and handling late RX windows since this won't normally occur
- removed LDL_MAC_timeSinceValidDownlink()
- removed unused ldl_mac fields
- removed events that are not useful to the application (and replaced with log messages)
    - removed LDL_MAC_CHIP_ERROR event
    - removed LDL_MAC_RESET event
    - removed LDL_MAC_RX1_SLOT event
    - removed LDL_MAC_RX2_SLOT event
    - removed LDL_MAC_DOWNSTREAM
    - removed LDL_MAC_TX_BEGIN
    - removed LDL_MAC_TX_COMPLETE
- added examples/avr to keep an eye on whether LDL still fits on a 328P
- changed LDL_Radio_getAirTime() to return result in milliseconds and not in a timebase of your choice
- fixed most required MISRA 2012 warnings
- added option of defining ldl_mac_init_arg.tps at compile time with LDL_PARAM_TPS
- added option of defining ldl_mac_init_arg.a at compile time with LDL_PARAM_A
- added option of defining ldl_mac_init_arg.b at compile time with LDL_PARAM_B
- added option of defining ldl_mac_init_arg.advance at compile time with LDL_PARAM_ADVANCE
- refactored MBED radio wrapper so that CMWX1ZZABZ aggregates rather than inherits SX1276
- refactored CMWX1ZZABZ driver to use fixed pin mapping (since it is fixed) which further simplifies its use

## 0.4.4

- fixed bug where frequency set by MAC commands was 1/100th of the required value (i.e. I forgot to multiply by 100).
  This affected rxParamSetup, newChannel, and dlChannel commands. It's likely no-one has ever used
  this feature.
- added code to previously empty LDL_Region_validateFreq() to check centre frequency is within bounds for a given region
- changed setChannel() to call LDL_Region_validateFreq() to guard against illegal channel settings
- fixed bug in restoreDefaults() where region will always be set to zero
- refactored restoreDefaults() into separate initSession() and forgetNetwork() functions
- fixed bug where ctx.rx2Rate is used instead of ctx.rx2DataRate on receive
- removed rx2Rate from session struct which appears to be duplicate of rx2DataRate
- added debug code to print session to trace

## 0.4.3

- changed LDL::Device so that worker thread is scheduled to run when a radio ISR
  event is received

## 0.4.2

- fixed RegTcxo bug again: got the mapping backwards in 0.4.1
- fixed bug affecting behaviour of MAC if RX2 slot is handled too late
- added more debug registers that are read when LDL_ENABLE_RADIO_DEBUG is defined
- refactored MBED wrapper to not use EventQueue which was causing timing problems
- added Pout control for RFO SX1276 (was todo)
- added LDL_ENABLE_POINTONE_ERRATA_A1 build option to apply 1.1 A1 errata
- added LDL_DISABLE_RANDOM_DEV_NONCE build option for using a counter based devNonce if LDL_DISABLE_POINTONE is defined

## 0.4.1

- added LDL_TRACE_* macros for verbose debug messages
- added __FUNCTION__ as first argument to LDL_DEBUG(), LDL_INFO(), and LDL_ERROR() level messages to add consistency in logs
- added ldl_radio_debug.c/h to have option of printing register access using the trace macros
- added LDL_ENABLE_RADIO_DEBUG build option to enable trace level radio debugging (depends on LDL_TRACE_*)
- changed radio driver to set the read/write bit in the opcode instead of having chip_adapter do it
- fixed bug in SX1276 driver where RegPllHop is written instead of RegTcxo
- refactored modem config code in ldl_radio.c to work better with debug readback
- added options to mbed wrapper to enable/disable radio debugging

## 0.4.0

In this release effort has gone into making it easier to evaluate
a working version of this project on MBED.

Effort has also gone into simplifying porting. This was to the detriment
of the Arduino wrapper which depended on various strange things
to get around the severe lack of memory on the 328P. The Arduino wrapper
has been removed from the project to save on maintenance. It can be resurrected
if missed, but the memory requirements have increased slightly so it may
no longer work.

There are many breaking changes in this version. If you are updating
from an earlier version you will likely need to make some changes to your
port.

- decoupled MAC from SM by way of struct ldl_sm_interface
- decoupled MAC from Radio by way of struct ldl_radio_interface
- decoupled Radio from Chip Interface by way of struct ldl_chip_interface
- LDL_Radio_setEventCallback() must now be set by the application during
  initialisation (after LDL_MAC_init())
- removed LDL_DISABLE_CMD_DL_CHANNEL option
- removed LDL_DISABLE_FULL_CHANNEL option
- removed all options for disabling MAC callback events
- session keys are now always re-derived as part of LDL_MAC_init (so
  only session and root keys need to persist)
- antenna gain compensation moved from MAC to Radio
- refactored Radio interfaces and simplified MAC-Radio interaction
- refactored MAC states (made some states into operations)
- removed weak symbols since they are no longer required
- added TXCO configuration option to Radio
- added 'chip_set_mode' to the chip interface to combine reset
  line control with accessory line control
- added session_version field to MAC session so that MAC can detect
  when the session structure has changed between firmware versions
- removed Arduino wrapper
- added MBED wrapper
- added MBED examples for RTOS and baremetal profiles
- added LDL_MAC_radioEventWithTicks() to work better with OSes that
  need to defer processing an event with a tick value sampled
  closer to the event
- removed LDL_MAC_errno() since this information is now be returned
  by functions that used to set errno
- replaced LDL_System_eps() and LDL_System_advance() with ldl_mac.a and ldl_mac.advance. These
  must be initialised at LDL_MAC_init()
- defined timer compensation formula to be (compensation = 2*ldl_mac.a*ldl_mac.tps + ldl_mac.b)
- added section on timing compensation to API documentation and porting guide
- LDL_System_ticks replaced by function pointer ldl_mac.ticks
- LDL_System_getBatteryLevel replaced by function pointer ldl_mac.get_battery_level
- LDL_System_rand replaced by function pointer ldl_mac.rand
- optimised radio timeout to avoid adding extra symbols when large spreading factors are used

## 0.3.1

- fixed LDL_MAC_JOIN_COMPLETE event to pass argument instead of NULL (thanks dzurikmiroslav)
- transmit channel is now selected before MIC is generated to solve LoRaWAN 1.1 issues
  caused by the chIndex being part of the MIC generation
- MIC is now updated for redundant transmissions in LoRaWAN 1.1 mode
- fixed implementation to reject MAC commands that would cause all
  channels to be masked
- upstream MAC commands are now deferred until the application sends the next upstream message
- LDL_MAC_unconfirmedData() and LDL_MAC_confirmedData() will now indicate if they
  have failed due to prioritising deferred MAC commands which cannot fit in
  in the same frame
- removed LDL_MAC_setNbTrans() and LDL_MAC_getNbTrans() since the per-invocation
  override feature makes it redundant

## 0.3.0

- BREAKING CHANGE to ldl_chip.h interface to work with SPI transactions; see radio connector documentation or header file for more details
- updated arduino wrapper to work with new ldl_chip.h interface
- fixed SNR margin calculation required for DevStatus MAC command; was
  previously returning SNR not margin
- fixed arduino wrapper garbled payload issue (incorrect session key index from changes made at 0.2.4)
- fopts IV now being correctly generated for 1.1 servers (i was 1 instead of 0)

## 0.2.4

- now deriving join keys in LDL_MAC_otaa() so they are ready to check
  joinAccept
- fixed joinNonce comparison so that 1.1 joins are possible
- join nonce was being incremented before key derivation on joining which
  produced incorrect keys in 1.1 mode
- implemented a special security module for the arduino wrapper to save
  some memory in exchange for limiting the wrapper to LoRaWAN 1.0 servers
- added little endian optimisation build option (LDL_LITTLE_ENDIAN)
- processCommands wasn't recovering correctly from badly formatted
  mac commands.
- added LDL_DISABLE_POINTONE option to remove 1.1 features for devices
  that will only be used with 1.0 servers
- removed LDL_ENABLE_RANDOM_DEV_NONCE since it is now covered by
  LDL_DISABLE_POINTONE
- changed the way ctr mode IV is generated so that a generic ctr implementation
  can now be substituted

## 0.2.3

### bugs

- was using nwk instead of app to derive apps key in 1.1 mode

## 0.2.2

### features

none

### changes

- changed LDL_OPS_receiveFrame() to use nwk key to decrypt and MIC join accepts
  when they are answering a join request

### bugs

- was using app key instead of nwk to decrypt and MIC join accepts

## 0.2.1

### features

- new build options
    - LDL_DISABLE_FULL_CHANNEL_CONFIG halves memory required for channel config
    - LDL_DISABLE_CMD_DL_CHANNEL removes handling for this MAC command

### changes

- reduced stack usage during MAC command processing
- Arduino wrapper now uses LDL_ENABLE_STATIC_RX_BUFFER
- Arduino wrapper now uses LDL_DISABLE_CHECK
- Arduino wrapper now uses LDL_DISABLE_DEVICE_TIME
- Arduino wrapper now uses LDL_DISABLE_FULL_CHANNEL_CONFIG
- Arduino wrapper now uses LDL_DISABLE_CMD_DL_CHANNEL

### bugs

- Arduino wrapper on ATMEGA328P was running out of stack at the point where it had to
  process a MAC command. This is a worry because there is ~1K available for stack.

## 0.2.0

Warning: this update has a significant number of interface name changes.

### features

- LoRaWAN 1.1 support
- encryption and key management is now the domain of the Security Module (SM)
    - keys are referenced by descriptors
    - cipher/plain text is sent to SM for processing
    - generic cryptographic operations decoupled from LoRaWAN concerns
    - implementation is simple to modify or replace completely
- redundant unconfirmed data frames now defer off-time until all frames (i.e. nbTrans) have been sent
    - off-time will never exceed LDL_Region_getMaxDCycleOffLimit()
    - off-time calculated from band limits as well as maxDCycle (as per usual)
- confirmed and unconfirmed data interfaces now accept an invocation option structure
    - can be set to NULL to use defaults
    - can request piggy-backed LinkCheckReq
    - can request piggy-backed DeviceTimeReq
    - can specify nbTrans
    - can specify schedule dither
- confirmed data services now use the same retry/backoff strategy as otaa
- standard retry/backoff strategy now guarantees up to 30 seconds of dither to each retransmission attempt
- antenna gain/loss can now be compensated for at LDL_MAC_init()
- improved doxygen interface documentation

### changes

- changed all source files to use 'ldl' prefix instead of 'lora'
- added   'LDL' and 'ldl' prefixes to all enums that were not yet prefixed
- changed all 'LORA' and 'lora' prefixes to 'LDL' and 'ldl' for consistency
- added   LDL_Radio_interrupt() to take over from LDL_MAC_interrupt()
- changed LDL_MAC_interrupt() to LDL_MAC_radioEvent() which gets called by LDL_Radio_interrupt()
- changed lora_frame.c and lora_mac_commands.c to use the same pack/unpack code
- changed Region module to handle cflist processing instead of MAC
- changed Region module to handle cflist unpacking instead of Frame
- changed Frame to no longer perform encryption/decryption (now the domain of TSM)
- changed MAC to no longer perform key derivation (now the domain of TSM)
- changed uplink and downlink counters to 32 bit
- renamed LDL_MAC_setRedundancy() to LDL_MAC_setNbtrans()
- renamed LDL_MAC_getRedundancy() to LDL_MAC_getNbTrans()
- renamed LDL_MAC_setAggregated() to LDL_MAC_setMaxDCycle()
- renamed LDL_MAC_getAggregated() to LDL_MAC_getMaxDCycle()
- renamed LDL_System_time() to LDL_System_ticks()
- changed LDL_MAC_interrupt() to use one less argument (no need to pass time)
- changed LDL_MAC_unconfirmedData() to accept additional argument (invocation option structure)
- changed LDL_MAC_confirmedData() to accept additional argument(invocation option structure)
- changed OTAA procedure so that maxDCycle is no longer applied (this should only apply to data service)
- changed radio to board interfaces (now using LDL_Chip_*)
- removed LDL_MAC_ticksUntilNextChannel()
- removed LDL_MAC_check() since this can now be requested via invocation option structure
- removed LDL_MAC_setSendDither() since this can now be requested via invocation option structure
- removed weak implementations of mandatory system interfaces
- removed lora_board.c and lora_board.h
- added   lora_chip.h
- changed LDL_System_rand() to accept additional argument (app pointer)
- changed LORA_DEBUG() to accept argument (app pointer)
- changed LORA_INFO() to accept argument (app pointer)
- changed LORA_ERROR() to accept argument (app pointer)
- changed how off-time is accounted for

### bugs

none

## 0.1.6

- arduino wrapper improvements
    - debug messages now included/excluded by code in the the event callback handler
    - added sleepy example
    - global duty cycle limit used to ensure TTN fair use policy is met by default
- fixed global duty cycle limit bug where the limit was being reset after a join
