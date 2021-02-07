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

#ifndef MBED_LDL_HW_SX1272MB2XAS_H
#define MBED_LDL_HW_SX1272MB2XAS_H

#include "sx1272.h"

namespace LDL {

    namespace HW {

        /**
         * SX1272MB2XAS shield
         *
         * */
        class SX1272MB2XAS : public Radio {

            protected:

                SPI spi;
                SX1272 radio;

                void handle_event()
                {
                    _interrupt_handler((struct ldl_mac *)this);
                }

            public:

                /** Create
                 *
                 * @param[in] tx_gain       antenna gain (dB x 100)
                 *
                 * */
                SX1272MB2XAS(
                    int16_t tx_gain = 200
                )
                    :
                    Radio(),
                    spi(D11, D12, D13),
                    radio(
                        spi,
                        D10,                      // nss
                        A0,                       // reset
                        D2, D3, D4, D5, NC, NC,   // DIO0, DIO1, DIO2, DIO3, DIO4, DIO5
                        LDL_SX127X_PA_RFO,
                        tx_gain
                    )
                {
                    radio.set_event_handler(callback(this, &SX1272MB2XAS::handle_event));
                }

                const struct ldl_radio_interface *get_interface()
                {
                    return radio.get_interface();
                }

                Radio *get_state()
                {
                    return radio.get_state();
                }
        };
    };
};

#endif
