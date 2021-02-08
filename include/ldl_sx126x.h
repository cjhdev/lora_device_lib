/* Copyright (c) 2021 Cameron Harper
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

#ifndef LDL_SX126X_H
#define LDL_SX126X_H

/** @file */

#include "ldl_radio.h"

/** @addtogroup ldl_radio
 * @{
 * */

/** init arguments for SX1261 and SX1262 radio */
struct ldl_sx126x_init_arg {

    /** type of XTAL */
    enum ldl_radio_xtal xtal;

    /** Apply TX gain compensation in units of (dB x 10-2)
     *
     * (e.g. -2.4dB == -240)
     *
     * */
    int16_t tx_gain;

    /** a pointer that will be passed as self when calling ldl_chip_*
     * functions */
    void *chip;

    ldl_chip_write_fn chip_write;       /**< #ldl_chip_write_fn */
    ldl_chip_read_fn chip_read;         /**< #ldl_chip_read_fn */
    ldl_chip_set_mode_fn chip_set_mode; /**< #ldl_chip_set_mode_fn */

    /** choose regulator hardware is configured for */
    enum ldl_sx126x_regulator regulator;

    /** choose tcxo voltage setting */
    enum ldl_sx126x_voltage voltage;

    /** choose txen switch mode
     *
     * note. this setting is ignored by WL55 driver
     *
     *  */
    enum ldl_sx126x_txen txen;

    /** The WL55 has HP and LP power amplifiers, this setting
     * tells the driver which to use. Auto means the driver
     * will select the PA according to requested power.
     *
     * note. this setting is ignored by SX1261 and SX1262 drivers.
     *
     * */
    enum ldl_sx126x_pa pa;

    bool trim_xtal;     /**< set true to trim xtal with xta and xtb parameters */
    uint8_t xta;        /**< XTA trim value */
    uint8_t xtb;        /**< XTB trim value */
};

/** Initialise SX1261 radio driver
 *
 * This is a SX126x with LP power amplifier.
 *
 * @param[in] self
 * @param[in] arg   #ldl_sx126x_init_arg
 *
 * */
void LDL_SX1261_init(struct ldl_radio *self, const struct ldl_sx126x_init_arg *arg);

/** Initialise SX1262 radio driver
 *
 * This is a SX126x with HP power amplifier.
 *
 * @param[in] self
 * @param[in] arg   #ldl_sx126x_init_arg
 *
 * */
void LDL_SX1262_init(struct ldl_radio *self, const struct ldl_sx126x_init_arg *arg);

/** Initialise WL55 radio driver
 *
 * This is a SX126x with:
 *
 * - HP and LP power amplifiers
 * - dio2 switching disabled
 *
 * @param[in] self
 * @param[in] arg   #ldl_sx126x_init_arg
 *
 * */
void LDL_WL55_init(struct ldl_radio *self, const struct ldl_sx126x_init_arg *arg);

/** Get radio interface
 *
 * @return #ldl_radio_interface
 *
 * */
const struct ldl_radio_interface *LDL_SX1261_getInterface(void);

/** Get radio interface
 *
 * @return #ldl_radio_interface
 *
 * */
const struct ldl_radio_interface *LDL_SX1262_getInterface(void);

/** Get radio interface
 *
 * @return #ldl_radio_interface
 *
 * */
const struct ldl_radio_interface *LDL_WL55_getInterface(void);

void LDL_SX126X_setMode(struct ldl_radio *self, enum ldl_radio_mode mode);
void LDL_SX126X_transmit(struct ldl_radio *self, const struct ldl_radio_tx_setting *settings, const void *data, uint8_t len);
void LDL_SX126X_receive(struct ldl_radio *self, const struct ldl_radio_rx_setting *settings);
uint8_t LDL_SX126X_readBuffer(struct ldl_radio *self, struct ldl_radio_packet_metadata *meta, void *data, uint8_t max);
void LDL_SX126X_receiveEntropy(struct ldl_radio *self);
uint32_t LDL_SX126X_readEntropy(struct ldl_radio *self);
void LDL_SX126X_getStatus(struct ldl_radio *self, struct ldl_radio_status *status);
uint32_t LDL_SX126X_getXTALDelay(struct ldl_radio *self);

/** @} */
#endif
