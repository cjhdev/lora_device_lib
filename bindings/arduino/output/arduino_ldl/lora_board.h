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

#ifndef LORA_BOARD_H
#define LORA_BOARD_H

/** @file */

/**
 * @defgroup ldl_board Board
 * @ingroup ldl
 * 
 * @{
 * */

#ifdef __cplusplus
extern "C" {
#endif

#include "lora_platform.h"
#include <stdint.h>
#include <stdbool.h>

/** Operate select line
 * 
 * @param[in] receiver
 * @param[in] state **TRUE** for hold, **FALSE** for release
 * 
 * */
typedef void (*lora_board_select_fn)(void *receiver, bool state);

/** Operate reset line
 * 
 * @param[in] receiver
 * @param[in] state **TRUE** for hold, **FALSE** for release
 * 
 * */
typedef void (*lora_board_reset_fn)(void *receiver, bool state);

/** Write to radio
 * 
 * @param[in] receiver
 * @param[in] data
 * 
 * */
typedef void (*lora_board_write_fn)(void *receiver, uint8_t data);

/** Read from radio
 * 
 * @param[in] receiver
 * @return data
 * 
 * */
typedef uint8_t (*lora_board_read_fn)(void *receiver);

/** Board specific connections via function pointers 
 * 
 * @see LDL_Board_init()
 * 
 * */
struct lora_board {
    
    void *receiver;                     /**< passed back as receiver argument */
    lora_board_select_fn select;        /**< select */
    lora_board_reset_fn reset;          /**< reset */
    lora_board_write_fn write;          /**< write */
    lora_board_read_fn read;            /**< read */
};

/** Optional helper function for ensuring #lora_board is correctly initialised
 * 
 * @param[in] self
 * 
 * @param[in] receiver
 *  passed as the receiver argument in all functions.
 * 
 * @param[in] select
 *  selects the radio
 * 
 * @param[in] reset
 *  sets the radio reset line
 * 
 * @param[in] write
 *  write byte to radio
 * 
 * @param[in] read
 *  read byte from radio
 * 
 * */
void LDL_Board_init(struct lora_board *self, 
    void *receiver, 
    lora_board_select_fn select,
    lora_board_reset_fn reset,
    lora_board_write_fn write,
    lora_board_read_fn read
);

/** @} */

#ifdef __cplusplus
}
#endif

#endif
