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

/* ticks per second (micros()) */
#define TPS 1000000UL

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
    return TPS;    
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

ArduinoLDL::ArduinoLDL(get_identity_fn get_id, enum lora_region region, enum lora_radio_type radio_type, enum lora_radio_pa pa, uint8_t nreset, uint8_t nselect, uint8_t dio0, uint8_t dio1) : 
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
    
    /* apply TTN fair access policy 
     * 
     * ~30s per day
     * 
     * 30 / (60*60*24)  = 0.000347222
     * 
     * 1 / (2 ^ 11)     = 0.000488281
     * 1 / (2 ^ 12)     = 0.000244141
     * 
     * */
    setAggregatedDutyCycleLimit(12U);
}

uint32_t ArduinoLDL::time()
{
    return LDL_System_time();
}

bool ArduinoLDL::unconfirmedData(uint8_t port, const void *data, uint8_t len)
{
    return LDL_MAC_unconfirmedData(&mac, port, data, len); 
}

bool ArduinoLDL::unconfirmedData(uint8_t port)
{
    return LDL_MAC_unconfirmedData(&mac, port, NULL, 0U); 
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

enum lora_mac_errno ArduinoLDL::getErrno()
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

void ArduinoLDL::getIdentity(void *ptr, struct lora_system_identity *value)
{
    to_obj(ptr)->get_id(value);
}

void ArduinoLDL::enableADR()
{
    LDL_MAC_enableADR(&mac);
}

void ArduinoLDL::disableADR()
{
    LDL_MAC_enableADR(&mac);
}

bool ArduinoLDL::adr()
{
    LDL_MAC_adr(&mac);
}

void ArduinoLDL::onRX(handle_rx_fn handler)
{
    handle_rx = handler;
}

void ArduinoLDL::onEvent(handle_event_fn handler)
{
    handle_event = handler;
}

uint32_t ArduinoLDL::ticksUntilNextEvent()
{
    return LDL_MAC_ticksUntilNextEvent(&mac);
}

uint32_t ArduinoLDL::ticksUntilNextChannel()
{
    return LDL_MAC_ticksUntilNextEvent(&mac);
}

uint32_t ArduinoLDL::ticksPerSecond()
{
    return TPS;
}

uint32_t ArduinoLDL::ticksPerMilliSecond()
{
    return TPS/1000UL;
}

void ArduinoLDL::setSendDither(uint8_t dither)
{
    return LDL_MAC_setSendDither(&mac, dither);
}

void ArduinoLDL::setAggregatedDutyCycleLimit(uint8_t limit)
{
    return LDL_MAC_setAggregatedDutyCycleLimit(&mac, limit);
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

void ArduinoLDL::adapter(void *receiver, enum lora_mac_response_type type, const union lora_mac_response_arg *arg)
{
    ArduinoLDL *self = to_obj(receiver);
    
    /* need to seed rand on startup */
    if(type == LORA_MAC_STARTUP){

        srand(arg->startup.entropy);                        
    }

    if((type == LORA_MAC_RX) && (self->handle_rx != NULL)){
        
        self->handle_rx(arg->rx.counter, arg->rx.port, arg->rx.data, arg->rx.size);
    }
     
    if(self->handle_event != NULL){
        
        self->handle_event(type, arg);
    }
}

void ArduinoLDL::eventDebug(enum lora_mac_response_type type, const union lora_mac_response_arg *arg)
{
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
        break;
    case LORA_MAC_CHIP_ERROR:
        Serial.print(F("CHIP_ERROR"));
        break;            
    case LORA_MAC_RESET:
        Serial.print(F("RESET"));
        break;            
    case LORA_MAC_TX_BEGIN:
        Serial.print(F("TX_BEGIN"));
        break;
    case LORA_MAC_TX_COMPLETE:
        Serial.print(F("TX_COMPLETE"));        
        break;
    case LORA_MAC_RX1_SLOT:
    case LORA_MAC_RX2_SLOT:
        Serial.print((type == LORA_MAC_RX1_SLOT) ? F("RX1_SLOT") : F("RX2_SLOT"));
        break;
    case LORA_MAC_DOWNSTREAM:
        Serial.print(F("DOWNSTREAM"));
        break;
    case LORA_MAC_JOIN_COMPLETE:
        Serial.print(F("JOIN_COMPLETE"));
        break;
    case LORA_MAC_JOIN_TIMEOUT:
        Serial.print(F("JOIN_TIMEOUT"));
        break;
    case LORA_MAC_RX:
        Serial.print(F("RX"));
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
}

void ArduinoLDL::eventDebugVerbose(enum lora_mac_response_type type, const union lora_mac_response_arg *arg)
{
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
        Serial.print(F(": M="));
        Serial.print(arg->link_status.margin);
        Serial.print(F(" GW="));
        Serial.print(arg->link_status.gwCount);                
        break;
    case LORA_MAC_CHIP_ERROR:
        Serial.print(F("CHIP_ERROR"));
        break;            
    case LORA_MAC_RESET:
        Serial.print(F("RESET"));
        break;            
    case LORA_MAC_TX_BEGIN:
        Serial.print(F("TX_BEGIN"));
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
        break;
    case LORA_MAC_TX_COMPLETE:
        Serial.print(F("TX_COMPLETE"));        
        break;
    case LORA_MAC_RX1_SLOT:
    case LORA_MAC_RX2_SLOT:
        Serial.print((type == LORA_MAC_RX1_SLOT) ? F("RX1_SLOT") : F("RX2_SLOT"));
        Serial.print(F(": F="));
        Serial.print(arg->rx_slot.freq);
        Serial.print(F(" SF="));
        Serial.print((uint8_t)arg->rx_slot.sf);
        Serial.print(F(" BW="));
        Serial.print(bw[arg->rx_slot.bw]);                
        break;
    case LORA_MAC_DOWNSTREAM:
        Serial.print(F("DOWNSTREAM"));
        Serial.print(F(": SZ="));
        Serial.print(arg->downstream.size);
        Serial.print(F(" RSSI="));
        Serial.print(arg->downstream.rssi);
        Serial.print(F(" SNR="));
        Serial.print(arg->downstream.snr);                
        break;
    case LORA_MAC_JOIN_COMPLETE:
        Serial.print(F("JOIN_COMPLETE"));
        break;
    case LORA_MAC_JOIN_TIMEOUT:
        Serial.print(F("JOIN_TIMEOUT"));
        Serial.print(F(": RETRY_MS="));
        Serial.print(arg->join_timeout.retry_ms);
        break;
    case LORA_MAC_RX:
        Serial.print(F("RX"));
        Serial.print(F(": PORT="));
        Serial.print(arg->rx.port);
        Serial.print(F(" COUNT="));
        Serial.print(arg->rx.counter);
        Serial.print(F(" SZ="));
        Serial.print(arg->rx.size);
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
}
