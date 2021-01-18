#include "ldl_radio.h"
#include "ldl_mac.h"
#include "ldl_sm.h"
#include "ldl_system.h"

#include <stdlib.h>
#include <string.h>

#include <avr/io.h>

void app_handler(void *app, enum ldl_mac_response_type type, const union ldl_mac_response_arg *arg);
void chip_set_mode(void *self, enum ldl_chip_mode mode);
bool chip_write(void *self, const void *opcode, size_t opcode_size, const void *data, size_t size);
bool chip_read(void *self, const void *opcode, size_t opcode_size, void *data, size_t size);

void spi_init(void);
uint8_t spi_write(uint8_t data);

uint32_t system_ticks(void *app);
uint32_t system_rand(void *app);

struct ldl_mac mac;
struct ldl_radio radio;
struct ldl_sm sm;

void main() __attribute__ ((noreturn));

void main(void)
{
    spi_init();


    {
        const uint8_t nwk_key[] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

        LDL_SM_init(&sm, nwk_key);
    }

    {
        /* this will work with the RFM modules */
        struct ldl_sx127x_init_arg arg = {0};

        arg.xtal = LDL_RADIO_XTAL_CRYSTAL;
        arg.tx_gain = 0;
        arg.pa = LDL_SX127X_PA_BOOST;

        arg.chip_read = chip_read,
        arg.chip_write = chip_write,
        arg.chip_set_mode = chip_set_mode,

        LDL_SX1276_init(&radio, &arg);
    }

    {
        struct ldl_mac_init_arg arg = {0};

        const uint8_t dev_eui[] = {0,0,0,0,0,0,0,0};
        const uint8_t join_eui[] = {0,0,0,0,0,0,0,0};

        arg.ticks = system_ticks;

        arg.rand = system_rand;

        arg.radio = &radio;
        arg.radio_interface = LDL_Radio_getInterface(&radio);

        arg.sm = &sm;
        arg.sm_interface = LDL_SM_getInterface();

        arg.devEUI = dev_eui;
        arg.joinEUI = join_eui;

        arg.handler = app_handler;

        LDL_MAC_init(&mac, LDL_EU_863_870, &arg);
    }

    /* entropy is no longer gathered automatically
     *
     * If you seed srand from an different source this step can be
     * removed.
     *
     * */
    for(;;){

        if(LDL_MAC_ready(&mac)){

            LDL_MAC_entropy(&mac);
            break;
        }
    }

    for(;;){

        if(LDL_MAC_ready(&mac)){

            if(LDL_MAC_joined(&mac)){

                static const char msg[] = "hello";

                LDL_MAC_unconfirmedData(&mac, 1, msg, strlen(msg), NULL);
            }
            else{

                LDL_MAC_otaa(&mac);
            }
        }

        LDL_MAC_process(&mac);
    }
}

void app_handler(void *app, enum ldl_mac_response_type type, const union ldl_mac_response_arg *arg)
{
    (void)app;

    switch(type){
    default:
        break;
    case LDL_MAC_ENTROPY:
        srand(arg->entropy.value);
        break;
    }
}

void chip_set_mode(void *self, enum ldl_chip_mode mode)
{
    (void)self;

    switch(mode){
    case LDL_CHIP_MODE_RESET:
        // drive reset high
        break;
    case LDL_CHIP_MODE_SLEEP:
        //  hiz reset
        break;
    case LDL_CHIP_MODE_STANDBY:
        break;
    case LDL_CHIP_MODE_RX:
        break;
    case LDL_CHIP_MODE_TX_BOOST:
        break;
    case LDL_CHIP_MODE_TX_RFO:
        break;
    }
}

bool chip_write(void *self, const void *opcode, size_t opcode_size, const void *data, size_t size)
{
    (void)self;

    for(size_t i=0U; i < opcode_size; i++){

        (void)spi_write(((const uint8_t *)opcode)[i]);
    }

    for(size_t i=0U; i < size; i++){

        (void)spi_write(((const uint8_t *)data)[i]);
    }

    return true;
}

bool chip_read(void *self, const void *opcode, size_t opcode_size, void *data, size_t size)
{
    (void)self;

    for(size_t i=0U; i < opcode_size; i++){

        (void)spi_write(((const uint8_t *)opcode)[i]);
    }

    for(size_t i=0U; i < size; i++){

        ((uint8_t *)data)[i] = spi_write(0);
    }


    return true;
}

uint32_t system_ticks(void *app)
{
    (void)app;

    return 42U;
}

uint32_t system_rand(void *app)
{
    (void)app;

    return rand();
}

void spi_init(void)
{
    // todo
}

uint8_t spi_write(uint8_t data)
{
    SPDR = data;
    while(!(SPSR & _BV(SPIF)));
    return SPDR;
}
