/* Copyright (c) 2021 Cameron Harper
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

#ifndef LDL_MAC_INTERNAL_H
#define LDL_MAC_INTERNAL_H

#include "ldl_platform.h"
#include <stdint.h>
#include <stdbool.h>

struct ldl_mac;

enum ldl_timer_inst {

    LDL_TIMER_WAITA,    /* general use + RX1 slot timing */
    LDL_TIMER_WAITB,    /* RX2 slot timing */
    LDL_TIMER_BAND,     /* tracks time used to decrement down counters */
#ifdef LDL_ENABLE_CLASS_B
    LDL_TIMER_BEACON,   /* beacon tracking */
#endif
    LDL_TIMER_MAX
};


bool LDL_MAC_addChannel(struct ldl_mac *self, uint8_t chIndex, uint32_t freq, uint8_t minRate, uint8_t maxRate);
void LDL_MAC_removeChannel(struct ldl_mac *self, uint8_t chIndex);
bool LDL_MAC_maskChannel(struct ldl_mac *self, uint8_t chIndex);
bool LDL_MAC_unmaskChannel(struct ldl_mac *self, uint8_t chIndex);

void LDL_MAC_timerSet(struct ldl_mac *self, enum ldl_timer_inst timer, uint32_t timeout);
void LDL_MAC_timerAppend(struct ldl_mac *self, enum ldl_timer_inst timer, uint32_t timeout);
bool LDL_MAC_timerCheck(struct ldl_mac *self, enum ldl_timer_inst timer, uint32_t *lag);
uint32_t LDL_MAC_timerTicksSince(struct ldl_mac *self, enum ldl_timer_inst timer);
void LDL_MAC_timerClear(struct ldl_mac *self, enum ldl_timer_inst timer);
uint32_t LDL_MAC_timerTicksUntilNext(const struct ldl_mac *self);
uint32_t LDL_MAC_timerTicksUntil(const struct ldl_mac *self, enum ldl_timer_inst timer, uint32_t *lag);

#endif
