Release History
===============

# 0.2.1

## features

- new build options
    - LDL_DISABLE_FULL_CHANNEL_CONFIG halves memory required for channel config
    - LDL_DISABLE_CMD_DL_CHANNEL removes handling for this MAC command

## changes

- reduced stack usage during MAC command processing
- Arduino wrapper now uses LDL_ENABLE_STATIC_RX_BUFFER
- Arduino wrapper now uses LDL_DISABLE_CHECK
- Arduino wrapper now uses LDL_DISABLE_DEVICE_TIME
- Arduino wrapper now uses LDL_DISABLE_FULL_CHANNEL_CONFIG
- Arduino wrapper now uses LDL_DISABLE_CMD_DL_CHANNEL

## bugs

- Arduino wrapper on ATMEGA328P was running out of stack at the point where it had to
  process a MAC command. This is a worry because there is ~1K available for stack.

# 0.2.0

Warning: this update has a significant number of interface name changes. 

## features

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

## changes

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

## bugs

none

# 0.1.6

- arduino wrapper improvements
    - debug messages now included/excluded by code in the the event callback handler
    - added sleepy example
    - global duty cycle limit used to ensure TTN fair use policy is met by default
- fixed global duty cycle limit bug where the limit was being reset after a join
