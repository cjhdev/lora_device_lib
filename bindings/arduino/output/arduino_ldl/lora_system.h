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

#ifndef LORA_SYSTEM_H
#define LORA_SYSTEM_H

/** @file */

/** 
 * @defgroup ldl_optional Optional
 * @ingroup ldl
 * 
 * Functions and macros which LDL depends on. 
 * 
 * These interfaces and macros have default implementations
 * and definitions. They are in the optional group because they are
 * designed to be redefined to suit the target.
 * 
 * */

/**
 * @defgroup ldl_system System
 * @ingroup ldl
 *
 * System interfaces.
 * 
 * LDL includes weak implementations for each of these functions. 
 * Different targets may or may not need to implement their own versions.
 * 
 * @{
 * */

#ifdef __cplusplus
extern "C" {
#endif

#include "lora_platform.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

struct lora_mac_session;
struct on_timeout;

/** identifiers and application key */
struct lora_system_identity {
  
    uint8_t appEUI[8U];     /**< application identifier */
    uint8_t devEUI[8U];     /**< device identifier */
    uint8_t appKey[16U];    /**< application key */
};

/** Get system time (ticks)
 * 
 * @note system time must increment at a rate of LDL_System_tps()
 * @warning this function must be implemented on target for correct operation
 * 
 * @return system time
 * 
 * */
uint32_t LDL_System_time(void);

/** Get AppEUI, DevEUI, and DevKey
 * 
 * @warning this function must be implemented on target for correct operation
 * 
 * @param[in] app   application state
 * @param[out] value
 * 
 * */
void LDL_System_getIdentity(void *app, struct lora_system_identity *value);

/** Get battery level
 *  
 * @param[in] app   application state
 * @return battery level
 * @retval 255 not implemented
 * 
 * */
uint8_t LDL_System_getBatteryLevel(void *app);

/** Get a random number in range 0..255
 * 
 * @return random number in range 0..255
 * 
 * */
uint8_t LDL_System_rand(void);

/** The number of ticks in one second
 * 
 * @return ticks per second
 * 
 * */
uint32_t LDL_System_tps(void);

/** XTAL error per second in ticks
 * 
 * For example, if an oscillator is accurate to +/1% of nominal:
 * 
 * @code
 * 
 * F_CPU := 8000000UL
 * PRESCALE := 64UL
 * ERROR := 0.01
 * 
 * # works out to 1250 ticks
 * XTAL_ERROR := '( $(F_CPU) * $(ERROR) / $(PRESCALE) )'
 * 
 * @endcode
 * 
 * @return xtal error in ticks
 * 
 * */
uint32_t LDL_System_eps(void);


/** Advance schedule by this many ticks to compensate for delay
 * in processing an interrupt
 * 
 * - advances RX1 and RX2 window schedule by so many system time ticks
 * 
 * */
uint32_t LDL_System_advance(void);

/** Restore saved context
 * 
 * @note called only once during #LDL_MAC_init
 * 
 * @param[in] app    application state
 * @param[out] value
 * 
 * @retval true if context was restored
 * @retval false MAC will restore session from defaults
 * 
 * */
bool LDL_System_restoreContext(void *app, struct lora_mac_session *value);

/** Save context
 * 
 * @note This will be called by MAC every time a member in lora_mac_session changes
 * 
 * @param[in] app
 * @param[in] value
 * 
 * */
void LDL_System_saveContext(void *app, const struct lora_mac_session *value);

#ifndef LORA_SYSTEM_ENTER_CRITICAL

/** Expanded inside functions which might be called from both mainloop 
 * and interrupt.
 * 
 * Always paired with #LORA_SYSTEM_LEAVE_CRITICAL within the same function like so:
 * 
 * @code
 * void LDL_Event_xxx(void *app)
 * {
 *      LORA_SYSTEM_ENTER_CRITICAL(app)
 * 
 *      // critical section code
 * 
 *      LORA_SYSTEM_LEAVE_CRITICAL(app)
 * }
 * @endcode
 * 
 * Note that there are no semicolons following the 
 * macro when it appears in LDL. This means if you are using avr-libc you 
 * are free to insert the ATOMIC_BLOCK macro like so:
 * 
 * @code
 * #include <util/atomic.h>
 * 
 * #define LORA_SYSTEM_ENTER_CRITICAL(APP) ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
 * #define LORA_SYSTEM_ENTER_CRITICAL(APP) }
 * @endcode
 * 
 * @param[in] app   application state
 * 
 * */
#define LORA_SYSTEM_ENTER_CRITICAL(APP)
#endif

#ifndef LORA_SYSTEM_LEAVE_CRITICAL

/** Expanded inside functions which might be called from both mainloop 
 * and interrupt.
 * 
 * Always paired with #LORA_SYSTEM_ENTER_CRITICAL within the same function. 
 * See #LORA_SYSTEM_ENTER_CRITICAL for example.
 * 
 * @param[in] app   application state
 * 
 * */
#define LORA_SYSTEM_LEAVE_CRITICAL(APP)
#endif

#ifdef __cplusplus
}
#endif


/** @} */
#endif
