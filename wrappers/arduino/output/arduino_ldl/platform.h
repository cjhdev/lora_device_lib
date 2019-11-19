/* Copyright (c) 2019 Cameron Harper
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * */

#ifndef PLATFORM_H
#define PLATFORM_H

#include <util/atomic.h>

#ifndef F_CPU
#   error "F_CPU is not defined"
#endif

/* timing settings */

/* +/- error as PPM
 * 
 * Assume a ceramic resonator has ~0.5% error, so
 * 
 * 16M * 0.005 = 80000
 * 
 * 80000 / 16 = 5000
 * 
 * */
#define XTAL_PPM 5000

/* optimisations ******************************************************/

/* the atmega328p is short on RAM, you probably shouldn't increase this */
#define LDL_MAX_PACKET 64U

/* optionally constrain to one radio */
#define LDL_ENABLE_SX1272
#define LDL_ENABLE_SX1276

/* optionally constrain to one or more regions */
#define LDL_ENABLE_EU_863_870
//#define LDL_ENABLE_EU_433
//#define LDL_ENABLE_US_902_928
//#define LDL_ENABLE_AU_915_928

/* define to keep the RX buffer in mac state rather than on the stack */
//#define LDL_ENABLE_STATIC_RX_BUFFER

/* time in milliseconds until channels become available after device reset
 * 
 * this is a safety feature for devices stuck in a transmit-reset loop, perhaps
 * due to an end-of-life battery .
 * 
 *  */
#define LDL_STARTUP_DELAY 0UL

#define LDL_DISABLE_SESSION_UPDATE
#define LDL_ENABLE_RANDOM_DEV_NONCE

/* optionally disable these event callbacks */
//#define LDL_DISABLE_MAC_RESET_EVENT
//#define LDL_DISABLE_CHIP_ERROR_EVENT
//#define LDL_DISABLE_DOWNSTREAM_EVENT
//#define LDL_DISABLE_TX_BEGIN_EVENT
//#define LDL_DISABLE_TX_COMPLETE_EVENT
//#define LDL_DISABLE_SLOT_EVENT
//#define LDL_DISABLE_JOIN_TIMEOUT_EVENT
//#define LDL_DISABLE_JOIN_COMPLETE_EVENT
//#define LDL_DISABLE_DATA_TIMEOUT_EVENT
//#define LDL_DISABLE_DATA_COMPLETE_EVENT
//#define LDL_DISABLE_DATA_CONFIRMED_EVENT
//#define LDL_DISABLE_RX_EVENT

/* optionally disable link check */
//#define LDL_DISABLE_CHECK

/* do not change ******************************************************/

#define LDL_ENABLE_AVR
#define LDL_SYSTEM_ENTER_CRITICAL(APP) ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
#define LDL_SYSTEM_LEAVE_CRITICAL(APP) }

#endif
