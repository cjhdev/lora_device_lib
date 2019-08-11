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

#include "lora_board.h"
#include "lora_debug.h"

#include <string.h>

void LDL_Board_init(struct lora_board *self, 
    void *receiver, 
    void (*select)(void *receiver, bool state),
    void (*reset)(void *receiver, bool state),
    void (*write)(void *receiver, uint8_t data),
    uint8_t (*read)(void *receiver)
){    
    LORA_PEDANTIC(self != NULL)
    
    // the following are mandatory
    LORA_PEDANTIC(select != NULL)
    LORA_PEDANTIC(reset != NULL)
    LORA_PEDANTIC(write != NULL)
    LORA_PEDANTIC(read != NULL)
    
    (void)memset(self, 0, sizeof(*self));
    
    self->receiver = receiver;
    self->select = select;
    self->reset = reset;
    self->write = write;
    self->read = read;
}
