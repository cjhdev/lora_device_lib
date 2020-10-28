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

#include "spi_radio.h"
#include "ldl_system.h"
#include "ldl_radio.h"

using namespace LDL;

const struct ldl_radio_interface SPIRadio::_interface = {
    .set_mode = SPIRadio::_set_mode,
    .read_entropy = SPIRadio::_read_entropy,
    .read_buffer = SPIRadio::_read_buffer,
    .transmit = SPIRadio::_transmit,
    .receive = SPIRadio::_receive,
    .get_xtal_delay = SPIRadio::_get_xtal_delay,
    .receive_entropy = SPIRadio::_receive_entropy
};

/* constructors *******************************************************/

SPIRadio::SPIRadio(
    SPI& spi,
    PinName nss
)
    :
    Radio(),
    spi(spi),
    nss(nss, 1)
{
}

/* static protected ***************************************************/

void
SPIRadio::_chip_write(void *self, uint8_t opcode, const void *data, uint8_t size)
{
    static_cast<SPIRadio *>(self)->chip_write(opcode, data, size);
}

void
SPIRadio::_chip_read(void *self, uint8_t opcode, void *data, uint8_t size)
{
    static_cast<SPIRadio *>(self)->chip_read(opcode, data, size);
}

SPIRadio *
SPIRadio::to_radio(void *self)
{
    return static_cast<SPIRadio *>(self);
}

uint32_t
SPIRadio::_read_entropy(struct ldl_radio *self)
{
    return to_radio(self)->read_entropy();
}

uint8_t
SPIRadio::_read_buffer(struct ldl_radio *self, struct ldl_radio_packet_metadata *meta, void *data, uint8_t max)
{
    return to_radio(self)->read_buffer(meta, data, max);
}

void
SPIRadio::_transmit(struct ldl_radio *self, const struct ldl_radio_tx_setting *settings, const void *data, uint8_t len)
{
    to_radio(self)->transmit(settings, data, len);
}

void
SPIRadio::_receive(struct ldl_radio *self, const struct ldl_radio_rx_setting *settings)
{
    to_radio(self)->receive(settings);
}

void
SPIRadio::_interrupt_handler(struct ldl_mac *self, enum ldl_radio_event event)
{
    if(to_radio(self)->event_cb){

        to_radio(self)->event_cb(event);
    }
}

uint8_t
SPIRadio::_get_xtal_delay(struct ldl_radio *self)
{
    return to_radio(self)->get_xtal_delay();
}

void
SPIRadio::_set_mode(struct ldl_radio *self, enum ldl_radio_mode mode)
{
    to_radio(self)->set_mode(mode);
}

void
SPIRadio::_receive_entropy(struct ldl_radio *self)
{
    to_radio(self)->receive_entropy();
}

/* protected **********************************************************/

void
SPIRadio::chip_select(bool state)
{
    if(state){

        spi.lock();
        spi.frequency(MBED_CONF_LDL_SPI_FREQUENCY);
        spi.format(8U, 0U);
        nss = 0;
    }
    else{

        nss = 1;
        spi.unlock();
    }
}

void
SPIRadio::chip_write(uint8_t opcode, const void *data, uint8_t size)
{
    chip_select(true);

    spi.write(opcode);

    spi.write((const char *)data, size, nullptr, 0);

    chip_select(false);
}

void
SPIRadio::chip_read(uint8_t opcode, void *data, uint8_t size)
{
    chip_select(true);

    spi.write(opcode);

    spi.write(nullptr, 0, (char *)data, size);

    chip_select(false);
}

/* public *************************************************************/

uint32_t
SPIRadio::read_entropy()
{
    return LDL_Radio_readEntropy(&radio);
}

uint8_t
SPIRadio::read_buffer(struct ldl_radio_packet_metadata *meta, void *data, uint8_t max)
{
    return LDL_Radio_readBuffer(&radio, meta, data, max);
}

void
SPIRadio::transmit(const struct ldl_radio_tx_setting *settings, const void *data, uint8_t len)
{
    LDL_Radio_transmit(&radio, settings, data, len);
}

void
SPIRadio::receive(const struct ldl_radio_rx_setting *settings)
{
    LDL_Radio_receive(&radio, settings);
}

uint8_t
SPIRadio::get_xtal_delay()
{
    return LDL_Radio_getXTALDelay(&radio);
}

void
SPIRadio::receive_entropy()
{
    LDL_Radio_receiveEntropy(&radio);
}

void
SPIRadio::set_mode(enum ldl_radio_mode mode)
{
    LDL_Radio_setMode(&radio, mode);
}

const struct ldl_radio_interface *
SPIRadio::get_interface()
{
    return &_interface;
}
