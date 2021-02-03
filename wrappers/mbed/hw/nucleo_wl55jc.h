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

#ifndef MBED_LDL_HW_NUCLEO_WL55JC_H
#define MBED_LDL_HW_NUCLEO_WL55JC_H

#ifdef STM32WL55xx

#include "wl55.h"

namespace LDL {

    namespace HW {

        /**
         * NUCLEO-WL55JC development kit
         *
         * */
        class NucleoWL55JC : public WL55 {

            public:

                NucleoWL55JC(
                    int16_t tx_gain = 0
                )
                    :
                    WL55(
                        tx_gain,
                        LDL_SX126X_REGULATOR_DCDC,
                        LDL_SX126X_TXEN_ENABLED,
                        LDL_SX126X_VOLTAGE_1V7,
                        LDL_RADIO_XTAL_TCXO
                    )
                {
                }
        };
    };
};

#endif

#endif
