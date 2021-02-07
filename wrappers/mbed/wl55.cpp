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

#ifdef STM32WL55xx

#include "wl55.h"
#include "ldl_system.h"
#include "ldl_radio.h"

#include "stm32wlxx.h"

#include "mbed_critical.h"

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

WL55 * WL55::instances = nullptr;
Mutex WL55::lock;

static void enable_irq()
{
    /* clear the pending that will be hanging around
     * from the last time we silenced an active interrupt
     * source */
    NVIC_ClearPendingIRQ(SUBGHZ_Radio_IRQn);

    NVIC_EnableIRQ(SUBGHZ_Radio_IRQn);
}

static void disable_irq()
{
    NVIC_DisableIRQ(SUBGHZ_Radio_IRQn);
}

/* constructors *******************************************************/

WL55::WL55(
    int16_t tx_gain,
    enum ldl_sx126x_pa pa,
    enum ldl_sx126x_regulator regulator,
    enum ldl_sx126x_voltage voltage,
    enum ldl_radio_xtal xtal,
    Callback<void(enum ldl_chip_mode)> chip_mode_cb
)
    :
    Radio(),
    chip_mode_cb(chip_mode_cb),
    next_instance(nullptr)
{
    timer.start();

    internal_if = LDL_WL55_getInterface();

    struct ldl_sx126x_init_arg arg = {};

    arg.xtal = xtal;
    arg.tx_gain = tx_gain;

    arg.regulator = regulator;
    arg.voltage = voltage;
    arg.pa = pa;

    arg.chip = this;
    arg.chip_write = &WL55::_chip_write;
    arg.chip_read = &WL55::_chip_read;
    arg.chip_set_mode = &WL55::_chip_set_mode;

    LDL_WL55_init(&radio, &arg);

    LDL_Radio_setEventCallback(&radio, (struct ldl_mac *)this, &Radio::_interrupt_handler);

    core_util_critical_section_enter();

    if(instances){

        WL55 *ptr;

        for(ptr=instances; ptr->next_instance; ptr = ptr->next_instance);

        ptr->next_instance = this;
    }
    else{

        NVIC_SetVector(SUBGHZ_Radio_IRQn, (uint32_t)_handle_irq);

        instances = this;
    }

    /* this is about waking from sleep, nothing to do with the interrupt */
#if defined(CM0PLUS)
    LL_C2_EXTI_EnableIT_32_63(LL_EXTI_LINE_44);
#else
    LL_EXTI_EnableIT_32_63(LL_EXTI_LINE_44);
#endif

    //LL_PWR_SetRadioIRQTrigger(LL_PWR_RADIO_IRQ_TRIGGER_WU_IT);


    core_util_critical_section_exit();

}

WL55::~WL55()
{
   timer.stop();

    WL55 *ptr, *prev = nullptr;

    core_util_critical_section_enter();

    for(ptr=instances; ptr; prev = ptr, ptr = ptr->next_instance){

        if(ptr == this){

            if(prev){

                prev->next_instance = next_instance;
            }
            else{

                instances = next_instance;
            }
        }
    }

    core_util_critical_section_exit();
}

/* functions **********************************************************/

static uint32_t get_prescale(uint32_t hz)
{
    static const uint32_t settings[] =  {
        SUBGHZSPI_BAUDRATEPRESCALER_2,
        SUBGHZSPI_BAUDRATEPRESCALER_4,
        SUBGHZSPI_BAUDRATEPRESCALER_8,
        SUBGHZSPI_BAUDRATEPRESCALER_16,
        SUBGHZSPI_BAUDRATEPRESCALER_32,
        SUBGHZSPI_BAUDRATEPRESCALER_64,
        SUBGHZSPI_BAUDRATEPRESCALER_128,
        SUBGHZSPI_BAUDRATEPRESCALER_256
    };

    /* apparently PCLK3 and HCLK3 are the same thing */
    uint32_t spi_hz = HAL_RCC_GetHCLK3Freq();
    size_t i;

    for(i=0; i < sizeof(settings)/sizeof(*settings); i++){

        spi_hz >>= 1;

        if(spi_hz <= hz){

            break;
        }
    }

    return settings[i];
}

static void write_spi(const void *data, size_t size)
{
    size_t i;
    uint32_t count;
    const uint32_t count_preset = 100 * ((SystemCoreClock*28)>>19);

    for(count = count_preset; count > 0U; count--){

        if(READ_BIT(SUBGHZSPI->SR, SPI_SR_TXE) == SPI_SR_TXE){

            break;
        }
    }

    for(i=0; i < size; i++){

#if defined (__GNUC__)
        __IO uint8_t *spidr = ((__IO uint8_t *)&SUBGHZSPI->DR);
        *spidr = ((const uint8_t *)data)[i];
#else
        *((__IO uint8_t *)&SUBGHZSPI->DR) = ((const uint8_t *)data)[i];
#endif
        for(count = count_preset; count > 0U; count--){

            if(READ_BIT(SUBGHZSPI->SR, SPI_SR_RXNE) == SPI_SR_RXNE){

                break;
            }
        }

        READ_REG(SUBGHZSPI->DR);
    }
}

static void read_spi(void *data, size_t size)
{
    size_t i;
    uint32_t count;
    const uint32_t count_preset = 100 * ((SystemCoreClock*28)>>19);

    for(count = count_preset; count > 0U; count--){

        if(READ_BIT(SUBGHZSPI->SR, SPI_SR_TXE) == SPI_SR_TXE){

            break;
        }
    }

    for(i=0; i < size; i++){

#if defined (__GNUC__)
        __IO uint8_t *spidr = ((__IO uint8_t *)&SUBGHZSPI->DR);
        *spidr = 0;
#else
        *((__IO uint8_t *)&SUBGHZSPI->DR) = 0;
#endif
        for(count = count_preset; count > 0U; count--){

            if(READ_BIT(SUBGHZSPI->SR, SPI_SR_RXNE) == SPI_SR_RXNE){

                break;
            }
        }

        ((uint8_t *)data)[i] = (uint8_t)(READ_REG(SUBGHZSPI->DR));
    }
}

/* static protected ***************************************************/

volatile uint16_t last_status = 0;
volatile int nothing_interrupt = 0;

void
WL55::_handle_irq()
{
    if(instances){

        for(WL55 *ptr=instances; ptr; ptr = ptr->next_instance){

    #if 0
            HAL_Delay(100);
    #endif

    #if 0
            last_status = 0;
            uint8_t opcode[] = {0x12,0};

            ptr->chip_select(true);
            ptr->chip_read(opcode, sizeof(opcode), (void *)&last_status, sizeof(last_status));
            ptr->chip_select(false);

            if(last_status != 0){

                LDL_Radio_handleInterrupt(&ptr->radio, 1);

                /* this is a direct that can only be cleared by accessing radio via subghz
                 * in this handler.
                 *
                 * this is not how the driver expects to work and so workaround is
                 * just to turn it off here and on later when required */
                disable_irq();
            }
            else{

                nothing_interrupt++;
            }
    #else
            LDL_Radio_handleInterrupt(&ptr->radio, 1);
    #endif
        }
    }

    disable_irq();
}

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

void
WL55::_chip_set_mode(void *self, enum ldl_chip_mode mode)
{
    static_cast<WL55 *>(self)->chip_set_mode(mode);
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

        //lock.lock();

        __HAL_RCC_SUBGHZSPI_CLK_ENABLE();

        CLEAR_BIT(SUBGHZSPI->CR1, SPI_CR1_SPE);

        WRITE_REG(SUBGHZSPI->CR1, (SPI_CR1_MSTR | SPI_CR1_SSI | get_prescale(MBED_CONF_LDL_SPI_FREQUENCY) | SPI_CR1_SSM));

        WRITE_REG(SUBGHZSPI->CR2, (SPI_CR2_FRXTH |  SPI_CR2_DS_0 | SPI_CR2_DS_1 | SPI_CR2_DS_2));

        SET_BIT(SUBGHZSPI->CR1, SPI_CR1_SPE);

        LL_PWR_SelectSUBGHZSPI_NSS();
    }
    else{

        LL_PWR_UnselectSUBGHZSPI_NSS();

        CLEAR_BIT(SUBGHZSPI->CR1, SPI_CR1_SPE);

        __HAL_RCC_SUBGHZSPI_CLK_DISABLE();

        //lock.unlock();
    }
}

volatile uint32_t write_wait = 0;
volatile uint32_t read_wait = 0;

bool
WL55::chip_write(const void *opcode, size_t opcode_size, const void *data, size_t size)
{
    bool retval = false;

    timer.reset();

    chip_select(true);

    while(!retval){

        if((LL_PWR_IsActiveFlag_RFBUSYS() & LL_PWR_IsActiveFlag_RFBUSYMS()) == 0U){

            write_spi(opcode, opcode_size);
            write_spi(data, size);

            retval = true;
        }
        else if(timer.elapsed_time() > 1s){

            LDL_ERROR("RF BUSY")
            break;
        }
        else{

            write_wait++;

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

    timer.reset();

    chip_select(true);

    while(!retval){

        if((LL_PWR_IsActiveFlag_RFBUSYS() & LL_PWR_IsActiveFlag_RFBUSYMS()) == 0U){

            write_spi(opcode, opcode_size);
            read_spi(data, size);

            retval = true;
        }
        else if(timer.elapsed_time() > 1s){

            LDL_ERROR("RF BUSY")
            break;
        }
        else{

            read_wait++;

            /* loop */
        }
    }

    chip_select(false);

    return retval;
}

void
WL55::chip_set_mode(enum ldl_chip_mode mode)
{
    switch(mode){
    case LDL_CHIP_MODE_RESET:
        LL_RCC_RF_EnableReset();
        break;
    case LDL_CHIP_MODE_SLEEP:
        LL_RCC_RF_DisableReset();
        break;
    case LDL_CHIP_MODE_STANDBY:
        break;
    case LDL_CHIP_MODE_RX:
    case LDL_CHIP_MODE_TX_RFO:
    case LDL_CHIP_MODE_TX_BOOST:
        last_status = UINT16_MAX;
        enable_irq();
        break;
    default:
        break;
    }

    if(chip_mode_cb){

        chip_mode_cb(mode);
    }
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
