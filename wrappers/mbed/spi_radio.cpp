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
    .receive_entropy = SPIRadio::_receive_entropy,
    .get_status = SPIRadio::_get_status
};

/* constructors *******************************************************/

SPIRadio::SPIRadio(
    SPI& spi,
    PinName nss,
    PinName busy
)
    :
    Radio(),
    spi(spi),
    nss(nss, 1),
    busy(busy)
{
    timer.start();
}

/* static protected ***************************************************/

bool
SPIRadio::_chip_write(void *self, const void *opcode, size_t opcode_size, const void *data, size_t size)
{
    return static_cast<SPIRadio *>(self)->chip_write(opcode, opcode_size, data, size);
}

bool
SPIRadio::_chip_read(void *self, const void *opcode, size_t opcode_size, void *data, size_t size)
{
    return static_cast<SPIRadio *>(self)->chip_read(opcode, opcode_size, data, size);
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
SPIRadio::_interrupt_handler(struct ldl_mac *self)
{
    if(to_radio(self)->event_cb){

        to_radio(self)->event_cb();
    }
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

void
SPIRadio::_get_status(struct ldl_radio *self, struct ldl_radio_status *status)
{
    to_radio(self)->get_status(status);
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

bool
SPIRadio::chip_write(const void *opcode, size_t opcode_size, const void *data, size_t size)
{
    bool retval = false;

    timer.reset();

    chip_select(true);

    while(!retval){

        if(!busy || (busy.read() == 0)){

            spi.write((const char *)opcode, opcode_size, nullptr, 0);

            spi.write((const char *)data, size, nullptr, 0);

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
SPIRadio::chip_read(const void *opcode, size_t opcode_size, void *data, size_t size)
{
    bool retval = false;

    timer.reset();

    chip_select(true);

    while(!retval){

        if(!busy || (busy.read() == 0)){

            spi.write((const char *)opcode, opcode_size, nullptr, 0);

            spi.write(nullptr, 0, (char *)data, size);

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
SPIRadio::read_entropy()
{
    return internal_if->read_entropy(&radio);
}

uint8_t
SPIRadio::read_buffer(struct ldl_radio_packet_metadata *meta, void *data, uint8_t max)
{
    return internal_if->read_buffer(&radio, meta, data, max);
}

void
SPIRadio::transmit(const struct ldl_radio_tx_setting *settings, const void *data, uint8_t len)
{
    internal_if->transmit(&radio, settings, data, len);
}

void
SPIRadio::receive(const struct ldl_radio_rx_setting *settings)
{
    internal_if->receive(&radio, settings);
}

void
SPIRadio::receive_entropy()
{
    internal_if->receive_entropy(&radio);
}

void
SPIRadio::set_mode(enum ldl_radio_mode mode)
{
    internal_if->set_mode(&radio, mode);
}

void
SPIRadio::get_status(struct ldl_radio_status *status)
{
    internal_if->get_status(&radio, status);
}

const struct ldl_radio_interface *
SPIRadio::get_interface()
{
    return &_interface;
}
