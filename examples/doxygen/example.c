/** @file */

#include <stdlib.h>
#include <string.h>
#include "ldl_radio.h"
#include "ldl_mac.h"
#include "ldl_sm.h"
#include "ldl_system.h"

/* LDL state */
struct ldl_radio radio;
struct ldl_mac mac;
struct ldl_sm sm;

/* pointers to your 16B root keys */
extern const void *app_key_ptr;
extern const void *nwk_key_ptr;

/* pointers to 8B identifiers */
extern const void *dev_eui_ptr;
extern const void *join_eui_ptr;

/* a pointer to be passed back to the application (anything you like) */
void *app_pointer;

/* a pointer to be passed to the chip interface (anything you like) */
void *chip_interface_pointer;

/* somehow set a timer event that will ensure a wakeup so many ticks in future */
extern void wakeup_after(uint32_t ticks);

/* somehow activate sleep mode */
extern void sleep(void);

/* somehow enable interrupts */
extern void enable_interrupts(void);

/* Called from within LDL_MAC_process() to pass events back to the application */
void app_handler(void *app, enum ldl_mac_response_type type, const union ldl_mac_response_arg *arg);

void chip_set_mode(void *self, enum ldl_chip_mode mode);
void chip_write(void *self, uint8_t addr, const void *data, uint8_t size);
void chip_read(void *self, uint8_t addr, void *data, uint8_t size);

uint32_t system_ticks(void *app);
unsigned int system_rand(void *app);

int main(void)
{
    /* initialise the default security module */
    LDL_SM_init(&sm, app_key_ptr, nwk_key_ptr);

    /* setup the radio */
    {
        struct ldl_radio_init_arg arg = {0};

        arg.type = LDL_RADIO_SX1272;
        arg.xtal = LDL_RADIO_XTAL_CRYSTAL;
        arg.xtal_delay = 0U;
        arg.tx_gain = 0;
        arg.pa = LDL_RADIO_PA_RFO;
        arg.chip = chip_interface_pointer,

        arg.chip_read = chip_read,
        arg.chip_write = chip_write,
        arg.chip_set_mode = chip_set_mode,

        LDL_Radio_init(&radio, &arg);
    }

    {
        /* clearing the arg struct is recommended */
        struct ldl_mac_init_arg arg = {0};

        /* tell LDL the frequency of the clock source and compensate
         * for uncertainty */
        arg.ticks = system_ticks;
        arg.tps = 1000000UL;
        arg.a = 10UL;
        arg.b = 0UL;
        arg.advance = 0UL;

        /* connect LDL to a source of random numbers */
        arg.rand = system_rand;

        /* note that if ldl_mac_init_arg.radio_interface is NULL, the MAC
         * will use the default interface (LDL_Radio_interface).
         *
         * */
        arg.radio = &radio;

        /* note that if ldl_mac_init_arg.sm_interface is NULL, the MAC
         * will use the default interface (LDL_SM_interface).
         *
         * */
        arg.sm = &sm;

        arg.devEUI = dev_eui_ptr;
        arg.joinEUI = join_eui_ptr;

        arg.app = app_pointer;
        arg.handler = app_handler;
        arg.session = NULL; /* restore cached session state (or not, in this case) */
        arg.devNonce = 0U;  /* restore devNonce */
        arg.joinNonce = 0U; /* restore joinNonce */

        LDL_MAC_init(&mac, LDL_EU_863_870, &arg);

        /* remember to connect the radio events back to the MAC */
        LDL_Radio_setEventCallback(&radio, &mac, LDL_MAC_radioEvent);
    }

    /* Ensure a maximum aggregated duty cycle of ~1%
     *
     * EU_863_870 already includes duty cycle limits. This is to safeguard
     * the example if the region is changed to US_902_928 or AU_915_928.
     *
     * Aggregated Duty Cycle Limit = 1 / (2 ^ setting)
     *
     * */
    LDL_MAC_setMaxDCycle(&mac, 7);

    enable_interrupts();

    for(;;){

        if(LDL_MAC_ready(&mac)){

            if(LDL_MAC_joined(&mac)){

                const char msg[] = "hello world";

                /* final argument is NULL since we don't have any specific invocation options */
                (void)LDL_MAC_unconfirmedData(&mac, 1, msg, sizeof(msg), NULL);
            }
            else{

                (void)LDL_MAC_otaa(&mac);
            }
        }

        LDL_MAC_process(&mac);

        /* a demonstration of how you might use sleep modes with LDL */
        {
            uint32_t ticks_until_next_event = LDL_MAC_ticksUntilNextEvent(&mac);

            if(ticks_until_next_event > 0UL){

                wakeup_after(ticks_until_next_event);
                sleep();
            }
        }
    }
}

void app_handler(void *app, enum ldl_mac_response_type type, const union ldl_mac_response_arg *arg)
{
    switch(type){

    /* Some random is required for dithering channels and scheduling
     *
     * Applications that have no better source of entropy will use this
     * event to seed the stdlib random generator.
     *
     * */
    case LDL_MAC_STARTUP:
        srand(arg->startup.entropy);
        break;

    /* this is data from confirmed/unconfirmed down frames */
    case LDL_MAC_RX:
        (void)arg->rx.port;
        (void)arg->rx.data;
        (void)arg->rx.size;
        break;

    /* an opportunity for application to cache session */
    case LDL_MAC_SESSION_UPDATED:
        (void)arg->session_updated.session;
        break;

    /* an opportunity for the application to:
     *
     * - cache the joinNonce
     * - cache the next devNonce
     * - cache session keys
     * - view join parameters (which are stored as part of session state)
     *
     * */
    case LDL_MAC_JOIN_COMPLETE:
        (void)arg->join_complete.nextDevNonce;
        (void)arg->join_complete.joinNonce;
        (void)arg->join_complete.netID;
        (void)arg->join_complete.devAddr;
        break;

    case LDL_MAC_JOIN_TIMEOUT:
    case LDL_MAC_DATA_COMPLETE:
    case LDL_MAC_DATA_TIMEOUT:
    case LDL_MAC_DATA_NAK:
    case LDL_MAC_LINK_STATUS:
    default:
        break;
    }
}

uint32_t system_ticks(void *app)
{
    /* this must read from 32bit ticker */
    return 0UL;
}

unsigned int system_rand(void *app)
{
    (void)app;

    /* can just use stdlib rand */
    return rand();
}

/* somehow connect DIOx interrupt lines back to the radio driver */
void handle_radio_interrupt_dio0(void)
{
    LDL_Radio_handleInterrupt(&radio, 0);
}
void handle_radio_interrupt_dio1(void)
{
    LDL_Radio_handleInterrupt(&radio, 1);
}

void chip_set_mode(void *self, enum ldl_chip_mode mode)
{
    switch(mode){
    case LDL_CHIP_MODE_RESET:
        break;
    case LDL_CHIP_MODE_SLEEP:
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
void chip_write(void *self, uint8_t addr, const void *data, uint8_t size)
{
}
void chip_read(void *self, uint8_t addr, void *data, uint8_t size)
{
}
