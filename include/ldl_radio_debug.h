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

#ifndef LDL_RADIO_DEBUG_H
#define LDL_RADIO_DEBUG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifndef LDL_RADIO_DEBUG_LOG_SIZE
#define LDL_RADIO_DEBUG_LOG_SIZE 40
#endif

struct ldl_radio;

struct ldl_radio_debug_log_entry {

    uint8_t opcode;
    uint8_t value;
    uint8_t size;
};

struct ldl_radio_debug_log {

    struct ldl_radio_debug_log_entry log[LDL_RADIO_DEBUG_LOG_SIZE];
    uint8_t pos;
    bool overflow;
};

void LDL_Radio_debugLogReset(struct ldl_radio *self);
void LDL_Radio_debugLogPush(struct ldl_radio *self, uint8_t opcode, const uint8_t *data, size_t size);
void LDL_Radio_debugLogFlush(struct ldl_radio *self, const char *fn);

#ifdef __cplusplus
}
#endif

#endif
