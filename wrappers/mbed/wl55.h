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

#ifndef MBED_LDL_WL55_H
#define MBED_LDL_WL55_H

#ifdef STM32WL55xx

#include "radio.h"

namespace LDL {

    /**
     * STM32WL55x driver
     *
     * This is essentially an SX1262 driver with special IO handling.
     *
     * */
    class WL55 : public Radio {

        protected:

            LowPowerTimer timer;

            struct ldl_radio radio;
            const struct ldl_radio_interface *internal_if;

            Callback<void(enum ldl_chip_mode)> chip_mode_cb;

            static bool _chip_write(void *self, const void *opcode, size_t opcode_size, const void *data, size_t size);
            static bool _chip_read(void *self, const void *opcode, size_t opcode_size, void *data, size_t size);

            static WL55 * to_radio(void *self);

            static uint32_t _read_entropy(struct ldl_radio *self);
            static uint8_t _read_buffer(struct ldl_radio *self, struct ldl_radio_packet_metadata *meta, void *data, uint8_t max);
            static void _transmit(struct ldl_radio *self, const struct ldl_radio_tx_setting *settings, const void *data, uint8_t len);
            static void _receive(struct ldl_radio *self, const struct ldl_radio_rx_setting *settings);
            static void _interrupt_handler(struct ldl_mac *self);
            static void _set_mode(struct ldl_radio *self, enum ldl_radio_mode mode);
            static void _receive_entropy(struct ldl_radio *self);
            static void _get_status(struct ldl_radio *self, struct ldl_radio_status *status);

            static void _handle_irq(void);

            void chip_select(bool state);

            bool chip_write(const void *opcode, size_t opcode_size, const void *data, size_t size);
            bool chip_read(const void *opcode, size_t opcode_size, void *data, size_t size);
            void chip_set_mode(enum ldl_chip_mode mode);

            static void _chip_set_mode(void *self, enum ldl_chip_mode mode);

            /* it's absurd to have more than one instance but this
             * this list and mutex ensures MBED won't go mad if you do
             *
             * */
            WL55 *next_instance;
            static WL55 *instances;
            static Mutex lock;

        public:

            static const struct ldl_radio_interface _interface;

            /** create
             *
             * @param[in] tx_gain
             * @param[in] regulator
             * @param[in] voltage
             * @param[in] xtal
             * @param[in] chip_mode_cb
             *
             * */
            WL55(
                int16_t tx_gain = 0,
                enum ldl_sx126x_regulator regulator = LDL_SX126X_REGULATOR_LDO,
                enum ldl_sx126x_txen txen = LDL_SX126X_TXEN_ENABLED,
                enum ldl_sx126x_voltage voltage = LDL_SX126X_VOLTAGE_1V6,
                enum ldl_radio_xtal xtal = LDL_RADIO_XTAL_CRYSTAL,
                Callback<void(enum ldl_chip_mode)> chip_mode_cb = nullptr
            );

            ~WL55();

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
#endif
