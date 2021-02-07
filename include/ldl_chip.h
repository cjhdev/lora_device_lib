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
 *
 * The @ref ldl_radio uses chip interface function pointers to control
 * the hardware:
 *
 * - #ldl_chip_set_mode_fn
 * - #ldl_chip_write_fn
 * - #ldl_chip_read_fn
 *
 * Implementations of these functions must be assigned to the radio
 * driver during initialisation. Use the examples in the function pointer
 * documentation as a guide. These functions will need to interact with
 * IO connected to the radio chip.
 *
 * The Radio driver can receive an interrupt from the radio via the
 * LDL_Radio_handleInterrupt() function.
 *
 * The following connections are required for SX1272 and SX1276:
 *
 * | signal | direction on host    | type                    | polarity    |
 * |--------|--------------|-------------------------|-------------|
 * | MOSI   | output       | push-pull               |             |
 * | MISO   | input        | hiz with opt. pull-up   |             |
 * | SCK    | output       | push-pull               |             |
 * | NSS    | output       | push-pull               | active-low  |
 * | Reset  | input/output | hiz/push-pull           | active-high |
 * | DIO0   | input        | hiz with opt. pull-down | active-high |
 * | DIO1   | input        | hiz with opt. pull-down | active-high |
 *
 * The following connections are required for SX1261 and SX1262:
*
 * | signal | direction on host    | type                    | polarity    |
 * |--------|---------------|-------------------------|-------------|
 * | MOSI   | output        | push-pull               |             |
 * | MISO   | input         | hiz with opt. pull-up   |             |
 * | SCK    | output        | push-pull               |             |
 * | NSS    | output        | push-pull               | active-low  |
 * | Reset  | input/output  | hiz/push-pull           | active-low  |
 * | Busy   | input         | hiz                     | active-high |
 * | DIO1   | input         | hiz with opt. pull-down | active-high |
 *
 * **Note that SX126X reset is active-low, while SX127X reset is active-high.**
 *
 * @{
 * */

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdbool.h>

/** Chip mode tells the chip interface code what to do
 * with various IO lines.
 *
 * - WL55 and SX127X series have two PAs, one of which may need to be selected
 * - SX1261 and SX1262 have one PA
 * - TX_BOOST and TX_RFO are legacy names
 *   - TX_BOOST means high power
 *   - TX_RFO means low power
 *
 * */
enum ldl_chip_mode {

    LDL_CHIP_MODE_RESET,        /**< assert reset line */
    LDL_CHIP_MODE_SLEEP,        /**< oscillator is off */
    LDL_CHIP_MODE_STANDBY,      /**< oscillator is on */
    LDL_CHIP_MODE_RX,           /**< receiving */
    LDL_CHIP_MODE_TX_RFO,       /**< transmit using low power PA */
    LDL_CHIP_MODE_TX_BOOST      /**< transmit using high power PA */
};

/** Use this function to configure the transceiver and associated
 * hardware.
 *
 * @param[in] self
 * @param[in] mode  #ldl_chip_mode
 *
 * @include examples/chip_interface/set_mode_example.c
 *
 * */
typedef void (*ldl_chip_set_mode_fn)(void *self, enum ldl_chip_mode mode);

/**Write opcode and write payload
 *
 * @param[in] self
 * @param[in] opcode        opcode data that must be written
 * @param[in] opcode_size   size of opcode data
 * @param[in] data      buffer to write
 * @param[in] size      size of buffer in bytes
 *
 * @retval true     operation complete
 * @retval false    chip is still busy after timeout
 *
 * This function must handle:
 *
 * - busy polling & timeout (SX126X series)
 * - chip selection
 * - data transfer
 *
 * @include examples/chip_interface/write_example.c
 *
 * */
typedef bool (*ldl_chip_write_fn)(void *self, const void *opcode, size_t opcode_size, const void *data, size_t size);

/** Write opcode and read payload
 *
 * @param[in] self
 * @param[in] opcode    opcode data that must be written
 * @param[in] opcode_size size of opcode data
 * @param[out] data     read into this buffer
 * @param[in] size      number of bytes to read (and size of buffer in bytes)
 *
 * @retval true     operation complete
 * @retval false    chip is still busy after timeout
 *
 * This function must handle:
 *
 * - busy polling & timeout (SX126X series)
 * - chip selection
 * - data transfer
 *
 * For example:
 *
 * @include examples/chip_interface/read_example.c
 *
 *
 * */
typedef bool (*ldl_chip_read_fn)(void *self, const void *opcode, size_t opcode_size, void *data, size_t size);

#ifdef __cplusplus
}
#endif

/** @} */
#endif
