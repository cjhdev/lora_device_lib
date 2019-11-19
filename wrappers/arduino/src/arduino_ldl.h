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

#include "ldl_radio.h"
#include "ldl_mac.h"
#include "ldl_system.h"
#include "ldl_chip.h"
#include "ldl_sm.h"

struct arduino_ldl_id {
    
    uint8_t joinEUI[8U];
    uint8_t devEUI[8U];
    uint8_t appKey[16U];    
    uint8_t nwkKey[16U];    // note in LoRaWAN 1.0 this is is appKey
};

typedef void (*handle_rx_fn)(uint16_t counter, uint8_t port, const uint8_t *msg, uint8_t size);
typedef void (*handle_event_fn)(enum ldl_mac_response_type type, const union ldl_mac_response_arg *arg);
typedef void (*get_identity_fn)(struct arduino_ldl_id *id);

namespace LDL {

    /* use the subclasses */
    class Radio {
        
        protected:
        
            struct DioInput {

                const uint8_t pin;
                const uint8_t signal;
                struct ldl_radio &radio;
                volatile bool state;                
                struct DioInput *next;                
                
                DioInput(uint8_t pin, uint8_t signal, struct ldl_radio &radio) : 
                    pin(pin), signal(signal), radio(radio), state(false), next(nullptr) 
                {}
            };
            
            static struct DioInput *dio_inputs; 
        
            const uint8_t nreset;
            const uint8_t nselect;
            
            struct DioInput dio0;
            struct DioInput dio1;
            
            void arm_dio(struct DioInput *dio);
            void unmask_pcint(uint8_t pin);        
            static Radio *to_obj(void *ptr);
                
        public:
        
            static void interrupt();
        
            struct ldl_radio radio;
        
            Radio(const Radio&) = delete;
            void operator=(const Radio&) = delete;
            
            static void radioSelect(void *self, bool state);
            static void radioReset(void *self, bool state);
            
            Radio(enum ldl_radio_type type, enum ldl_radio_pa pa, uint8_t nreset, uint8_t nselect, uint8_t dio0, uint8_t dio1);                        
    };
    
    class SX1272 : public Radio {
        
        public:
        
            /* create an SX1272 radio
             * 
             * @param[in] pa        which power amplifier is physically connected
             * @param[in] nreset    pin connected to the radio reset line
             * @param[in] nselect   pin connected to the radio select line
             * @param[in] dio0      pin connected to the radio dio0 line
             * @param[in] dio1      pin connected to the radio dio0 line
             * 
             * */
            SX1272(enum ldl_radio_pa pa, uint8_t nreset, uint8_t nselect, uint8_t dio0, uint8_t dio1);
    };
    
    class SX1276 : public Radio {

        public:
        
            /* create an SX1276 radio
             * 
             * @param[in] pa        which power amplifier is physically connected
             * @param[in] nreset    pin connected to the radio reset line
             * @param[in] nselect   pin connected to the radio select line
             * @param[in] dio0      pin connected to the radio dio0 line
             * @param[in] dio1      pin connected to the radio dio0 line
             * 
             * */
            SX1276(enum ldl_radio_pa pa, uint8_t nreset, uint8_t nselect, uint8_t dio0, uint8_t dio1);
    };    
    
    class MAC {

        protected:

            struct ldl_sm sm;
            struct ldl_mac mac;
            Radio& radio;
            
            get_identity_fn get_id;
            
            handle_rx_fn handle_rx;
            handle_event_fn handle_event;
            
            static MAC *to_obj(void *ptr);
            static void adapter(void *app, enum ldl_mac_response_type type, const union ldl_mac_response_arg *arg);
            
        public:

            MAC(const MAC&) = delete;
            void operator=(const MAC&) = delete;
            
            
            static void getIdentity(void *ptr, struct arduino_ldl_id *id);
            static uint32_t ticks();        
            
            /* create an instance of MAC
             * 
             * @param[in] radio     
             * @param[in] region
             * @param[in] get_id
             * 
             * */
            MAC(Radio& radio, enum ldl_region region, get_identity_fn get_id);
                 
            /* print event information to serial */
            static void eventDebug(enum ldl_mac_response_type type, const union ldl_mac_response_arg *arg);
            
            /* print more event information to serial */
            static void eventDebugVerbose(enum ldl_mac_response_type type, const union ldl_mac_response_arg *arg);
                 
            /* send unconfirmed data */
            bool unconfirmedData(uint8_t port, const void *data, uint8_t len, const struct ldl_mac_data_opts *opts = NULL);        
            bool unconfirmedData(uint8_t port, const struct ldl_mac_data_opts *opts = NULL);        
            
            /* send confirmed data */
            bool confirmedData(uint8_t port, const void *data, uint8_t len, const struct ldl_mac_data_opts *opts = NULL);        
            bool confirmedData(uint8_t port, const struct ldl_mac_data_opts *opts = NULL);        
            
            /* initiate join */
            bool otaa();     
            
            /* forget network */   
            void forget();
            
            /* cancel current operation */
            void cancel();
            
            /* manage data rate setting */
            bool setRate(uint8_t rate);
            uint8_t getRate();
            
            /* manage power setting */
            bool setPower(uint8_t power);        
            uint8_t getPower();
            
            /* manage ADR (note. ADR is active by default) */
            void enableADR();
            void disableADR();
            
            /* is ADR enabled? */
            bool adr();     
            
            /* get the last error code */
            enum ldl_mac_errno getErrno();                
            
            /* is stack joined to a network? */
            bool joined();
            
            /* is stack ready to send */
            bool ready();
            
            /* current operation */
            enum ldl_mac_operation getOP();
            
            /* current state */
            enum ldl_mac_state getState();
            
            /* set a callback for receiving downstream data messages */
            void onRX(handle_rx_fn handler);
            
            /* set a callback for handling any event 
             * 
             * note if you simply want to print event information
             * you can use the 
             * 
             * */
            void onEvent(handle_event_fn handler);
            
            /* call (repeatedly) to make stack work */
            void process();
            
            /* system ticks until next LDL event */
            uint32_t ticksUntilNextEvent();        
            
            /* system ticks per second */  
            uint32_t ticksPerSecond();
            
            /* system ticks per millisecond */  
            uint32_t ticksPerMilliSecond();
            
            /* Have LDL limit the aggregated duty cycle
             * 
             * This is useful for enforcing things like fair access policies
             * that are more restrictive than region limit.
             * 
             * limit = 1 / (2 ^ limit) 
             * 
             * This is set to 12 by default
             * 
             * */
            void setMaxDCycle(uint8_t maxDCycle);            
            uint8_t getMaxDCycle();
            
            /* transmission redundancy to apply to all upstream messages
             * 
             * */        
            void setNbTrans(uint8_t nbTrans);            
            uint8_t getNbTrans();
    };
    
};

#endif
