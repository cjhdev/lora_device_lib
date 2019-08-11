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

#ifndef LORA_STREAM_H
#define LORA_STREAM_H

#include "lora_platform.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

struct lora_stream {
  
    uint8_t *write;
    const uint8_t *read;
    size_t size;    
    size_t pos;    
};

struct lora_stream * LDL_Stream_init(struct lora_stream *self, void *buf, size_t size);
struct lora_stream * LDL_Stream_initReadOnly(struct lora_stream *self, const void *buf, size_t size);
bool LDL_Stream_read(struct lora_stream *self, void *buf, size_t count);
bool LDL_Stream_write(struct lora_stream *self, const void *buf, size_t count);
size_t LDL_Stream_tell(const struct lora_stream *self);
size_t LDL_Stream_remaining(const struct lora_stream *self);
bool LDL_Stream_peek(const struct lora_stream *self, void *out);
bool LDL_Stream_seekSet(struct lora_stream *self, size_t offset);

#endif
