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

#ifndef LDL_CHIP_H
#define LDL_CHIP_H

/** @file */

/**
 * @defgroup ldl_radio_connector Radio Connector
 * @ingroup ldl
 * 
 * # Radio Connector
 * 
 * The Radio Connector connects the @ref ldl_radio driver to the transceiver digital interface.
 * 
 * The following interfaces MUST be implemented:
 * 
 * - LDL_Chip_read()
 * - LDL_Chip_write()
 * - LDL_Chip_select()
 * - LDL_Chip_reset()
 * 
 * The following interface MUST be called when the radio signals an interrupt:
 * 
 * - LDL_Radio_interrupt()
 * 
 * The @ref ldl_radio_connector must be implemented in such a way as to ensure that LDL_Radio_interrupt()
 * will not be called before LDL_MAC_init() has been performed.
 * 
 * ## Transceiver Digital Interface
 * 
 * ### SX1272 and SX1276
 * 
 * The following connections are required:
 * 
 * | signal | direction    | type                    | polarity    |
 * |--------|--------------|-------------------------|-------------|
 * | MOSI   | input        | hiz                     |             |
 * | MISO   | output       | push-pull/hiz           |             |
 * | SCK    | input        | hiz                     |             |
 * | NSS    | input        | hiz                     | active-low  |
 * | Reset  | input/output | open-collector + pullup | active-low  |
 * | DIO0   | output       | push-pull               | active-high |
 * | DIO1   | output       | push-pull               | active-high |
 * | DIO2   | output       | push-pull               | active-high |
 * | DIO3   | output       | push-pull               | active-high | 
 * 
 * - Direction is from perspective of transceiver
 * - SPI mode is CPOL=0 and CPHA=0
 * - consider adding pullup to MISO to prevent floating when not selected
 * - LDL_Chip_reset() should be implemented like this:
 * 
 * @code{.c}
 * void LDL_Chip_reset(void *self, bool state)
 * {
 *      if(state){
 * 
 *          // pull down
 *      }
 *      else{
 * 
 *          // hiz
 *      }
 * }
 * @endcode
 * 
 * - DIOx become active to indicate events and stay active until they are cleared by LDL
 *  - The radio connector needs to detect the rising edge and call LDL_MAC_interrupt() with the line number as argument
 *  - Edge detection can be by interrupt or polling
 *  - If interrupt is used, LDL_SYSTEM_ENTER_CRITICAL() and LDL_SYSTEM_LEAVE_CRITICAL() must be defined
 * - DIOx interrupt example:
 * 
 * @code{.c}
 * extern ldl_mac mac;
 * 
 * void dio0_rising_edge_isr(void)
 * {
 *   LDL_Radio_interrupt(&mac, 0);
 * }
 * void dio1_rising_edge_isr(void)
 * {
 *   LDL_Radio_interrupt(&mac, 1);
 * }
 * void dio2_rising_edge_isr(void)
 * {
 *   LDL_Radio_interrupt(&mac, 2);
 * }
 * void dio3_rising_edge_isr(void)
 * {
 *   LDL_Radio_interrupt(&mac, 3);
 * }
 * @endcode
 * 
 * 
 * @{
 * */

#ifdef __cplusplus
extern "C" {
#endif

#include "ldl_platform.h"
#include <stdint.h>
#include <stdbool.h>

/** Operate select line
 * 
 * @param[in] self      board from LDL_Radio_init()
 * @param[in] state     **true** for hold, **false** for release
 * 
 * */
void LDL_Chip_select(void *self, bool state);

/** Operate reset line
 * 
 * @param[in] self      board from LDL_Radio_init()
 * @param[in] state     **true** for hold, **true** for release
 * 
 * */
void LDL_Chip_reset(void *self, bool state);

/** Write byte
 * 
 * @param[in] self      board from LDL_Radio_init()
 * @param[in] data
 * 
 * */
void LDL_Chip_write(void *self, uint8_t data);

/** Read byte
 * 
 * @param[in] self      board from LDL_Radio_init()
 * @return data
 * 
 * */
uint8_t LDL_Chip_read(void *self);

#ifdef __cplusplus
}
#endif

/** @} */

#endif
