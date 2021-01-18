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

#ifndef MBED_LDL_SX127X_H
#define MBED_LDL_SX127X_H

#include "spi_radio.h"

namespace LDL {

    class SX127X : public SPIRadio {

        protected:

            DigitalInOut reset;
            InterruptIn dio0;
            InterruptIn dio1;

            Callback<void(enum ldl_chip_mode)> chip_mode_cb;

            static void _chip_set_mode(void *self, enum ldl_chip_mode mode);
            void chip_set_mode(enum ldl_chip_mode mode);

            void dio0_handler();
            void dio1_handler();

        public:

            SX127X(
                enum ldl_radio_type type,
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
                Callback<void(enum ldl_chip_mode)> = nullptr
            );
    };

};

#endif
