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

#include "sx126x.h"
#include "ldl_system.h"
#include "ldl_radio.h"

using namespace LDL;

/* constructors *******************************************************/

SX126X::SX126X(
    enum ldl_radio_type type,
    SPI& spi,
    PinName nss,
    PinName reset,
    PinName busy,
    PinName dio1,
    PinName dio2,
    PinName dio3,
    int16_t tx_gain,
    enum ldl_sx126x_regulator regulator,
    enum ldl_sx126x_txen txen,
    enum ldl_sx126x_voltage voltage,
    enum ldl_radio_xtal xtal,
    Callback<void(enum ldl_chip_mode)> chip_mode_cb
)
    :
    SPIRadio(
        spi,
        nss,
        busy
    ),
    reset(reset, PIN_INPUT, PullNone, 0),
    dio1(dio1),
    chip_mode_cb(chip_mode_cb)
{
    internal_if = LDL_SX1262_getInterface();

    struct ldl_sx126x_init_arg arg = {};

    arg.xtal = xtal;
    arg.tx_gain = tx_gain;

    arg.regulator = regulator;
    arg.txen = txen;
    arg.voltage = voltage;

    arg.chip = this;
    arg.chip_write = &SPIRadio::_chip_write;
    arg.chip_read = &SPIRadio::_chip_read;
    arg.chip_set_mode = &SX126X::_chip_set_mode;

    if(type == LDL_RADIO_SX1262){

        LDL_SX1262_init(&radio, &arg);
    }
    else{

        LDL_SX1261_init(&radio, &arg);
    }

    this->dio1.rise(callback(this, &SX126X::dio1_handler));

    LDL_Radio_setEventCallback(&radio, (struct ldl_mac *)this, &Radio::_interrupt_handler);
}

/* static protected ***************************************************/

void
SX126X::_chip_set_mode(void *self, enum ldl_chip_mode mode)
{
    static_cast<SX126X *>(self)->chip_set_mode(mode);
}

void
SX126X::chip_set_mode(enum ldl_chip_mode mode)
{
    switch(mode){
    case LDL_CHIP_MODE_RESET:
        reset.output();
        break;
    case LDL_CHIP_MODE_SLEEP:
        reset.input();
        break;
    default:
        break;
    }

    if(chip_mode_cb){

        chip_mode_cb(mode);
    }
}

/* protected **********************************************************/

void
SX126X::dio1_handler()
{
    LDL_Radio_handleInterrupt(&radio, 0);
}

