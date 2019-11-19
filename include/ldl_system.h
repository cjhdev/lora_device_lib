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

#ifndef LDL_SYSTEM_H
#define LDL_SYSTEM_H

/** @file */

/**
 * @defgroup ldl_system System
 * @ingroup ldl
 * 
 * # System Interface
 * 
 * System interfaces connect LDL to the underlying system. 
 * 
 * The following *MUST* be implemented:
 * 
 * - LDL_System_ticks()
 * - LDL_System_tps()
 * - LDL_System_eps()
 * 
 * The following have weak implementations which *MAY* be
 * re-implemented:
 * 
 * - LDL_System_advance()
 * - LDL_System_getBatteryLevel()
 * - LDL_System_rand()
 * 
 * The following macros *MUST* be defined if LDL_Radio_interrupt() or LDL_MAC_ticksUntilNextEvent() are called from an interrupt:
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
#include <stddef.h>

/** This function reads a free-running 32 bit counter. The counter
 * is expected to increment at a rate of LDL_System_tps() ticks per second.
 * 
 * LDL uses this changing value to track of the passage of time and perform
 * scheduling. 
 * 
 * @note this may be called from mainloop or interrupt
 * 
 * @param[in]   app     from LDL_MAC_init()
 * 
 * @return ticks
 * 
 * */
uint32_t LDL_System_ticks(void *app);

/** This function returns the rate (ticks per second) at which the value
 * returned by LDL_System_ticks() increments.
 * 
 * The rate MUST be in the range 10KHz to 1MHz.
 * 
 * @return ticks per second
 * 
 * */
uint32_t LDL_System_tps(void);

/** XTAL uncertainty per second in ticks
 * 
 * For example, if a 1MHz ticker is accurate to +/0.1% of nominal:
 * 
 * @code{.c}
 * uint32_t LDL_System_eps(void)
 * {
 *     return 1000000UL / 1000UL;
 * }
 * @endcode
 * 
 * @return xtal uncertainty in ticks
 * 
 * */
uint32_t LDL_System_eps(void);

/** LDL uses this function to select channels and to add random dither 
 * to scheduled events.
 * 
 * @retval (0..255)
 * 
 * 
 * LDL includes the following weak implementation:
 * @snippet src/ldl_system.c LDL_System_rand
 * 
 * */
uint8_t LDL_System_rand(void *app);

/** LDL uses this function to discover the battery level for for device
 * status MAC command.
 *
 * @param[in]   app     from LDL_MAC_init()
 * 
 * @return      DevStatusAns.battery 
 * @retval      255 not implemented
 * 
 *  
 * LDL includes the following weak implementation:
 * @snippet src/ldl_system.c LDL_System_getBatteryLevel
 * 
 * */
uint8_t LDL_System_getBatteryLevel(void *app);

/** Advance schedule by this many ticks to compensate for delay
 * between detecting an event and processing it.
 * 
 * @retval ticks
 * 
 * 
 * LDL includes the following weak implementation:
 * @snippet src/ldl_system.c LDL_System_advance
 * 
 * */
uint32_t LDL_System_advance(void);

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
 * @param[in] APP   app from LDL_MAC_init()   
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
 * @param[in] APP   app from LDL_MAC_init()
 * 
 * */
#define LDL_SYSTEM_LEAVE_CRITICAL(APP)
#endif

#ifdef __cplusplus
}
#endif


/** @} */
#endif
