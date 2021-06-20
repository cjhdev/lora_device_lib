/** @file */

#include <stdlib.h>
#include <string.h>
#include "ldl_radio.h"
#include "ldl_mac.h"
#include "ldl_sm.h"
#include "ldl_system.h"

struct ldl_radio radio;
struct ldl_mac mac;
struct ldl_sm sm;

/* pointers to 16B root keys */
extern const void *app_key_ptr;

/* pointers to 8B identifiers */
extern const void *dev_eui_ptr;
extern const void *join_eui_ptr;

/* a pointer to be passed back to the application (anything you like) */
void *app_pointer;

/* a pointer to be passed to the chip interface (anything you like) */
void *chip_interface_pointer;

/* the chip interface */
extern void chip_set_mode(void *self, enum ldl_chip_mode mode);
extern bool chip_write(void *self, const void *opcode, size_t opcode_size, const void *data, size_t size);
extern bool chip_read(void *self, const void *opcode, size_t opcode_size, void *data, size_t size);

/* somehow set a timer event that will ensure a wakeup so many ticks in future */
extern void wakeup_after(uint32_t ticks);

/* somehow activate sleep mode */
extern void sleep(void);

/* somehow enable interrupts */
extern void enable_interrupts(void);

/* Called from within LDL_MAC_process() to pass events back to the application */
void app_handler(void *app, enum ldl_mac_response_type type, const union ldl_mac_response_arg *arg);

uint32_t system_ticks(void *app);
unsigned int system_rand(void *app);

int main(void)
{
    /* initialise the default security module */
    LDL_SM_init(&sm, app_key_ptr);

    /* setup the radio */
    {
        struct ldl_sx127x_init_arg arg = {0};

        arg.xtal = LDL_RADIO_XTAL_CRYSTAL;
        arg.tx_gain = 200;  /* 2dBi */
        arg.pa = LDL_SX127X_PA_RFO;
        arg.chip = chip_interface_pointer,

        arg.chip_read = chip_read,
        arg.chip_write = chip_write,
        arg.chip_set_mode = chip_set_mode,

        LDL_SX1272_init(&radio, &arg);
    }

    {
        struct ldl_mac_init_arg arg = {0};

        /* tell LDL the frequency of the clock source and compensate
         * for uncertainty */
        arg.ticks = system_ticks;
        arg.tps = 1000000;
        arg.a = 10;
        arg.b = 0;
        arg.advance = 0;

        /* connect LDL to a source of random numbers */
        arg.rand = system_rand;

        arg.radio = &radio;
        arg.radio_interface = LDL_Radio_getInterface(&radio);

        arg.sm = &sm;
        arg.sm_interface = LDL_SM_getInterface();

        arg.devEUI = dev_eui_ptr;
        arg.joinEUI = join_eui_ptr;

        arg.app = app_pointer;
        arg.handler = app_handler;
        arg.session = NULL; /* restore cached session state (or not, in this case) */
        arg.devNonce = 0;   /* restore devNonce */
        arg.joinNonce = 0;  /* restore joinNonce */

        LDL_MAC_init(&mac, LDL_EU_863_870, &arg);

        /* remember to connect the radio events back to the MAC */
        LDL_Radio_setEventCallback(&radio, &mac, LDL_MAC_radioEvent);
    }

    /* Optional:
     *
     * Ensure a maximum aggregated duty cycle of ~1%
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

            if(ticks_until_next_event > 0U){

                wakeup_after(ticks_until_next_event);
                sleep();
            }
        }
    }
}

void app_handler(void *app, enum ldl_mac_response_type type, const union ldl_mac_response_arg *arg)
{
    switch(type){

    /* This event is trigger by an earlier call to LDL_MAC_entropy()
     *
     * The value returned is random gathered by the radio driver
     * which can be used to seed stdlib.
     *
     * */
    case LDL_MAC_ENTROPY:
        srand(arg->entropy.value);
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

    /* an opportunity for application to cache joinNonce */
    case LDL_MAC_JOIN_COMPLETE:
        (void)arg->join_complete.joinNonce;
        break;

    /* an opportunity for application to cache nextDevNonce */
    case LDL_MAC_DEV_NONCE_UPDATED:
        (void)arg->dev_nonce_updated.nextDevNonce;
        break;

    case LDL_MAC_CHANNEL_READY:
    case LDL_MAC_OP_ERROR:
    case LDL_MAC_OP_CANCELLED:
    case LDL_MAC_DATA_COMPLETE:
    case LDL_MAC_DATA_TIMEOUT:
    case LDL_MAC_LINK_STATUS:
    case LDL_MAC_DEVICE_TIME:
    case LDL_MAC_JOIN_EXHAUSTED:
    default:
        break;
    }
}

uint32_t system_ticks(void *app)
{
    /* this must read from 32bit ticker */
    return 0;
}

unsigned int system_rand(void *app)
{
    (void)app;

    return rand();
}

void dio0_rising_edge_isr(void)
{
    LDL_Radio_handleInterrupt(&radio, 0);
}

void dio1_rising_edge_isr(void)
{
    LDL_Radio_handleInterrupt(&radio, 1);
}
