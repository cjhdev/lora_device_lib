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

#ifndef MBED_LDL_SPI_RADIO_H
#define MBED_LDL_SPI_RADIO_H

#include "radio.h"

namespace LDL {

    class SPIRadio : public Radio {

        protected:

            SPI& spi;
            DigitalOut nss;
            DigitalIn busy;
            LowPowerTimer timer;

            struct ldl_radio radio;
            const struct ldl_radio_interface *internal_if;

            static bool _chip_write(void *self, const void *opcode, size_t opcode_size, const void *data, size_t size);
            static bool _chip_read(void *self, const void *opcode, size_t opcode_size, void *data, size_t size);

            static SPIRadio * to_radio(void *self);

            static uint32_t _read_entropy(struct ldl_radio *self);
            static uint8_t _read_buffer(struct ldl_radio *self, struct ldl_radio_packet_metadata *meta, void *data, uint8_t max);
            static void _transmit(struct ldl_radio *self, const struct ldl_radio_tx_setting *settings, const void *data, uint8_t len);
            static void _receive(struct ldl_radio *self, const struct ldl_radio_rx_setting *settings);
            static void _interrupt_handler(struct ldl_mac *self);
            static void _set_mode(struct ldl_radio *self, enum ldl_radio_mode mode);
            static void _receive_entropy(struct ldl_radio *self);
            static void _get_status(struct ldl_radio *self, struct ldl_radio_status *status);

            void chip_select(bool state);

            bool chip_write(const void *opcode, size_t opcode_size, const void *data, size_t size);
            bool chip_read(const void *opcode, size_t opcode_size, void *data, size_t size);

        public:

            static const struct ldl_radio_interface _interface;

            SPIRadio(
                SPI& spi,
                PinName nss,
                PinName busy
            );

            uint32_t read_entropy();
            uint8_t read_buffer(struct ldl_radio_packet_metadata *meta, void *data, uint8_t max);
            void transmit(const struct ldl_radio_tx_setting *settings, const void *data, uint8_t len);
            void receive(const struct ldl_radio_rx_setting *settings);
            void interrupt_handler();
            void set_mode(enum ldl_radio_mode mode);
            void receive_entropy();
            void get_status(struct ldl_radio_status *status);

            const struct ldl_radio_interface *get_interface();
    };

};

#endif
