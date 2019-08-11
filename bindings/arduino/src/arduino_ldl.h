/* Copyright (c) 2019 Cameron Harper
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


#ifndef ARDUINO_LDL_H
#define ARDUINO_LDL_H

#include <Arduino.h>
#include <SPI.h>

#include "lora_mac.h"
#include "lora_board.h"
#include "lora_system.h"

typedef void (*get_identity_fn)(struct lora_system_identity *id);
typedef void (*handle_rx_fn)(uint16_t counter, uint8_t port, const uint8_t *msg, uint8_t size);

class ArduinoLDL {

    protected:

        struct DioInput {

            struct lora_mac &mac;
            const uint8_t pin;
            const uint8_t signal;
            volatile bool state;
            struct DioInput *next;                
            
            DioInput(uint8_t pin, uint8_t signal, struct lora_mac &mac) : 
                pin(pin), signal(signal), mac(mac), state(false), next(nullptr) 
            {}
        };
        
        static struct DioInput *dio_inputs; 
    
        struct DioInput dio0;
        struct DioInput dio1;
        
        const uint8_t nreset;
        const uint8_t nselect;
        
        struct lora_mac mac;
        struct lora_radio radio;
        struct lora_board board;
        
        get_identity_fn get_id;
    
        handle_rx_fn handle_rx;
        
        static void radio_select(void *reciever, bool state);
        static void radio_reset(void *reciever, bool state);
        static void radio_write(void *reciever, uint8_t data);
        static uint8_t radio_read(void *reciever);        
        void arm_dio(struct DioInput *dio);
        void unmask_pcint(uint8_t pin);        
        static ArduinoLDL *to_obj(void *ptr);

#ifndef DEBUG_LEVEL
#define DEBUG_LEVEL 0
#endif
        
        static void adapter(void *receiver, enum lora_mac_response_type type, const union lora_mac_response_arg *arg)
        {
            /* need to seed rand on startup */
            if(type == LORA_MAC_STARTUP){

                srand(arg->startup.entropy);                        
            }

            {
                ArduinoLDL *self = to_obj(receiver);

                if((type == LORA_MAC_RX) && (self->handle_rx != NULL)){
                    
                    self->handle_rx(arg->rx.counter, arg->rx.port, arg->rx.data, arg->rx.size);
                }
            }

#if DEBUG_LEVEL > 0
            
            const char *bw125 PROGMEM = "125";
            const char *bw250 PROGMEM = "250";
            const char *bw500 PROGMEM = "500";
            
            const char *bw[] PROGMEM = {
                bw125,
                bw250,
                bw500
            };

            Serial.print('[');
            Serial.print(time());
            Serial.print(']');
            
            switch(type){
            case LORA_MAC_STARTUP:
                Serial.print(F("STARTUP"));
                break;            
            case LORA_MAC_LINK_STATUS:
                Serial.print(F("LINK_STATUS"));
#if DEBUG_LEVEL > 1                
                Serial.print(F(": M="));
                Serial.print(arg->link_status.margin);
                Serial.print(F(" GW="));
                Serial.print(arg->link_status.gwCount);                
#endif                
                break;
            case LORA_MAC_CHIP_ERROR:
                Serial.print(F("CHIP_ERROR"));
                break;            
            case LORA_MAC_RESET:
                Serial.print(F("RESET"));
                break;            
            case LORA_MAC_TX_BEGIN:
                Serial.print(F("TX_BEGIN"));
#if DEBUG_LEVEL > 1                                
                Serial.print(F(": SZ="));
                Serial.print(arg->tx_begin.size);
                Serial.print(F(" F="));
                Serial.print(arg->tx_begin.freq);
                Serial.print(F(" SF="));
                Serial.print((uint8_t)arg->tx_begin.sf);
                Serial.print(F(" BW="));
                Serial.print(bw[arg->tx_begin.bw]);                
                Serial.print(F(" P="));
                Serial.print(arg->tx_begin.power);
#endif                
                break;
            case LORA_MAC_TX_COMPLETE:
                Serial.print(F("TX_COMPLETE"));        
                break;
            case LORA_MAC_RX1_SLOT:
            case LORA_MAC_RX2_SLOT:
                Serial.print((type == LORA_MAC_RX1_SLOT) ? F("RX1_SLOT") : F("RX2_SLOT"));
#if DEBUG_LEVEL > 1                
                Serial.print(F(": F="));
                Serial.print(arg->rx_slot.freq);
                Serial.print(F(" SF="));
                Serial.print((uint8_t)arg->rx_slot.sf);
                Serial.print(F(" BW="));
                Serial.print(bw[arg->rx_slot.bw]);                
#endif                
                break;
            case LORA_MAC_DOWNSTREAM:
                Serial.print(F("DOWNSTREAM"));
#if DEBUG_LEVEL > 1                
                Serial.print(F(": SZ="));
                Serial.print(arg->downstream.size);
                Serial.print(F(" RSSI="));
                Serial.print(arg->downstream.rssi);
                Serial.print(F(" SNR="));
                Serial.print(arg->downstream.snr);                
#endif                
                break;
            case LORA_MAC_JOIN_COMPLETE:
                Serial.print(F("JOIN_COMPLETE"));
                break;
            case LORA_MAC_JOIN_TIMEOUT:
                Serial.print(F("JOIN_TIMEOUT"));
#if DEBUG_LEVEL > 1                
                Serial.print(F(": RETRY_MS="));
                Serial.print(arg->join_timeout.retry_ms);
#endif                
                break;
            case LORA_MAC_RX:
                Serial.print(F("RX"));
#if DEBUG_LEVEL > 1                
                Serial.print(F(": PORT="));
                Serial.print(arg->rx.port);
                Serial.print(F(" COUNT="));
                Serial.print(arg->rx.counter);
                Serial.print(F(" SZ="));
                Serial.print(arg->rx.size);
#endif                
                break;
            case LORA_MAC_DATA_COMPLETE:
                Serial.print(F("DATA_COMPLETE"));
                break;
            case LORA_MAC_DATA_TIMEOUT:
                Serial.print(F("DATA_TIMEOUT"));
                break;
            case LORA_MAC_DATA_NAK:
                Serial.print(F("DATA_NAK"));
                break;            
            default:
                break;
            }        
            
            Serial.print('\n');
#endif   

        }
        
    public:

        ArduinoLDL(const ArduinoLDL&) = delete;
        void operator=(const ArduinoLDL&) = delete;
        
        ArduinoLDL(get_identity_fn get_id, enum lora_region region, enum lora_radio_type radio_type, enum lora_radio_pa pa, uint8_t nreset, uint8_t nselect, uint8_t dio0, uint8_t dio1) :
            dio0(dio0, 0, mac), dio1(dio1, 1, mac), nreset(nreset), nselect(nselect), get_id(get_id)
        {
            handle_rx = NULL;
            
            pinMode(nreset, INPUT);
            pinMode(nselect, OUTPUT);
            digitalWrite(nselect, HIGH);
            
            arm_dio(&this->dio0);
            arm_dio(&this->dio1);
            
            SPI.begin();
            
            LDL_Board_init(&board,
                this, 
                radio_select, 
                radio_reset,
                radio_write,
                radio_read
            );
            
            LDL_Radio_init(&radio, radio_type, &board);
            LDL_Radio_setPA(&radio, pa);
            LDL_MAC_init(&mac, this, region, &radio, adapter);
            
            /* works for AVR only */
            PCICR |= _BV(PCIE0) |_BV(PCIE1) |_BV(PCIE2);  
        }
        
        static void interrupt();
        static void getIdentity(void *ptr, struct lora_system_identity *value);
        static uint32_t time();        
           
        /* the API */
           
        /* send unconfirmed data */
        bool unconfirmedData(uint8_t port, const void *data, uint8_t len);        
        
        /* initiate join */
        bool otaa();     
        
        /* forget network */   
        void forget();
        
        /* cancel current operation */
        void cancel();
        
        /* note. setting a rate will disable ADR */
        bool setRate(uint8_t rate);
        uint8_t getRate();
        
        /* note. setting power will disable ADR */
        bool setPower(uint8_t power);        
        uint8_t getPower();
        
        void enableADR();
        
        /* is ADR enabled? */
        bool adr();        
        
        /* get the last error code */
        enum lora_mac_errno get_errno();                
        
        /* is stack joined to a network? */
        bool joined();
        
        /* is stack ready (i.e. not busy) */
        bool ready();
        
        enum lora_mac_operation getOP();
        enum lora_mac_state getState();
        
        /* set a callback for receiving downstream data messages */
        void on_rx(handle_rx_fn handler);
        
        /* call (repeatedly) to make stack work */
        void process();        
};

#endif
