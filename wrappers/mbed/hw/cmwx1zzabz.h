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

#ifndef MBED_LDL_HW_CMWX1ZZABZ_H
#define MBED_LDL_HW_CMWX1ZZABZ_H

#include "sx1276.h"

namespace LDL {

    namespace HW {

        /**
         * The CMWX1ZZABZ module aggregates:
         *
         * - SX1276
         * - TCXO with power switch
         * - switches for engaging RFI, RFO, and BOOST
         * - dedicated SPI peripheral
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

                void set_mode(enum ldl_chip_mode mode)
                {
                    /* we don't manpulate reset since that is the
                     * domain of the radio driver and not of the module */
                    switch(mode){
                    case LDL_CHIP_MODE_RESET:
                    case LDL_CHIP_MODE_SLEEP:
                        enable_rfi = 0;
                        enable_rfo = 0;
                        enable_boost = 0;
                        enable_tcxo.input();
                        break;
                    case LDL_CHIP_MODE_STANDBY:
                        enable_rfi = 0;
                        enable_rfo = 0;
                        enable_boost = 0;
                        enable_tcxo.output();
                        break;
                    case LDL_CHIP_MODE_RX:
                        enable_rfi = 1;
                        enable_rfo = 0;
                        enable_boost = 0;
                        break;
                    case LDL_CHIP_MODE_TX_BOOST:
                        enable_rfi = 0;
                        enable_rfo = 0;
                        enable_boost = 1;
                        break;
                    case LDL_CHIP_MODE_TX_RFO:
                        enable_rfi = 0;
                        enable_rfo = 1;
                        enable_boost = 0;
                        break;
                    }
                }

                void handle_event()
                {
                    _interrupt_handler((struct ldl_mac *)this);
                }

            public:

                /* PA_12 is where the reference design puts the TCXO on/off
                 * control line, and also where you will find it on the LRWAN
                 * kit.
                 *
                 * If you want to use USB this needs to move to another pin.
                 *
                 * */
                CMWX1ZZABZ(
                    PinName enable_tcxo = PA_12,
                    int16_t tx_gain = 0
                )
                    :
                    Radio(),
                    spi(PA_7, PA_6, PB_3),
                    enable_boost(PC_1, 0),
                    enable_rfo(PC_2, 0),
                    enable_rfi(PA_1, 0),
                    enable_tcxo(enable_tcxo, PIN_INPUT, PullNone, 1),
                    radio(
                        spi,
                        PA_15,                      // nss
                        PC_0,                       // reset
                        PB_4, PB_1, NC, NC, NC, NC, // DIO0, DIO1, DIO2, DIO3, DIO4, DIO5
                        LDL_SX127X_PA_AUTO,
                        tx_gain,
                        LDL_RADIO_XTAL_TCXO,
                        callback(this, &CMWX1ZZABZ::set_mode)
                    )
                {
                    /* ensure that radio events can make it out of the real driver */
                    radio.set_event_handler(callback(this, &CMWX1ZZABZ::handle_event));
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
