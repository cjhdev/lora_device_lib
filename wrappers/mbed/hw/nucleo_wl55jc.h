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
         * NUCLEO-WL55JC development kit in default configuration
         *
         * */
        class NucleoWL55JC : public Radio {

            private:

                DigitalOut fe_ctrl1;
                DigitalOut fe_ctrl2;
                DigitalOut fe_ctrl3;

                WL55 radio;

                void set_mode(enum ldl_chip_mode mode)
                {
                    switch(mode){
                    default:
                        break;
                    case LDL_CHIP_MODE_RESET:
                    case LDL_CHIP_MODE_SLEEP:
                    case LDL_CHIP_MODE_STANDBY:
                        fe_ctrl1 = 0;
                        fe_ctrl2 = 0;
                        fe_ctrl3 = 0;
                        break;
                    case LDL_CHIP_MODE_TX_BOOST:
                        fe_ctrl1 = 0;
                        fe_ctrl2 = 1;
                        fe_ctrl3 = 1;
                        break;
                    case LDL_CHIP_MODE_TX_RFO:
                        fe_ctrl1 = 1;
                        fe_ctrl2 = 1;
                        fe_ctrl3 = 1;
                        break;
                    case LDL_CHIP_MODE_RX:
                        fe_ctrl1 = 1;
                        fe_ctrl2 = 0;
                        fe_ctrl3 = 1;
                        break;
                    }
                }

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
                NucleoWL55JC(
                    int16_t tx_gain = 200
                )
                    :
                    Radio(),
                    fe_ctrl1(PC_4),
                    fe_ctrl2(PC_5),
                    fe_ctrl3(PC_3),
                    radio(
                        tx_gain,
                        LDL_SX126X_PA_AUTO,  // select based on requested power
                        LDL_SX126X_REGULATOR_DCDC,
                        LDL_SX126X_VOLTAGE_1V7,
                        LDL_RADIO_XTAL_TCXO,
                        callback(this, &NucleoWL55JC::set_mode)
                    )
                {
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

#endif
