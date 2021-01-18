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

#ifndef MBED_LDL_SX1261_H
#define MBED_LDL_SX1261_H

#include "sx126x.h"

namespace LDL {

    class SX1261 : public SX126X {

        public:

            SX1261(
                SPI& spi,
                PinName nss,
                PinName reset,
                PinName busy,
                PinName dio1,
                PinName dio2,
                PinName dio3,
                int16_t tx_gain = 0,
                enum ldl_sx126x_regulator regulator = LDL_SX126X_REGULATOR_LDO,
                enum ldl_sx126x_txen txen = LDL_SX126X_TXEN_ENABLED,
                enum ldl_sx126x_voltage voltage = LDL_SX126X_VOLTAGE_1V6,
                enum ldl_radio_xtal xtal = LDL_RADIO_XTAL_CRYSTAL,
                Callback<void(enum ldl_chip_mode)> chip_mode_cb = nullptr
            )
                :
                SX126X(
                    LDL_RADIO_SX1261,
                    spi,
                    nss,
                    reset,
                    busy,
                    dio1,
                    dio2,
                    dio3,
                    tx_gain,
                    regulator,
                    txen,
                    voltage,
                    xtal,
                    chip_mode_cb
                )
            {}
    };

};

#endif
