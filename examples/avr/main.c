#include "ldl_radio.h"
#include "ldl_mac.h"
#include "ldl_sm.h"
#include "ldl_system.h"

#include <stdlib.h>
#include <string.h>

void app_handler(void *app, enum ldl_mac_response_type type, const union ldl_mac_response_arg *arg);
void chip_set_mode(void *self, enum ldl_chip_mode mode);
void chip_write(void *self, uint8_t addr, const void *data, uint8_t size);
void chip_read(void *self, uint8_t addr, void *data, uint8_t size);

uint32_t system_ticks(void *app);
uint32_t system_rand(void *app);

struct ldl_mac mac;
struct ldl_radio radio;
struct ldl_sm sm;

void main() __attribute__ ((noreturn));

void main(void)
{
    {
        const uint8_t nwk_key[] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

        LDL_SM_init(&sm, nwk_key);
    }

    {
        struct ldl_radio_init_arg arg = {0};

        arg.type = LDL_RADIO_SX1276;
        arg.xtal = LDL_RADIO_XTAL_CRYSTAL;
        arg.xtal_delay = 0U;
        arg.tx_gain = 0;
        arg.pa = LDL_RADIO_PA_BOOST;

        arg.chip_read = chip_read,
        arg.chip_write = chip_write,
        arg.chip_set_mode = chip_set_mode,

        LDL_Radio_init(&radio, &arg);
    }

    {
        struct ldl_mac_init_arg arg = {0};

        const uint8_t dev_eui[] = {0,0,0,0,0,0,0,0};
        const uint8_t join_eui[] = {0,0,0,0,0,0,0,0};

        arg.ticks = system_ticks;

        arg.rand = system_rand;

        arg.radio = &radio;
        arg.sm = &sm;

        arg.devEUI = dev_eui;
        arg.joinEUI = join_eui;

        arg.handler = app_handler;

        LDL_MAC_init(&mac, LDL_EU_863_870, &arg);
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
    case LDL_MAC_STARTUP:
        srand(arg->startup.entropy);
        break;
    }
}

void chip_set_mode(void *self, enum ldl_chip_mode mode)
{
}

void chip_write(void *self, uint8_t addr, const void *data, uint8_t size)
{
}

void chip_read(void *self, uint8_t addr, void *data, uint8_t size)
{
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
