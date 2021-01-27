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

#ifdef TARGET_STM32WL55

#include "wl55.h"
#include "ldl_system.h"
#include "ldl_radio.h"

#include

using namespace LDL;

const struct ldl_radio_interface WL55::_interface = {
    .set_mode = WL55::_set_mode,
    .read_entropy = WL55::_read_entropy,
    .read_buffer = WL55::_read_buffer,
    .transmit = WL55::_transmit,
    .receive = WL55::_receive,
    .receive_entropy = WL55::_receive_entropy,
    .get_status = WL55::_get_status
};

/* constructors *******************************************************/

WL55::WL55(
    int16_t tx_gain,
    enum ldl_sx126x_regulator regulator,
    enum ldl_sx126x_txen txen,
    enum ldl_sx126x_voltage voltage,
    enum ldl_radio_xtal xtal,
    Callback<void(enum ldl_chip_mode)> chip_mode_cb
)
    :
    Radio(),
    chip_mode_cb(chip_mode_cb)
{
    timer.start();
    // init SPI?

    internal_if = LDL_SX1262_getInterface();

    struct ldl_sx126x_init_arg arg = {};

    arg.xtal = xtal;
    arg.tx_gain = tx_gain;

    arg.regulator = regulator;
    arg.txen = txen;
    arg.voltage = voltage;

    arg.chip = this;
    arg.chip_write = &WL55::_chip_write;
    arg.chip_read = &WL55::_chip_read;
    arg.chip_set_mode = &WL55::_chip_set_mode;

    LDL_SX1262_init(&radio, &arg);

    //setup dio0 interrupt
    //this->dio1.rise(callback(this, &SX126X::dio1_handler));

    LDL_Radio_setEventCallback(&radio, (struct ldl_mac *)this, &Radio::_interrupt_handler);
}

/* functions **********************************************************/


/* static protected ***************************************************/

bool
WL55::_chip_write(void *self, const void *opcode, size_t opcode_size, const void *data, size_t size)
{
    return static_cast<WL55 *>(self)->chip_write(opcode, opcode_size, data, size);
}

bool
WL55::_chip_read(void *self, const void *opcode, size_t opcode_size, void *data, size_t size)
{
    return static_cast<WL55 *>(self)->chip_read(opcode, opcode_size, data, size);
}

WL55 *
WL55::to_radio(void *self)
{
    return static_cast<WL55 *>(self);
}

uint32_t
WL55::_read_entropy(struct ldl_radio *self)
{
    return to_radio(self)->read_entropy();
}

uint8_t
WL55::_read_buffer(struct ldl_radio *self, struct ldl_radio_packet_metadata *meta, void *data, uint8_t max)
{
    return to_radio(self)->read_buffer(meta, data, max);
}

void
WL55::_transmit(struct ldl_radio *self, const struct ldl_radio_tx_setting *settings, const void *data, uint8_t len)
{
    to_radio(self)->transmit(settings, data, len);
}

void
WL55::_receive(struct ldl_radio *self, const struct ldl_radio_rx_setting *settings)
{
    to_radio(self)->receive(settings);
}

void
WL55::_interrupt_handler(struct ldl_mac *self)
{
    if(to_radio(self)->event_cb){

        to_radio(self)->event_cb();
    }
}

void
WL55::_set_mode(struct ldl_radio *self, enum ldl_radio_mode mode)
{
    to_radio(self)->set_mode(mode);
}

void
WL55::_receive_entropy(struct ldl_radio *self)
{
    to_radio(self)->receive_entropy();
}

void
WL55::_get_status(struct ldl_radio *self, struct ldl_radio_status *status)
{
    to_radio(self)->get_status(status);
}


/* protected **********************************************************/

void
WL55::chip_select(bool state)
{
    if(state){

        // set format?

        LL_PWR_SelectSUBGHZSPI_NSS();
    }
    else{

        LL_PWR_UnselectSUBGHZSPI_NSS();
    }
}

bool
WL55::chip_write(const void *opcode, size_t opcode_size, const void *data, size_t size)
{
    bool retval = false;

    uint32_t mask = LL_PWR_IsActiveFlag_RFBUSYMS();

    timer.reset();

    chip_select(true);

    while(!retval){

        if((LL_PWR_IsActiveFlag_RFBUSYS() & mask) == 0U){

            //spi.write((const char *)opcode, opcode_size, nullptr, 0);

            //spi.write((const char *)data, size, nullptr, 0);

            retval = true;
        }
        else if(timer.elapsed_time() > 1s){

            break;
        }
        else{

            /* loop */
        }
    }

    chip_select(false);

    return retval;
}

bool
WL55::chip_read(const void *opcode, size_t opcode_size, void *data, size_t size)
{
    bool retval = false;

    uint32_t mask = LL_PWR_IsActiveFlag_RFBUSYMS();

    timer.reset();

    chip_select(true);

    while(!retval){

        if((LL_PWR_IsActiveFlag_RFBUSYS() & mask) == 0U){

            //spi.write((const char *)opcode, opcode_size, nullptr, 0);

            //spi.write(nullptr, 0, (char *)data, size);

            retval = true;
        }
        else if(timer.elapsed_time() > 1s){

            break;
        }
        else{

            /* loop */
        }
    }

    chip_select(false);

    return retval;
}

/* public *************************************************************/

uint32_t
WL55::read_entropy()
{
    return internal_if->read_entropy(&radio);
}

uint8_t
WL55::read_buffer(struct ldl_radio_packet_metadata *meta, void *data, uint8_t max)
{
    return internal_if->read_buffer(&radio, meta, data, max);
}

void
WL55::transmit(const struct ldl_radio_tx_setting *settings, const void *data, uint8_t len)
{
    internal_if->transmit(&radio, settings, data, len);
}

void
WL55::receive(const struct ldl_radio_rx_setting *settings)
{
    internal_if->receive(&radio, settings);
}

void
WL55::receive_entropy()
{
    internal_if->receive_entropy(&radio);
}

void
WL55::set_mode(enum ldl_radio_mode mode)
{
    internal_if->set_mode(&radio, mode);
}

void
WL55::get_status(struct ldl_radio_status *status)
{
    internal_if->get_status(&radio, status);
}

const struct ldl_radio_interface *
WL55::get_interface()
{
    return &_interface;
}

#endif
