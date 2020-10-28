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

#ifndef MBED_LDL_CMWX1ZZABZ_H
#define MBED_LDL_CMWX1ZZABZ_H

#include "sx1276.h"

namespace LDL {

    /**
     * A module that aggregates:
     *
     * - SX1276
     * - TCXO with power switch
     * - switches for engaging RFI, RFO, and BOOST
     * - dedicated SPI peripheral
     *
     * Inherits radio so that it can be passed to the MAC.
     *
     * */
    class CMWX1ZZABZ : public Radio {

        protected:

            SPI spi;

            DigitalOut enable_boost;
            DigitalOut enable_rfo;
            DigitalOut enable_rfi;
            DigitalInOut enable_tcxo;

            SX1276 radio;

            void set_mode(enum ldl_chip_mode mode);

            void handle_event(enum ldl_radio_event event);

        public:

            CMWX1ZZABZ(
                PinName enable_tcxo,
                int16_t tx_gain = 0
            );

            const struct ldl_radio_interface *get_interface();
            Radio *get_state();
    };
};

#endif
