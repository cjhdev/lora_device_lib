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

#ifndef LDL_STREAM_H
#define LDL_STREAM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

struct ldl_stream {
  
    uint8_t *write;
    const uint8_t *read;
    uint8_t size;    
    uint8_t pos;    
    bool error;
};

void LDL_Stream_init(struct ldl_stream *self, void *buf, uint8_t size);
void LDL_Stream_initReadOnly(struct ldl_stream *self, const void *buf, uint8_t size);
bool LDL_Stream_read(struct ldl_stream *self, void *buf, uint8_t count);
bool LDL_Stream_write(struct ldl_stream *self, const void *buf, uint8_t count);
uint8_t LDL_Stream_tell(const struct ldl_stream *self);
uint8_t LDL_Stream_remaining(const struct ldl_stream *self);
bool LDL_Stream_peek(const struct ldl_stream *self, void *out);
bool LDL_Stream_seekSet(struct ldl_stream *self, uint8_t offset);
bool LDL_Stream_seekCur(struct ldl_stream *self, int16_t offset);
bool LDL_Stream_error(struct ldl_stream *self);

bool LDL_Stream_putU8(struct ldl_stream *self, uint8_t value);
bool LDL_Stream_putU16(struct ldl_stream *self, uint16_t value);
bool LDL_Stream_putU24(struct ldl_stream *self, uint32_t value);
bool LDL_Stream_putU32(struct ldl_stream *self, uint32_t value);
bool LDL_Stream_putEUI(struct ldl_stream *self, const uint8_t *value);

bool LDL_Stream_getU8(struct ldl_stream *self, uint8_t *value);
bool LDL_Stream_getU16(struct ldl_stream *self, uint16_t *value);
bool LDL_Stream_getU24(struct ldl_stream *self, uint32_t *value);
bool LDL_Stream_getU32(struct ldl_stream *self, uint32_t *value);
bool LDL_Stream_getEUI(struct ldl_stream *self, uint8_t *value);

#ifdef __cplusplus
}
#endif

#endif
