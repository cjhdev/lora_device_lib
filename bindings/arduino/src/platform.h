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
#error F_CPU is not defined
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

/* no need to change this */
#define LORA_ENABLE_AVR
#define LORA_DISABLE_FULL_CODEC

/* the atmega328p is short on RAM, you probably shouldn't increase this */
#define LORA_MAX_PACKET 64U

/* optionally constrain to one radio */
#define LORA_ENABLE_SX1272
#define LORA_ENABLE_SX1276

/* optionally constrain to one or more regions */
#define LORA_ENABLE_EU_863_870
//#define LORA_ENABLE_EU_433
//#define LORA_ENABLE_US_902_928
//#define LORA_ENABLE_AU_915_928

/* optionally disable the lengthy start delay */
#define LORA_DISABLE_START_DELAY

#define LORA_SYSTEM_ENTER_CRITICAL(APP) ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
#define LORA_SYSTEM_LEAVE_CRITICAL(APP) }

/* optionally disable these events */
//#define LORA_DISABLE_MAC_RESET_EVENT
//#define LORA_DISABLE_CHIP_ERROR_EVENT
//#define LORA_DISABLE_DOWNSTREAM_EVENT
//#define LORA_DISABLE_TX_BEGIN_EVENT
//#define LORA_DISABLE_TX_COMPLETE_EVENT
//#define LORA_DISABLE_SLOT_EVENT
//#define LORA_DISABLE_CHECK

#endif
