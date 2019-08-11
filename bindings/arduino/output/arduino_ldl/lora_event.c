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

#include "lora_event.h"
#include "lora_debug.h"
#include "lora_system.h"
#include <string.h>

/* static function prototypes *****************************************/

static uint32_t delta(uint32_t timeout, uint32_t time);

/* functions **********************************************************/

void LDL_Event_init(struct lora_event *self, void *app)
{
    LORA_PEDANTIC(self != NULL)
    (void)memset(self, 0, sizeof(*self));    
    self->app = app;
}

void LDL_Event_signal(struct lora_event *self, enum lora_event_input type, uint32_t time)
{
    LORA_PEDANTIC(self != NULL)
    
    LORA_SYSTEM_ENTER_CRITICAL(self->app)
    
    if((self->input.state == 0U) && (self->input.armed & (1U << type))){
    
        self->input.time = time;
        self->input.state = (1U << type);
    }
    
    LORA_SYSTEM_LEAVE_CRITICAL(self->app)
}

void LDL_Event_setInput(struct lora_event *self, enum lora_event_input type)
{
    LORA_PEDANTIC(self != NULL)    
    
    LORA_SYSTEM_ENTER_CRITICAL(self->app) 
    
    self->input.armed |= (1U << type);
    
    LORA_SYSTEM_LEAVE_CRITICAL(self->app)        
}

void LDL_Event_setTimer(struct lora_event *self, enum lora_event_timer timer, uint32_t timeout)
{
    LORA_PEDANTIC(self != NULL)
    LORA_ASSERT(timeout <= INT32_MAX)    
    
    LORA_SYSTEM_ENTER_CRITICAL(self->app) 
    
    self->timer[timer].time = LDL_System_time() + timeout;
    self->timer[timer].armed = true;
    
    LORA_SYSTEM_LEAVE_CRITICAL(self->app)    
}

bool LDL_Event_checkTimer(struct lora_event *self, enum lora_event_timer timer, uint32_t *error)
{
    LORA_PEDANTIC(self != NULL)
    LORA_PEDANTIC(error != NULL)
    
    bool retval = false;
    uint32_t time;
        
    LORA_SYSTEM_ENTER_CRITICAL(self->app)     
        
    if(self->timer[timer].armed){
        
        time = LDL_System_time();
        
        if(delta(self->timer[timer].time, time) < INT32_MAX){
    
            self->timer[timer].armed = false;            
            *error = delta(self->timer[timer].time, time);
            retval = true;
        }
    }    
    
    LORA_SYSTEM_LEAVE_CRITICAL(self->app)    
    
    return retval;
}

bool LDL_Event_checkInput(struct lora_event *self, enum lora_event_input type, uint32_t *error)
{
    bool retval = false;
    
    LORA_SYSTEM_ENTER_CRITICAL(self->app)     
    
    if((self->input.state & (1U << type)) > 0U){
        
        self->input.state = 0U;
        *error = delta(self->input.time, LDL_System_time());
        retval = true;
    }
    
    LORA_SYSTEM_LEAVE_CRITICAL(self->app)    
    
    return retval;
}

void LDL_Event_clearInput(struct lora_event *self)
{
    LORA_SYSTEM_ENTER_CRITICAL(self->app)     
    
    self->input.state = 0U;
    self->input.armed = 0U;
    
    LORA_SYSTEM_LEAVE_CRITICAL(self->app)    
}

void LDL_Event_clearTimer(struct lora_event *self, enum lora_event_timer timer)
{
    LORA_SYSTEM_ENTER_CRITICAL(self->app)     
    
    self->timer[timer].armed = false;
    
    LORA_SYSTEM_LEAVE_CRITICAL(self->app)    
}

uint32_t LDL_Event_ticksUntilNext(const struct lora_event *self)
{
    size_t i;
    uint32_t retval;
    uint32_t time;

    LORA_SYSTEM_ENTER_CRITICAL(self->app)     
    
    retval = (self->input.state > 0U) ? 0U : UINT32_MAX;
    
    LORA_SYSTEM_LEAVE_CRITICAL(self->app)    
    
    if(retval > 0U){
    
        time = LDL_System_time();
    
        for(i=0U; i < sizeof(self->timer)/sizeof(*self->timer); i++){

            LORA_SYSTEM_ENTER_CRITICAL(self->app)     

            if(self->timer[i].armed){
                
                if(delta(self->timer[i].time, time) <= INT32_MAX){
                    
                    retval = 0U;
                }
                else{
                    
                    if(delta(time, self->timer[i].time) < retval){
                        
                        retval = delta(time, self->timer[i].time);
                    }
                }
            }
            
            LORA_SYSTEM_LEAVE_CRITICAL(self->app)    
            
            if(retval == 0U){
                
                break;
            }
        }
    }
    
    return retval;
}

uint32_t LDL_Event_ticksUntil(const struct lora_event *self, enum lora_event_timer timer)
{
    uint32_t retval = UINT32_MAX;
    uint32_t time;
    
    if(self->timer[timer].armed){
        
        time = LDL_System_time();
        
        if(delta(self->timer[timer].time, time) <= INT32_MAX){
            
            retval = 0U;
        }
        else{
            
            retval = delta(time, self->timer[timer].time);
        }
    }
    
    return retval;
}

/* static functions ***************************************************/

static uint32_t delta(uint32_t timeout, uint32_t time)
{
    return (timeout <= time) ? (time - timeout) : (UINT32_MAX - timeout + time);
}
