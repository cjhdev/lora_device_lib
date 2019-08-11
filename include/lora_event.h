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

#ifndef LORA_EVENT_H
#define LORA_EVENT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lora_platform.h"
#include <stdint.h>
#include <stdbool.h>

enum lora_event_input {
  
    LORA_EVENT_INPUT_TX_COMPLETE,
    LORA_EVENT_INPUT_RX_READY,
    LORA_EVENT_INPUT_RX_TIMEOUT
};

enum lora_event_timer {
    
    LORA_EVENT_TIME,
    LORA_EVENT_WAITA,
    LORA_EVENT_WAITB,
    LORA_EVENT_BAND1,
    LORA_EVENT_BAND2,
    LORA_EVENT_BAND3,
    LORA_EVENT_BAND4,
    LORA_EVENT_BAND5,
    LORA_EVENT_BAND_COMBINED
};

struct lora_event_timer_state {
    
    volatile uint32_t time;    
    volatile bool armed;
};

struct lora_event_input_state {
    
    uint8_t armed;
    uint8_t state;
    uint32_t time;
};

struct lora_event {
   
    volatile struct lora_event_timer_state timer[LORA_EVENT_BAND_COMBINED + 1U];    
    volatile struct lora_event_input_state input;
    void *app;
};

void LDL_Event_init(struct lora_event *self, void *app);
void LDL_Event_setTimer(struct lora_event *self, enum lora_event_timer timer, uint32_t timeout);
void LDL_Event_clearTimer(struct lora_event *self, enum lora_event_timer timer);
bool LDL_Event_checkTimer(struct lora_event *self, enum lora_event_timer timer, uint32_t *error);
void LDL_Event_setInput(struct lora_event *self, enum lora_event_input type);
bool LDL_Event_checkInput(struct lora_event *self, enum lora_event_input type, uint32_t *error);
void LDL_Event_clearInput(struct lora_event *self);
void LDL_Event_signal(struct lora_event *self, enum lora_event_input type, uint32_t time);
uint32_t LDL_Event_ticksUntilNext(const struct lora_event *self);
uint32_t LDL_Event_ticksUntil(const struct lora_event *self, enum lora_event_timer timer);

#ifdef __cplusplus
}
#endif

#endif
