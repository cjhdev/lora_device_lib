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

#include "lora_system.h"

#include <stdlib.h>
#include <string.h>

uint8_t LDL_System_rand(void) __attribute__((weak));
uint8_t LDL_System_getBatteryLevel(void *app) __attribute__((weak));
bool LDL_System_restoreContext(void *app, struct lora_mac_session *value) __attribute__((weak));
void LDL_System_saveContext(void *app, const struct lora_mac_session *value) __attribute__((weak));
void LDL_System_getIdentity(void *app, struct lora_system_identity *value) __attribute__((weak));
uint32_t LDL_System_time(void) __attribute__((weak));
uint32_t LDL_System_tps(void) __attribute__((weak));
uint32_t LDL_System_eps(void) __attribute__((weak));
uint32_t LDL_System_advance(void) __attribute__((weak));

uint8_t LDL_System_rand(void)
{
    return (uint8_t)rand();
}

uint8_t LDL_System_getBatteryLevel(void *app)
{
    return 255U;    /* not available */
}

bool LDL_System_restoreContext(void *app, struct lora_mac_session *value)
{
    return false;
}

void LDL_System_saveContext(void *app, const struct lora_mac_session *value)
{
}

void LDL_System_getIdentity(void *app, struct lora_system_identity *value)
{
    (void)memset(value, 0, sizeof(*value));    
}

uint32_t LDL_System_time(void)
{
    return 0UL;
}

uint32_t LDL_System_tps(void)
{
    return 32768UL;
}

uint32_t LDL_System_eps(void)
{
    return 0UL;
}

uint32_t LDL_System_advance(void)
{
    return 0UL;
}
