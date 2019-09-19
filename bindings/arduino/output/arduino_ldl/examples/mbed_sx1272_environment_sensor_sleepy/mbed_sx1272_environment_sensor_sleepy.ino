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

#include <arduino_ldl.h>
#include "src/Grove_Temperature_And_Humidity_Sensor/DHT.h"
#include <avr/sleep.h>
#include <avr/wdt.h>

/* dug out of wiring */
#define MICROSECONDS_PER_TIMER0_OVERFLOW (clockCyclesToMicroseconds(64 * 256))
extern unsigned long timer0_millis;
extern unsigned long timer0_overflow_count;

static void get_identity(struct lora_system_identity *id)
{    
    static const struct lora_system_identity _id = {
        .appEUI = {0x00U,0x00U,0x00U,0x00U,0x00U,0x00U,0x00U,0x00U},
        .devEUI = {0x00U,0x00U,0x00U,0x00U,0x00U,0x00U,0x00U,0x01U},
        .appKey = {0x2bU,0x7eU,0x15U,0x16U,0x28U,0xaeU,0xd2U,0xa6U,0xabU,0xf7U,0x15U,0x88U,0x09U,0xcfU,0x4fU,0x3cU}
    };
    
    memcpy(id, &_id, sizeof(*id));
}

ArduinoLDL& get_ldl()
{
    static ArduinoLDL ldl(
        get_identity,           /* specify name of function that returns euis and key */ 
        EU_863_870,             /* specify region */
        LORA_RADIO_SX1272,      /* specify radio */    
        LORA_RADIO_PA_RFO,      /* specify radio power amplifier */    
        A0,                     /* radio reset pin */
        10,                     /* radio select pin */
        2,                      /* radio dio0 pin */
        3                       /* radio dio1 pin */
    );
    
    return ldl;
}

DHT dht(
    6,                  /* pin */
    DHT11               /* sensor type */
);

static void on_rx(uint16_t counter, uint8_t port, const uint8_t *data, uint8_t size)
{
    // do something with this information
}

void setup() 
{
    wdt_disable();
    
    Serial.begin(115200U);       
    dht.begin();            
 
    ArduinoLDL& ldl = get_ldl();

    ldl.onRX(on_rx);
    ldl.onEvent(ldl.eventDebug);
}

void loop() 
{  
    ArduinoLDL& ldl = get_ldl();
    
    if(ldl.ready()){
    
        if(ldl.joined()){
                
            float buf[] = {
                dht.readTemperature(),
                dht.readHumidity()
            };
            
            ldl.unconfirmedData(1U, buf, sizeof(buf));            
        }
        else{
                
            ldl.otaa();
        }
    }
    
    ldl.process();            
    
    {
        uint32_t next_event = ldl.ticksUntilNextEvent();        
        
        /* power down will use the WDT to wake up approx 8s from now */
        if(next_event >= (8UL * ldl.ticksPerSecond())){
        
            /* any active serial will not continue in power down mode so flush it */
            Serial.flush();
            
            set_sleep_mode(SLEEP_MODE_PWR_DOWN);
            
            cli();
            
            wdt_enable(WDTO_8S);
            WDTCSR |= (1 << WDIE);
            
            sleep_enable();
            sleep_bod_disable();
            
            sei();
            
            sleep_cpu();
            sleep_disable();
            
            wdt_disable();
            
            /* fix micros and millis */
            cli();            
            timer0_overflow_count += (8000000UL / MICROSECONDS_PER_TIMER0_OVERFLOW);
            timer0_millis += 8000UL;
            sei();            
        }
        else{
        
            /* idle will be woken by the micros() ticker overflow in the next millisecond */
            if(next_event >= ldl.ticksPerMilliSecond()){
                
                set_sleep_mode(SLEEP_MODE_IDLE);
                cli();
                sleep_enable();
                sei();
                sleep_cpu();
                sleep_disable();
            }
            else{
                
                /* if you sleep here you might be too late to an event! */
            }            
        }
    }
}
