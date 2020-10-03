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

#ifndef MBED_LDL_RADIO_H
#define MBED_LDL_RADIO_H

#include "mbed.h"
#include "ldl_radio.h"
#include "ldl_system.h"

namespace LDL {

    /** Abstract radio driver
     *
     * Users should instantiate one of the concrete subclasses named for the
     * radio transceiver they want to drive.
     *
     * Users can choose to subclass one of the concrete subclasses
     * in order to correctly drive any accessory IO lines that
     * are needed to make the transceiver work properly.
     *
     * There is only one virtual method (chip_set_mode()). This method
     * exists to manipulate accessory IO lines according to the radio mode.
     *
     * The CMWX1ZZABZ subclass is a good example of how to manipulate
     * accessory IO lines.
     *
     * */
    class Radio {

        protected:

            SPI& spi;
            DigitalOut nss;
            DigitalInOut reset;
            InterruptIn dio0;
            InterruptIn dio1;
            Callback<void(enum ldl_radio_event)> event_cb;

            struct ldl_radio radio;

            static Radio *to_obj(void *self);
            static struct ldl_radio *to_state(void *self);

            static unsigned int _read_entropy(struct ldl_radio *self);
            static uint8_t _read_buffer(struct ldl_radio *self, struct ldl_radio_packet_metadata *meta, void *data, uint8_t max);
            static void _transmit(struct ldl_radio *self, const struct ldl_radio_tx_setting *settings, const void *data, uint8_t len);
            static void _receive(struct ldl_radio *self, const struct ldl_radio_rx_setting *settings);
            static void _interrupt_handler(void *self, enum ldl_radio_event event);
            static uint8_t _get_xtal_delay(struct ldl_radio *self);
            static void _set_mode(struct ldl_radio *self, enum ldl_radio_mode mode);
            static void _receive_entropy(struct ldl_radio *self);

            static void _chip_set_mode(void *self, enum ldl_chip_mode mode);
            static void _chip_write(void *self, uint8_t opcode, const void *data, uint8_t size);
            static void _chip_read(void *self, uint8_t opcode, void *data, uint8_t size);

            void dio0_handler();
            void dio1_handler();

            void chip_select(bool state);
            void chip_reset(bool state);
            void chip_write(uint8_t opcode, const void *data, uint8_t size);
            void chip_read(uint8_t opcode, void *data, uint8_t size);
            virtual void chip_set_mode(enum ldl_chip_mode mode) = 0;

        public:

            static const struct ldl_radio_interface interface;

            Radio(
                SPI& spi,
                PinName nss,
                PinName reset,
                PinName dio0,
                PinName dio1
            );

            void set_event_handler(Callback<void(enum ldl_radio_event)> handler);
    };

    class SX1272 : public Radio {

        protected:

            void chip_set_mode(enum ldl_chip_mode mode);

        public:

            SX1272(
                SPI& spi,
                PinName nss,
                PinName reset,
                PinName dio0,
                PinName dio1,
                enum ldl_radio_pa pa = LDL_RADIO_PA_RFO,
                int16_t tx_gain = 0,
                enum ldl_radio_xtal xtal = LDL_RADIO_XTAL_CRYSTAL,
                uint8_t xtal_delay = 1UL
            );
    };

    class SX1276 : public Radio {

        protected:

            void chip_set_mode(enum ldl_chip_mode mode);

        public:

            SX1276(
                SPI& spi,
                PinName nss,
                PinName reset,
                PinName dio0,
                PinName dio1,
                enum ldl_radio_pa pa = LDL_RADIO_PA_RFO,
                int16_t tx_gain = 0,
                enum ldl_radio_xtal xtal = LDL_RADIO_XTAL_CRYSTAL,
                uint8_t xtal_delay = 1UL
            );
    };

    /** An SX1276 module with a TCXO and RF switches for engaging RFI, RFO, and BOOST
     *
     * */
    class CMWX1ZZABZ : public SX1276 {

        protected:

            DigitalOut enable_boost;
            DigitalOut enable_rfo;
            DigitalOut enable_rfi;
            DigitalInOut enable_tcxo;

            void chip_set_mode(enum ldl_chip_mode mode);

        public:

            CMWX1ZZABZ(
                SPI& spi,
                PinName nss,
                PinName reset,
                PinName dio0,
                PinName dio1,

                PinName enable_boost,
                PinName enable_rfo,
                PinName enable_rfi,
                PinName enable_tcxo,

                int16_t tx_gain = 0
            );
    };

};

#endif
