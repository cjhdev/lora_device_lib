/* Copyright (c) 2019-2020 Cameron Harper
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

#ifndef LDL_SYSTEM_H
#define LDL_SYSTEM_H

/** @file */

/**
 * @defgroup ldl_system System
 *
 * @ref ldl_mac accesses various things through system interfaces which
 * must be passed as function pointers to @ref ldl_mac during LDL_MAC_init().
 *
 * The interfaces are:
 *
 * - #ldl_mac_init_arg.ticks
 * - #ldl_mac_init_arg.rand
 * - #ldl_mac_init_arg.get_battery_level
 *
 * In addition to function pointers, the following macros *MUST* be
 * defined if LDL_MAC_radioEvent(), LDL_MAC_radioEventWithTicks(), or LDL_MAC_ticksUntilNextEvent()
 * are called from an interrupt:
 *
 * - LDL_SYSTEM_ENTER_CRITICAL()
 * - LDL_SYSTEM_LEAVE_CRITICAL()
 *
 * @{
 * */


#ifdef __cplusplus
extern "C" {
#endif

#include "ldl_platform.h"
#include <stdint.h>
#include <stdbool.h>

/** This function reads a free-running 32 bit counter. The counter
 * is expected to increment at a rate of #ldl_mac_init_arg.tps ticks per second.
 *
 * LDL uses this changing value to track of the passage of time and perform
 * scheduling.
 *
 * If you intend to keep LDL running in deep sleep modes, make sure that
 * the timer continues to increment in this mode.
 *
 * @note this may be called from mainloop or interrupt
 *
 * @param[in]   app     from #ldl_mac_init_arg.app
 *
 * @return ticks
 *
 * */
typedef uint32_t (*ldl_system_ticks_fn)(void *app);

/** LDL uses this function to select channels and to add random dither
 * to scheduled events.
 *
 * @param[in]     app     from #ldl_mac_init_arg.app
 * @retval (0..UINT32_MAX)
 *
 * */
typedef uint32_t (*ldl_system_rand_fn)(void *app);

/** LDL uses this function to discover the battery level for for device
 * status MAC command.
 *
 * @param[in]   app     from #ldl_mac_init_arg.app
 *
 * @return      DevStatusAns.battery
 * @retval      255 not implemented
 *
 * */
typedef uint8_t (*ldl_system_get_battery_level_fn)(void *app);

#ifndef LDL_SYSTEM_ENTER_CRITICAL

/** Expanded inside functions which might be called from both mainloop
 * and interrupt. At the moment only LDL_MAC_ticksUntilNextEvent() and
 * LDL_MAC_interrupt() are safe to call this way.
 *
 * Always paired with #LDL_SYSTEM_LEAVE_CRITICAL within the same function like so:
 *
 * @code{.c}
 * void some_function(void *app)
 * {
 *      LDL_SYSTEM_ENTER_CRITICAL(app)
 *
 *      // critical section code
 *
 *      LDL_SYSTEM_LEAVE_CRITICAL(app)
 * }
 * @endcode
 *
 * If you are using avr-libc:
 *
 * @code{.c}
 * #include <util/atomic.h>
 *
 * #define LDL_SYSTEM_ENTER_CRITICAL(APP) ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
 * #define LDL_SYSTEM_LEAVE_CRITICAL(APP) }
 * @endcode
 *
 * If you are using CMSIS:
 * @code{.c}
 * #define LDL_SYSTEM_ENTER_CRITICAL(APP) volatile uint32_t primask = __get_PRIMASK(); __disable_irq();
 * #define LDL_SYSTEM_LEAVE_CRITICAL(APP) __set_PRIMASK(primask);
 * @endcode
 *
 * @param[in] APP   from #ldl_mac_init_arg.app
 *
 * */
#define LDL_SYSTEM_ENTER_CRITICAL(APP)
#endif

#ifndef LDL_SYSTEM_LEAVE_CRITICAL

/** Expanded inside functions which might be called from both mainloop
 * and interrupt.
 *
 * Always paired with #LDL_SYSTEM_ENTER_CRITICAL within the same function.
 *
 * @param[in] APP   from #ldl_mac_init_arg.app
 *
 * */
#define LDL_SYSTEM_LEAVE_CRITICAL(APP)
#endif

#ifdef __cplusplus
}
#endif


/** @} */
#endif
