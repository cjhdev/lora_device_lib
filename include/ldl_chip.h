/* Copyright (c) 2019-2020 Cameron Harper
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
 * @defgroup ldl_chip_interface Chip Interface
 * @ingroup ldl
 *
 * # Chip Interface
 *
 * The Radio driver uses chip interface function pointers to control
 * the transceiver.
 *
 * ## SX1272 and SX1276
 *
 * The following connections are required:
 *
 * | signal | direction    | type                    | polarity    |
 * |--------|--------------|-------------------------|-------------|
 * | MOSI   | input        | hiz                     |             |
 * | MISO   | output       | push-pull/hiz           |             |
 * | SCK    | input        | hiz                     |             |
 * | NSS    | input        | hiz                     | active-low  |
 * | Reset  | input/output | hiz/pull-down           | active-high |
 * | DIO0   | output       | push-pull               | active-high |
 * | DIO1   | output       | push-pull               | active-high |
 *
 * - Direction is from perspective of transceiver
 * - SPI mode is CPOL=0 and CPHA=0
 * - consider adding pullup to MISO to prevent floating when not selected
 *
 * ### ldl_chip_set_mode_fn
 *
 * Should manipulate Reset and accessory IO lines like so:
 *
 * @include examples/chip_interface/set_mode_example.c
 *
 * ### ldl_chip_set_read_fn
 *
 * Should manipulate the chip select line and SPI like this:
 *
 * @include examples/chip_interface/read_example.c
 *
 * ### ldl_chip_set_write_fn
 *
 * Should manipulate the chip select line and SPI like this:
 *
 * @include examples/chip_interface/write_example.c
 *
 * ### LDL_Radio_interrupt()
 *
 * The chip interface code needs to be able to detect the rising edge and call
 * LDL_Radio_handleInterrupt().
 *
 * The code might look like this:
 *
 * @code{.c}
 * extern ldl_radio radio;
 *
 * void dio0_rising_edge_isr(void)
 * {
 *   LDL_Radio_handleInterrupt(&radio, 0);
 * }
 * void dio1_rising_edge_isr(void)
 * {
 *   LDL_Radio_handleInterrupt(&radio, 1);
 * }
 * @endcode
 *
 * @{
 * */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/** chip mode tells the chip interface code what to do
 * with various IO lines.
 *
 * */
enum ldl_chip_mode {

    LDL_CHIP_MODE_RESET,        /**< assert reset line */
    LDL_CHIP_MODE_SLEEP,        /**< oscillator is off */
    LDL_CHIP_MODE_STANDBY,      /**< oscillator is on */
    LDL_CHIP_MODE_RX,           /**< receiving */
    LDL_CHIP_MODE_TX_RFO,       /**< transmit using RFO PA */
    LDL_CHIP_MODE_TX_BOOST      /**< transmit using BOOST PA */
};

/** Use this function to configure the transceiver and associated
 * hardware.
 *
 * @param[in] self  #ldl_radio_init_arg.chip
 * @param[in] mode  #ldl_chip_mode
 *
 * @include examples/chip_interface/set_mode_example.c
 *
 * */
typedef void (*ldl_chip_set_mode_fn)(void *self, enum ldl_chip_mode mode);

/** Write bytes to address
 *
 * @param[in] self      chip from LDL_Radio_init()
 * @param[in] addr      register address
 * @param[in] data      buffer to write
 * @param[in] size      size of buffer in bytes
 *
 * This function must handle chip selection, addressing, and transferring zero
 * or more bytes. For example:
 *
 * @include examples/chip_interface/write_example.c
 *
 * */
typedef void (*ldl_chip_write_fn)(void *self, uint8_t addr, const void *data, uint8_t size);

/** Read bytes from address
 *
 * @param[in] self      board from LDL_Radio_init()
 * @param[in] addr      register address
 * @param[out] data     read into this buffer
 * @param[in] size      number of bytes to read (and size of buffer in bytes)
 *
 * This function must handle chip selection, addressing, and transferring zero
 * or more bytes. For example:
 *
 * @include examples/chip_interface/read_example.c
 *
 * */
typedef void (*ldl_chip_read_fn)(void *self, uint8_t addr, void *data, uint8_t size);

#ifdef __cplusplus
}
#endif

/** @} */
#endif
