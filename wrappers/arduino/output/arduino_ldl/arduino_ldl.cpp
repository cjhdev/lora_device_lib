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

using namespace LDL;

struct Radio::DioInput *Radio::dio_inputs = nullptr; 

static const SPISettings spi_settings(4000000UL, MSBFIRST, SPI_MODE0);

/* functions **********************************************************/

uint32_t LDL_System_ticks(void *app)
{
    (void)app;
    return micros();
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
    return XTAL_PPM;    
}

void LDL_Chip_select(void *self, bool state)
{
    (void)self;
    Radio::radioSelect(self, state);               
}

void LDL_Chip_reset(void *self, bool state)
{    
    (void)self;
    Radio::radioReset(self, state);
}

void LDL_Chip_write(void *self, uint8_t data)
{
    (void)self;
    SPI.transfer(data);
}

uint8_t LDL_Chip_read(void *self)
{
    (void)self;
    return SPI.transfer(0U);
}

/* interrupt handlers *************************************************/

ISR(PCINT0_vect){
    
    Radio::interrupt();    
}
ISR(PCINT1_vect, ISR_ALIASOF(PCINT0_vect));
ISR(PCINT2_vect, ISR_ALIASOF(PCINT0_vect));

/* constructors *******************************************************/

Radio::Radio(enum ldl_radio_type type, enum ldl_radio_pa pa, uint8_t nreset, uint8_t nselect, uint8_t dio0, uint8_t dio1) : 
    nreset(nreset), nselect(nselect), dio0(dio0, 0, radio), dio1(dio1, 1, radio)
{
    pinMode(nreset, INPUT);
    pinMode(nselect, OUTPUT);
    digitalWrite(nselect, HIGH);
    
    arm_dio(&this->dio0);
    arm_dio(&this->dio1);
    
    SPI.begin();
    
    LDL_Radio_init(&radio, type, this);
    
    LDL_Radio_setPA(&radio, pa);
    
    /* works for AVR only */
    PCICR |= _BV(PCIE0) |_BV(PCIE1) |_BV(PCIE2);  
}

#ifdef LDL_ENABLE_SX1272
SX1272::SX1272(enum ldl_radio_pa pa, uint8_t nreset, uint8_t nselect, uint8_t dio0, uint8_t dio1) : 
    Radio(LDL_RADIO_SX1272, pa, nreset, nselect, dio0, dio1) 
{    
}
#endif

#ifdef LDL_ENABLE_SX1276
SX1276::SX1276(enum ldl_radio_pa pa, uint8_t nreset, uint8_t nselect, uint8_t dio0, uint8_t dio1) : 
    Radio(LDL_RADIO_SX1276, pa, nreset, nselect, dio0, dio1) 
{
}
#endif

MAC::MAC(Radio& radio, enum ldl_region region, get_identity_fn get_id) : 
    radio(radio), get_id(get_id)
{
    handle_rx = NULL;
    
    struct ldl_mac_init_arg arg;
    
    (void)memset(&arg, 0, sizeof(arg));
    
    struct arduino_ldl_id id;
    
    get_id(&id);
    
    LDL_SM_init(&sm, id.appKey, id.nwkKey);
    
    arg.app = this;
    arg.radio = &radio.radio;
    arg.handler = adapter;
    arg.sm = &sm;
    arg.devEUI = id.devEUI;
    arg.joinEUI = id.joinEUI;
    
    LDL_MAC_init(&mac, region, &arg);
    
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
    setMaxDCycle(12U);
}

/* public methods *****************************************************/

void Radio::radioSelect(void *self, bool state)
{
    if(state){

        SPI.beginTransaction(spi_settings); 
        digitalWrite(to_obj(self)->nselect, LOW);        
    }   
    else{
        
        digitalWrite(to_obj(self)->nselect, HIGH);
        SPI.endTransaction();        
    } 
}

void Radio::radioReset(void *self, bool state)
{
     pinMode(to_obj(self)->nreset, state ? OUTPUT : INPUT);
}

uint32_t MAC::ticks()
{
    return LDL_System_ticks(NULL);
}

bool MAC::unconfirmedData(uint8_t port, const void *data, uint8_t len, const struct ldl_mac_data_opts *opts)
{
    return LDL_MAC_unconfirmedData(&mac, port, data, len, opts); 
}

bool MAC::unconfirmedData(uint8_t port, const struct ldl_mac_data_opts *opts)
{
    return LDL_MAC_unconfirmedData(&mac, port, NULL, 0U, opts); 
}

bool MAC::confirmedData(uint8_t port, const void *data, uint8_t len, const struct ldl_mac_data_opts *opts)
{
    return LDL_MAC_confirmedData(&mac, port, data, len, opts); 
}

bool MAC::confirmedData(uint8_t port, const struct ldl_mac_data_opts *opts)
{
    return LDL_MAC_confirmedData(&mac, port, NULL, 0U, opts); 
}

bool MAC::otaa()
{
    return LDL_MAC_otaa(&mac); 
}

void MAC::forget()
{
    LDL_MAC_forget(&mac); 
}

void MAC::cancel()
{
    LDL_MAC_cancel(&mac);
}

bool MAC::setRate(uint8_t rate)
{
    return LDL_MAC_setRate(&mac, rate);
}

uint8_t MAC::getRate()
{
    return LDL_MAC_getRate(&mac);
}

bool MAC::setPower(uint8_t power)
{
    return LDL_MAC_setPower(&mac, power);
}

uint8_t MAC::getPower()
{
    return LDL_MAC_getPower(&mac);
}

enum ldl_mac_errno MAC::getErrno()
{
    return LDL_MAC_errno(&mac);
}    

bool MAC::joined()
{
    return LDL_MAC_joined(&mac);
}

bool MAC::ready()
{
    return LDL_MAC_ready(&mac);
}

enum ldl_mac_operation MAC::getOP()
{
    return LDL_MAC_op(&mac);
}

enum ldl_mac_state MAC::getState()
{
    return LDL_MAC_state(&mac);
}

void MAC::process()
{        
    LDL_MAC_process(&mac);    
}        

void Radio::interrupt()
{
    struct DioInput *ptr = dio_inputs;

    while(ptr != nullptr){
        
        bool state = (digitalRead(ptr->pin) == HIGH);
        
        if(state && !ptr->state){

            LDL_Radio_interrupt(&ptr->radio, ptr->signal);
        }
        
        ptr->state = state;
        
        ptr = ptr->next;
    }
}

void MAC::getIdentity(void *ptr, struct arduino_ldl_id *id)
{
    to_obj(ptr)->get_id(id);    
}

void MAC::enableADR()
{
    LDL_MAC_enableADR(&mac);
}

void MAC::disableADR()
{
    LDL_MAC_enableADR(&mac);
}

bool MAC::adr()
{
    return LDL_MAC_adr(&mac);
}

void MAC::onRX(handle_rx_fn handler)
{
    handle_rx = handler;
}

void MAC::onEvent(handle_event_fn handler)
{
    handle_event = handler;
}

uint32_t MAC::ticksUntilNextEvent()
{
    return LDL_MAC_ticksUntilNextEvent(&mac);
}

uint32_t MAC::ticksPerSecond()
{
    return TPS;
}

uint32_t MAC::ticksPerMilliSecond()
{
    return TPS/1000UL;
}

void MAC::setMaxDCycle(uint8_t maxDCycle)
{
    LDL_MAC_setMaxDCycle(&mac, maxDCycle);
}

uint8_t MAC::getMaxDCycle()
{
    return LDL_MAC_getMaxDCycle(&mac);
}

void MAC::setNbTrans(uint8_t nbTrans)
{
    LDL_MAC_setNbTrans(&mac, nbTrans);
}

uint8_t MAC::getNbTrans()
{
    return LDL_MAC_getNbTrans(&mac);
}

/* protected methods **************************************************/

void Radio::arm_dio(struct DioInput *dio)
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

/* works for AVR only */
void Radio::unmask_pcint(uint8_t pin)
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

MAC *MAC::to_obj(void *ptr)
{
    return static_cast<MAC *>(ptr);
}

Radio *Radio::to_obj(void *ptr)
{
    return static_cast<Radio *>(ptr);
}

void MAC::adapter(void *receiver, enum ldl_mac_response_type type, const union ldl_mac_response_arg *arg)
{
    MAC *self = to_obj(receiver);
    
    /* need to seed rand on startup */
    if(type == LDL_MAC_STARTUP){

        srand(arg->startup.entropy);                        
    }

    if((type == LDL_MAC_RX) && (self->handle_rx != NULL)){
        
        self->handle_rx(arg->rx.counter, arg->rx.port, arg->rx.data, arg->rx.size);
    }
     
    if(self->handle_event != NULL){
        
        self->handle_event(type, arg);
    }
}

void MAC::eventDebug(enum ldl_mac_response_type type, const union ldl_mac_response_arg *arg)
{
    (void)arg;
    
    Serial.print('[');
    Serial.print(ticks());
    Serial.print(']');
    
    switch(type){
    case LDL_MAC_STARTUP:
        Serial.print(F("STARTUP"));
        break;            
    case LDL_MAC_LINK_STATUS:
        Serial.print(F("LINK_STATUS"));
        break;
    case LDL_MAC_CHIP_ERROR:
        Serial.print(F("CHIP_ERROR"));
        break;            
    case LDL_MAC_RESET:
        Serial.print(F("RESET"));
        break;            
    case LDL_MAC_TX_BEGIN:
        Serial.print(F("TX_BEGIN"));
        break;
    case LDL_MAC_TX_COMPLETE:
        Serial.print(F("TX_COMPLETE"));        
        break;
    case LDL_MAC_RX1_SLOT:
    case LDL_MAC_RX2_SLOT:
        Serial.print((type == LDL_MAC_RX1_SLOT) ? F("RX1_SLOT") : F("RX2_SLOT"));
        break;
    case LDL_MAC_DOWNSTREAM:
        Serial.print(F("DOWNSTREAM"));
        break;
    case LDL_MAC_JOIN_COMPLETE:
        Serial.print(F("JOIN_COMPLETE"));
        break;
    case LDL_MAC_JOIN_TIMEOUT:
        Serial.print(F("JOIN_TIMEOUT"));
        break;
    case LDL_MAC_RX:
        Serial.print(F("RX"));
        break;
    case LDL_MAC_DATA_COMPLETE:
        Serial.print(F("DATA_COMPLETE"));
        break;
    case LDL_MAC_DATA_TIMEOUT:
        Serial.print(F("DATA_TIMEOUT"));
        break;
    case LDL_MAC_DATA_NAK:
        Serial.print(F("DATA_NAK"));
        break;            
    default:
        break;
    }        
    
    Serial.print('\n');
}

void MAC::eventDebugVerbose(enum ldl_mac_response_type type, const union ldl_mac_response_arg *arg)
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
    Serial.print(ticks());
    Serial.print(']');
    
    switch(type){
    case LDL_MAC_STARTUP:
        Serial.print(F("STARTUP"));
        break;            
    case LDL_MAC_LINK_STATUS:
        Serial.print(F("LINK_STATUS"));   
        Serial.print(F(": M="));
        Serial.print(arg->link_status.margin);
        Serial.print(F(" GW="));
        Serial.print(arg->link_status.gwCount);                
        break;
    case LDL_MAC_CHIP_ERROR:
        Serial.print(F("CHIP_ERROR"));
        break;            
    case LDL_MAC_RESET:
        Serial.print(F("RESET"));
        break;            
    case LDL_MAC_TX_BEGIN:
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
    case LDL_MAC_TX_COMPLETE:
        Serial.print(F("TX_COMPLETE"));        
        break;
    case LDL_MAC_RX1_SLOT:
    case LDL_MAC_RX2_SLOT:
        Serial.print((type == LDL_MAC_RX1_SLOT) ? F("RX1_SLOT") : F("RX2_SLOT"));
        Serial.print(F(": F="));
        Serial.print(arg->rx_slot.freq);
        Serial.print(F(" SF="));
        Serial.print((uint8_t)arg->rx_slot.sf);
        Serial.print(F(" BW="));
        Serial.print(bw[arg->rx_slot.bw]);                
        break;
    case LDL_MAC_DOWNSTREAM:
        Serial.print(F("DOWNSTREAM"));
        Serial.print(F(": SZ="));
        Serial.print(arg->downstream.size);
        Serial.print(F(" RSSI="));
        Serial.print(arg->downstream.rssi);
        Serial.print(F(" SNR="));
        Serial.print(arg->downstream.snr);                
        break;
    case LDL_MAC_JOIN_COMPLETE:
        Serial.print(F("JOIN_COMPLETE"));
        break;
    case LDL_MAC_JOIN_TIMEOUT:
        Serial.print(F("JOIN_TIMEOUT"));        
        break;
    case LDL_MAC_RX:
        Serial.print(F("RX"));
        Serial.print(F(": PORT="));
        Serial.print(arg->rx.port);
        Serial.print(F(" COUNT="));
        Serial.print(arg->rx.counter);
        Serial.print(F(" SZ="));
        Serial.print(arg->rx.size);
        break;
    case LDL_MAC_DATA_COMPLETE:
        Serial.print(F("DATA_COMPLETE"));
        break;
    case LDL_MAC_DATA_TIMEOUT:
        Serial.print(F("DATA_TIMEOUT"));
        break;
    case LDL_MAC_DATA_NAK:
        Serial.print(F("DATA_NAK"));
        break;            
    default:
        break;
    }        
    
    Serial.print('\n');
}
