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

#include "arduino_ldl.h"

#include <SPI.h>
#include <stdlib.h>

struct ArduinoLDL::DioInput *ArduinoLDL::dio_inputs = nullptr; 

static const SPISettings spi_settings(4000000UL, MSBFIRST, SPI_MODE0);

/* functions **********************************************************/

uint32_t LDL_System_time(void)
{
    return micros();
}

void LDL_System_getIdentity(void *receiver, struct lora_system_identity *value)
{
    ArduinoLDL::getIdentity(receiver, value);    
}

uint32_t LDL_System_tps(void)
{
    /* micros() */
    return 1000000UL;
}

uint32_t LDL_System_advance(void)
{
    return 116UL;
}

uint32_t LDL_System_eps(void)
{
    /* since time source is 1MHz */
    return XTAL_PPM;    
}

/* interrupt handlers *************************************************/

ISR(PCINT0_vect){
    
    ArduinoLDL::interrupt();    
}
ISR(PCINT1_vect, ISR_ALIASOF(PCINT0_vect));
ISR(PCINT2_vect, ISR_ALIASOF(PCINT0_vect));

/* public methods *****************************************************/

uint32_t ArduinoLDL::time()
{
    return LDL_System_time();
}

bool ArduinoLDL::unconfirmedData(uint8_t port, const void *data, uint8_t len)
{
    return LDL_MAC_unconfirmedData(&mac, port, data, len); 
}

bool ArduinoLDL::otaa()
{
    return LDL_MAC_otaa(&mac); 
}

void ArduinoLDL::forget()
{
    LDL_MAC_forget(&mac); 
}

void ArduinoLDL::cancel()
{
    LDL_MAC_cancel(&mac);
}

bool ArduinoLDL::setRate(uint8_t rate)
{
    return LDL_MAC_setRate(&mac, rate);
}

uint8_t ArduinoLDL::getRate()
{
    return LDL_MAC_getRate(&mac);
}

bool ArduinoLDL::setPower(uint8_t power)
{
    return LDL_MAC_setPower(&mac, power);
}

uint8_t ArduinoLDL::getPower()
{
    return LDL_MAC_getPower(&mac);
}

enum lora_mac_errno ArduinoLDL::get_errno()
{
    return LDL_MAC_errno(&mac);
}    

bool ArduinoLDL::joined()
{
    return LDL_MAC_joined(&mac);
}

bool ArduinoLDL::ready()
{
    return LDL_MAC_ready(&mac);
}

enum lora_mac_operation ArduinoLDL::getOP()
{
    return LDL_MAC_op(&mac);
}

enum lora_mac_state ArduinoLDL::getState()
{
    return LDL_MAC_state(&mac);
}

void ArduinoLDL::process()
{        
    LDL_MAC_process(&mac);    
}        

void ArduinoLDL::interrupt()
{
    struct DioInput *ptr = dio_inputs;

    while(ptr != nullptr){
        
        bool state = (digitalRead(ptr->pin) == HIGH);
        
        if(state && !ptr->state){

            LDL_MAC_interrupt(&ptr->mac, ptr->signal, LDL_System_time());
        }
        
        ptr->state = state;
        
        ptr = ptr->next;
    }
}

static void ArduinoLDL::getIdentity(void *ptr, struct lora_system_identity *value)
{
    to_obj(ptr)->get_id(value);
}

void ArduinoLDL::enableADR()
{
    LDL_MAC_enableADR(&mac);
}

bool ArduinoLDL::adr()
{
    LDL_MAC_adr(&mac);
}

void ArduinoLDL::on_rx(handle_rx_fn handler)
{
    handle_rx = handler;
}

/* protected methods **************************************************/

void ArduinoLDL::arm_dio(struct DioInput *dio)
{
    pinMode(dio->pin, INPUT);
    unmask_pcint(dio->pin);
    
    struct DioInput *cur = dio_inputs;
    
    if(cur == nullptr){
        
        dio_inputs = dio;
    }
    else{
        
        while(cur->next != NULL){
            
            cur = cur->next;
        }
        
        cur->next = dio;
    }
}

void ArduinoLDL::radio_select(void *receiver, bool state)
{
    if(state){

        SPI.beginTransaction(spi_settings); 
        digitalWrite(to_obj(receiver)->nselect, LOW);        
    }   
    else{
        
        digitalWrite(to_obj(receiver)->nselect, HIGH);
        SPI.endTransaction();        
    } 
}

void ArduinoLDL::radio_reset(void *receiver, bool state)
{    
    pinMode(to_obj(receiver)->nreset, state ? OUTPUT : INPUT);                
}

void ArduinoLDL::radio_write(void *receiver, uint8_t data)
{
    SPI.transfer(data);
}

uint8_t ArduinoLDL::radio_read(void *receiver)
{
    return SPI.transfer(0U);
}

/* works for AVR only */
void ArduinoLDL::unmask_pcint(uint8_t pin)
{
    volatile uint8_t *pcmsk;
    uint8_t bit;
    
    if(pin < A6){
        
        if(pin <= 7){            
            
            bit = (uint8_t)pin;
            pcmsk = &(PCMSK2);                        
        }
        else if(pin <= 13){
            
            bit = ((uint8_t)pin) - 8U;
            pcmsk = &(PCMSK0);
        }
        else{            
            
            bit = (uint8_t)pin - (uint8_t)A0;
            pcmsk = &(PCMSK1);
        }
 
        *pcmsk |= _BV(bit);
    }    
}

ArduinoLDL *ArduinoLDL::to_obj(void *ptr)
{
    return static_cast<ArduinoLDL *>(ptr);
}
