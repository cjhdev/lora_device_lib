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

#include "radio.h"
#include "ldl_system.h"
#include "ldl_radio.h"

using namespace LDL;

const struct ldl_radio_interface Radio::interface = {
    .set_mode = Radio::_set_mode,
    .read_entropy = Radio::_read_entropy,
    .read_buffer = Radio::_read_buffer,
    .transmit = Radio::_transmit,
    .receive = Radio::_receive,
    .get_xtal_delay = Radio::_get_xtal_delay,
    .receive_entropy = Radio::_receive_entropy
};

/* constructors *******************************************************/

Radio::Radio(
    SPI& spi,
    PinName nss,
    PinName reset,
    PinName dio0,
    PinName dio1
)
    :
    spi(spi),
    nss(nss, 1),
    reset(reset, PIN_INPUT, PullNone, 1),
    dio0(dio0),
    dio1(dio1)
{
}

SX1272::SX1272(
    SPI& spi,
    PinName nss,
    PinName reset,
    PinName dio0,
    PinName dio1,
    enum ldl_radio_pa pa,
    int16_t tx_gain,
    enum ldl_radio_xtal xtal,
    uint8_t xtal_delay
)
    :
    Radio(
        spi,
        nss,
        reset,
        dio0,
        dio1
    )
{
    struct ldl_radio_init_arg arg = {};

    arg.type = LDL_RADIO_SX1272;
    arg.pa = pa;
    arg.xtal = xtal;
    arg.xtal_delay = xtal_delay;
    arg.tx_gain = tx_gain;

    arg.chip = this;
    arg.chip_write = _chip_write;
    arg.chip_read = _chip_read;
    arg.chip_set_mode = _chip_set_mode;

    LDL_Radio_init(&radio, &arg);

    this->dio0.rise(callback(this, &SX1272::dio0_handler));
    this->dio1.rise(callback(this, &SX1272::dio1_handler));

    LDL_Radio_setEventCallback(&radio, (struct ldl_mac *)this, &Radio::_interrupt_handler);
}

SX1276::SX1276(
    SPI& spi,
    PinName nss,
    PinName reset,
    PinName dio0,
    PinName dio1,
    enum ldl_radio_pa pa,
    int16_t tx_gain,
    enum ldl_radio_xtal xtal,
    uint8_t xtal_delay
)
    :
    Radio(
        spi,
        nss,
        reset,
        dio0,
        dio1
    )
{
    struct ldl_radio_init_arg arg = {};

    arg.type = LDL_RADIO_SX1276;
    arg.pa = pa;
    arg.xtal = xtal;
    arg.xtal_delay = xtal_delay;
    arg.tx_gain = tx_gain;

    arg.chip = this;
    arg.chip_write = _chip_write;
    arg.chip_read = _chip_read;
    arg.chip_set_mode = _chip_set_mode;

    LDL_Radio_init(&radio, &arg);

    this->dio0.rise(callback(this, &SX1276::dio0_handler));
    this->dio1.rise(callback(this, &SX1276::dio1_handler));

    LDL_Radio_setEventCallback(&radio, (struct ldl_mac *)this, &Radio::_interrupt_handler);
}

CMWX1ZZABZ::CMWX1ZZABZ(
    SPI& spi,
    PinName nss,
    PinName reset,
    PinName dio0,
    PinName dio1,
    PinName enable_boost,
    PinName enable_rfo,
    PinName enable_rfi,
    PinName enable_tcxo,
    int16_t tx_gain
)
    :
    SX1276(
        spi,
        nss,
        reset,
        dio0,
        dio1,
        LDL_RADIO_PA_AUTO,
        tx_gain,
        LDL_RADIO_XTAL_TCXO,
        10UL
    ),
    enable_boost(enable_boost, 0),
    enable_rfo(enable_rfo, 0),
    enable_rfi(enable_rfi, 0),
    enable_tcxo(enable_tcxo, PIN_INPUT, PullNone, 1)
{
}

/* static protected ***************************************************/

Radio *
Radio::to_obj(void *self)
{
    return static_cast<Radio *>(self);
}

struct ldl_radio *
Radio::to_state(void *self)
{
    return &to_obj(self)->radio;
}

unsigned int
Radio::_read_entropy(struct ldl_radio *self)
{
    return LDL_Radio_readEntropy(to_state(self));
}

uint8_t
Radio::_read_buffer(struct ldl_radio *self, struct ldl_radio_packet_metadata *meta, void *data, uint8_t max)
{
    return LDL_Radio_readBuffer(to_state(self), meta, data, max);
}

void
Radio::_transmit(struct ldl_radio *self, const struct ldl_radio_tx_setting *settings, const void *data, uint8_t len)
{
    LDL_Radio_transmit(to_state(self), settings, data, len);
}

void
Radio::_receive(struct ldl_radio *self, const struct ldl_radio_rx_setting *settings)
{
    LDL_Radio_receive(to_state(self), settings);
}

void
Radio::_interrupt_handler(void *self, enum ldl_radio_event event)
{
    if(to_obj(self)->event_cb){

        to_obj(self)->event_cb(event);
    }
}

uint8_t
Radio::_get_xtal_delay(struct ldl_radio *self)
{
    return LDL_Radio_getXTALDelay(to_state(self));
}

void
Radio::_receive_entropy(struct ldl_radio *self)
{
    return LDL_Radio_receiveEntropy(to_state(self));
}

void
Radio::_set_mode(struct ldl_radio *self, enum ldl_radio_mode mode)
{
    LDL_Radio_setMode(to_state(self), mode);
}

void
Radio::_chip_set_mode(void *self, enum ldl_chip_mode mode)
{
    return to_obj(self)->chip_set_mode(mode);
}

void
Radio::_chip_write(void *self, uint8_t opcode, const void *data, uint8_t size)
{
    to_obj(self)->chip_write(opcode, data, size);
}

void
Radio::_chip_read(void *self, uint8_t opcode, void *data, uint8_t size)
{
    to_obj(self)->chip_read(opcode, data, size);
}

/* protected **********************************************************/

void
Radio::dio0_handler()
{
    LDL_Radio_handleInterrupt(&radio, 0);
}

void
Radio::dio1_handler()
{
    LDL_Radio_handleInterrupt(&radio, 1);
}

void
Radio::chip_select(bool state)
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
Radio::chip_reset(bool state)
{
    if(state){

        reset.output();
    }
    else{

        reset.input();
    }
}

void
Radio::chip_write(uint8_t opcode, const void *data, uint8_t size)
{
    chip_select(true);

    spi.write(opcode);

    spi.write((const char *)data, size, nullptr, 0);

    chip_select(false);
}

void
Radio::chip_read(uint8_t opcode, void *data, uint8_t size)
{
    chip_select(true);

    spi.write(opcode);

    spi.write(nullptr, 0, (char *)data, size);

    chip_select(false);
}

void
SX1272::chip_set_mode(enum ldl_chip_mode mode)
{
    switch(mode){
    case LDL_CHIP_MODE_RESET:
        chip_reset(true);
        break;
    case LDL_CHIP_MODE_SLEEP:
        chip_reset(false);
        break;
    default:
        break;
    }
}

void
SX1276::chip_set_mode(enum ldl_chip_mode mode)
{
    switch(mode){
    case LDL_CHIP_MODE_RESET:
        chip_reset(true);
        break;
    case LDL_CHIP_MODE_SLEEP:
        chip_reset(false);
        break;
    default:
        break;
    }
}

void
CMWX1ZZABZ::chip_set_mode(enum ldl_chip_mode mode)
{
    switch(mode){
    case LDL_CHIP_MODE_RESET:
        chip_reset(true);
        enable_rfi = 0;
        enable_rfo = 0;
        enable_boost = 0;
        enable_tcxo.input();
        break;
    case LDL_CHIP_MODE_SLEEP:
        chip_reset(false);
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

/* public *************************************************************/

void
Radio::set_event_handler(Callback<void(enum ldl_radio_event)> handler)
{
    event_cb = handler;
}
