/* Copyright (c) 2020 Cameron Harper
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

#ifndef MBED_LDL_PORT_H
#define MBED_LDL_PORT_H

#include "mbed_assert.h"
#include "mbed_trace.h"
#include "mbed_critical.h"

#define TRACE_GROUP "LDL"

#define LDL_ASSERT(X) MBED_ASSERT(X);

#define LDL_DEBUG(APP, ...) do{\
    tr_debug(__VA_ARGS__);\
}while(0);

#define LDL_ERROR(APP, ...) do{\
    tr_error( __VA_ARGS__);\
}while(0);

#ifdef __cplusplus
extern "C" {
#endif

void my_trace_begin(void);
void my_trace_part(const char *fmt, ...);
void my_trace_hex(const uint8_t *ptr, size_t len);
void my_trace_bitstring(const uint8_t *ptr, size_t len);
void my_trace_end(void);

#ifdef __cplusplus
}
#endif

#define LDL_TRACE_BEGIN() my_trace_begin();
#define LDL_TRACE_PART(...) my_trace_part(__VA_ARGS__);
#define LDL_TRACE_HEX(PTR, LEN) my_trace_hex(PTR, LEN);
#define LDL_TRACE_BIT_STRING(PTR, LEN) my_trace_bitstring(PTR, LEN);
#define LDL_TRACE_FINAL() my_trace_end();

#define LDL_SYSTEM_ENTER_CRITICAL(APP)  core_util_critical_section_enter();
#define LDL_SYSTEM_LEAVE_CRITICAL(APP)  core_util_critical_section_exit();

#endif
