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

#ifndef MBED_LDL_HW_SX126XMB2XAS_H
#define MBED_LDL_HW_SX126XMB2XAS_H

#include "sx126x.h"

namespace LDL {

    namespace HW {

        /**
         * SX126XMB2XAS series shields.
         *
         * This shield has IO lines that indicate:
         *
         * - SX1261 or SX1262
         * - crystal or tcxo
         *
         * */
        class SX126XMB2XAS : public Radio {

            protected:

                DigitalIn device_sel;
                DigitalIn xtal_sel;

                SPI spi;
                SX126X radio;

                void handle_event()
                {
                    _interrupt_handler((struct ldl_mac *)this);
                }

            public:

                SX126XMB2XAS(
                    int16_t tx_gain = 0
                )
                    :
                    Radio(),
                    device_sel(A2),
                    xtal_sel(A3),
                    spi(D11, D12, D13),
                    radio(
                        (device_sel.read() == 1) ? LDL_RADIO_SX1261 : LDL_RADIO_SX1262,
                        spi,
                        D7,             // nss
                        A0,             // reset
                        D3,             // busy
                        D5, NC, NC,     // DIO1, DIO2, DIO3
                        tx_gain,
                        LDL_SX126X_REGULATOR_DCDC,
                        LDL_SX126X_TXEN_ENABLED,
                        LDL_SX126X_VOLTAGE_1V7,
                        (xtal_sel.read() == 1) ? LDL_RADIO_XTAL_CRYSTAL : LDL_RADIO_XTAL_TCXO
                    )
                {
                    radio.set_event_handler(callback(this, &SX126XMB2XAS::handle_event));
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
