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

#ifndef MBED_LDL_SX1272_H
#define MBED_LDL_SX1272_H

#include "sx127x.h"

namespace LDL {

    class SX1272 : public SX127X {

        public:

            SX1272(
                SPI& spi,
                PinName nss,
                PinName reset,
                PinName dio0,
                PinName dio1,
                PinName dio2 = NC,
                PinName dio3 = NC,
                PinName dio4 = NC,
                PinName dio5 = NC,
                enum ldl_sx127x_pa pa = LDL_SX127X_PA_RFO,
                int16_t tx_gain = 0,
                enum ldl_radio_xtal xtal = LDL_RADIO_XTAL_CRYSTAL,
                Callback<void(enum ldl_chip_mode)> chip_mode_cb = nullptr
            )
                :
                SX127X(
                    LDL_RADIO_SX1272,
                    spi,
                    nss,
                    reset,
                    dio0,
                    dio1,
                    dio2,
                    dio3,
                    dio4,
                    dio5,
                    pa,
                    tx_gain,
                    xtal,
                    chip_mode_cb
                )
            {}
    };

};

#endif
