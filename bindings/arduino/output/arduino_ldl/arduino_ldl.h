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
typedef void (*handle_event_fn)(enum lora_mac_response_type type, const union lora_mac_response_arg *arg);

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
        handle_event_fn handle_event;
        
        static void radio_select(void *reciever, bool state);
        static void radio_reset(void *reciever, bool state);
        static void radio_write(void *reciever, uint8_t data);
        static uint8_t radio_read(void *reciever);        
        void arm_dio(struct DioInput *dio);
        void unmask_pcint(uint8_t pin);        
        static ArduinoLDL *to_obj(void *ptr);
        static void adapter(void *receiver, enum lora_mac_response_type type, const union lora_mac_response_arg *arg);
        
    public:

        ArduinoLDL(const ArduinoLDL&) = delete;
        void operator=(const ArduinoLDL&) = delete;
        
        static void interrupt();
        static void getIdentity(void *ptr, struct lora_system_identity *value);
        static uint32_t time();        
        
        /* create an instance 
         * 
         * @param[in] get_id    this function will return the identity structure
         * @param[in] region
         * @param[in] radio_type
         * @param[in] pa        which power amplifier is physically connected?
         * @param[in] nreset    pin connected to the radio reset line
         * @param[in] nselect   pin connected to the radio select line
         * @param[in] dio0      pin connected to the radio dio0 line
         * @param[in] dio1      pin connected to the radio dio0 line
         * 
         * */
        ArduinoLDL(get_identity_fn get_id, enum lora_region region, enum lora_radio_type radio_type, enum lora_radio_pa pa, uint8_t nreset, uint8_t nselect, uint8_t dio0, uint8_t dio1);
             
        /* print event information to serial */
        static void eventDebug(enum lora_mac_response_type type, const union lora_mac_response_arg *arg);
        
        /* print more event information to serial */
        static void eventDebugVerbose(enum lora_mac_response_type type, const union lora_mac_response_arg *arg);
             
        /* send unconfirmed data */
        bool unconfirmedData(uint8_t port, const void *data, uint8_t len);        
        bool unconfirmedData(uint8_t port);        
        
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
        enum lora_mac_errno getErrno();                
        
        /* is stack joined to a network? */
        bool joined();
        
        /* is stack ready to send */
        bool ready();
        
        /* current operation */
        enum lora_mac_operation getOP();
        
        /* current state */
        enum lora_mac_state getState();
        
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
        
        /* system ticks until next channel is available */
        uint32_t ticksUntilNextChannel();              
        
        /* system ticks per second */  
        uint32_t ticksPerSecond();
        
        /* system ticks per millisecond */  
        uint32_t ticksPerMilliSecond();
        
        /* dither send time by (0..dither) seconds for next message 
         * 
         * note. this applies ONLY to the next message sent
         * 
         *  */
        void setSendDither(uint8_t dither);
        
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
        void setAggregatedDutyCycleLimit(uint8_t limit);
     
};

#endif
