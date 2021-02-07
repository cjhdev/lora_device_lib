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

#ifndef LDL_SX127X_H
#define LDL_SX127X_H

/** @file */

#include "ldl_radio.h"

/** @addtogroup ldl_radio
 * @{
 * */

/** init arguments for SX1272 and SX1276 radio */
struct ldl_sx127x_init_arg {

    /** type of XTAL */
    enum ldl_radio_xtal xtal;

    /** Apply TX gain compensation in units of (dB x 100)
     *
     * (e.g. 2.4dB == 240)
     *
     * */
    int16_t tx_gain;

    /** a pointer that will be passed as self when calling ldl_chip_*
     * functions */
    void *chip;

    ldl_chip_write_fn chip_write;       /**< #ldl_chip_write_fn */
    ldl_chip_read_fn chip_read;         /**< #ldl_chip_read_fn */
    ldl_chip_set_mode_fn chip_set_mode; /**< #ldl_chip_set_mode_fn */

    /** SX1272/6 transceivers have PAs on different physical pins
     *
     * The driver needs to know if one or both are connected.
     *
     * */
    enum ldl_sx127x_pa pa;
};

/** Initialise SX1272 radio driver
 *
 * @param[in] self
 * @param[in] arg   #ldl_sx127x_init_arg
 *
 * */
void LDL_SX1272_init(struct ldl_radio *self, const struct ldl_sx127x_init_arg *arg);

/** Initialise SX1276 radio driver
 *
 * @param[in] self
 * @param[in] arg   #ldl_sx127x_init_arg
 *
 * */
void LDL_SX1276_init(struct ldl_radio *self, const struct ldl_sx127x_init_arg *arg);

/** Get radio interface
 *
 * @return #ldl_radio_interface
 *
 * */
const struct ldl_radio_interface *LDL_SX1272_getInterface(void);

/** Get radio interface
 *
 * @return #ldl_radio_interface
 *
 * */
const struct ldl_radio_interface *LDL_SX1276_getInterface(void);

void LDL_SX127X_setMode(struct ldl_radio *self, enum ldl_radio_mode mode);
void LDL_SX127X_transmit(struct ldl_radio *self, const struct ldl_radio_tx_setting *settings, const void *data, uint8_t len);
void LDL_SX127X_receive(struct ldl_radio *self, const struct ldl_radio_rx_setting *settings);
uint8_t LDL_SX127X_readBuffer(struct ldl_radio *self, struct ldl_radio_packet_metadata *meta, void *data, uint8_t max);
void LDL_SX127X_receiveEntropy(struct ldl_radio *self);
uint32_t LDL_SX127X_readEntropy(struct ldl_radio *self);
void LDL_SX127X_getStatus(struct ldl_radio *self, struct ldl_radio_status *status);
uint32_t LDL_SX127X_getXTALDelay(struct ldl_radio *self);

/** @} */
#endif
