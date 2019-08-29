/** @file */

#include "lora_board.h"
#include "lora_radio.h"
#include "lora_mac.h"

/* implemented to suit target */
extern void radio_select(void *receiver, bool state);
extern void radio_reset(void *receiver, bool state);
extern void radio_write(void *receiver, uint8_t value);
extern uint8_t radio_read(void *receiver);

/* implemented to suit application (shown further down) */
void app_handler(void *app, enum lora_mac_response_type type, const union lora_mac_response_arg *arg);

/* state for the board connections, radio, and MAC layer */
struct lora_board board;
struct lora_radio radio;
struct lora_mac mac;

int main(void)
{
    /* The radio driver uses a struct of function pointers to control 
     * the hardware (i.e. the board connections).
     * 
     * The board struct can be initialised directly or via the 
     * LDL_Board_init() function.
     * 
     * */
    LDL_Board_init(&board,
        NULL, 
        radio_select, 
        radio_reset,
        radio_write,
        radio_read
    );

    /* To initialise the radio driver you need:
     * 
     * - board connections
     * - radio type
     *
     * */ 
    LDL_Radio_init(&radio, LORA_RADIO_SX1272, &board);
    
    /* This radio has two power amplifiers. The amplifier in use
     * depends on the hardware (i.e. which pin the PCB traces connect).
     * 
     * You have to tell the driver which amplifier is connected:
     * 
     * - The Semtech MBED SX1272 shield uses LORA_RADIO_PA_RFO
     * - The HopeRF RFM95 SX1276 module uses LORA_RADIO_PA_BOOST
     * 
     * */
    LDL_Radio_setPA(&radio, LORA_RADIO_PA_RFO);

    /* To initialise the mac layer you need:
     * 
     * - initialised radio
     * - region code
     * - application callback
     * 
     * */
    LDL_MAC_init(&mac, NULL, EU_863_870, &radio, app_handler);

    /* 
     * - wait until MAC is ready to send
     * - if not joined, initiate the join
     * - if joined, send a message
     * - run the MAC by polling LDL_MAC_process()
     * 
     * */
    while(true){
        
        if(LDL_MAC_ready(&mac)){
            
            if(LDL_MAC_joined(&mac)){
                
                const char msg[] = "hello world";                
                                
                (void)LDL_MAC_unconfirmedData(&mac, 1, msg, sizeof(msg));
            }
            else{
                
                (void)LDL_MAC_otaa(&mac);
            }            
        }
        
        LDL_MAC_process(&mac);        
    }
}

/* This will be called from within LDL_MAC_process() to pass events back to
 * the application.
 * 
 * Your application can handle or ignore the events depending on
 * your requirements.
 * 
 * */
void app_handler(void *app, enum lora_mac_response_type type, const union lora_mac_response_arg *arg)
{
    switch(type){
        
    /* LoRaWAN needs a little bit of random for correct operation. 
     * 
     * Targets that have no better source of entropy will use this
     * event to seed the stdlib random generator.
     * 
     * */
    case LORA_MAC_STARTUP:
    
        srand(arg->startup.entropy);
        break;
    
    
    /* this is downstream data */
    case LORA_MAC_RX:    
        /* do something */
        break;
        
    /* Ignore all other events */
    case LORA_MAC_CHIP_ERROR:
    case LORA_MAC_RESET:
    case LORA_MAC_JOIN_COMPLETE:
    case LORA_MAC_JOIN_TIMEOUT:
    case LORA_MAC_DATA_COMPLETE:
    case LORA_MAC_DATA_TIMEOUT:
    case LORA_MAC_DATA_NAK:
    case LORA_MAC_LINK_STATUS:
    case LORA_MAC_RX1_SLOT:
    case LORA_MAC_RX2_SLOT:
    case LORA_MAC_TX_COMPLETE:
    case LORA_MAC_TX_BEGIN:
    default:
        break;    
    }
}
