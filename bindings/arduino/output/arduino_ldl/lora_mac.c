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

#include "lora_mac.h"
#include "lora_frame.h"
#include "lora_debug.h"
#include "lora_aes.h"
#include "lora_system.h"
#include "lora_mac_commands.h"
#include "lora_stream.h"
#include <string.h>

enum {
    
    ADRAckLimit = 64U,
    ADRAckDelay = 32U,
    ADRAckTimeout = 2U
};

/* static function prototypes *****************************************/

static uint8_t extraSymbols(uint32_t xtal_error, uint32_t symbol_period);
static bool externalDataCommand(struct lora_mac *self, bool confirmed, uint8_t port, const void *data, uint8_t len);
static bool dataCommand(struct lora_mac *self, bool confirmed, uint8_t port, const void *data, uint8_t len);
static uint8_t processCommands(struct lora_mac *self, const uint8_t *in, uint8_t len, bool inFopts, uint8_t *out, uint8_t max);
static bool selectChannel(const struct lora_mac *self, uint8_t rate, uint8_t prevChIndex, uint8_t *chIndex, uint32_t *freq);
static void registerTime(struct lora_mac *self, uint32_t freq, uint32_t airTime);
static void addDefaultChannel(struct lora_mac *receiver, uint8_t chIndex, uint32_t freq, uint8_t minRate, uint8_t maxRate);
static bool getChannel(const struct lora_mac_channel *self, enum lora_region region, uint8_t chIndex, uint32_t *freq, uint8_t *minRate, uint8_t *maxRate);
static bool isAvailable(const struct lora_mac *self, uint8_t chIndex, uint8_t rate);
static uint32_t transmitTime(enum lora_signal_bandwidth bw, enum lora_spreading_factor sf, uint8_t size, bool crc);
static void restoreDefaults(struct lora_mac *self, bool keep);
static bool setChannel(struct lora_mac_channel *self, enum lora_region region, uint8_t chIndex, uint32_t freq, uint8_t minRate, uint8_t maxRate);
static bool maskChannel(uint8_t *self, enum lora_region region, uint8_t chIndex);
static bool unmaskChannel(uint8_t *self, enum lora_region region, uint8_t chIndex);
static void unmaskAllChannels(uint8_t *self, enum lora_region region);
static bool channelIsMasked(const uint8_t *self, enum lora_region region, uint8_t chIndex);
static uint32_t symbolPeriod(enum lora_spreading_factor sf, enum lora_signal_bandwidth bw);
static uint32_t ticksUntilAvailable(const struct lora_mac *self, uint8_t chIndex, uint8_t rate);
static bool rateSettingIsValid(enum lora_region region, uint8_t rate);
static void adaptRate(struct lora_mac *self);
static uint32_t timeNow(struct lora_mac *self);
static uint32_t retryInterval(struct lora_mac *self, uint32_t start_time);

/* functions **********************************************************/

void LDL_MAC_init(struct lora_mac *self, void *app, enum lora_region region, struct lora_radio *radio, lora_mac_response_fn handler)
{
    LORA_PEDANTIC(self != NULL)
    LORA_PEDANTIC(radio != NULL)
    LORA_PEDANTIC(handler != NULL)
    
    (void)memset(self, 0, sizeof(*self));
    
    self->tx.chIndex = UINT8_MAX;
    
    self->app = app;
    self->region = region;
    self->handler = handler;
    self->radio = radio;
    
    LDL_Event_init(&self->events, self->app);
 
    if(!LDL_System_restoreContext(self->app, &self->ctx)){
        
        restoreDefaults(self, false);
    }
 
    self->band_ready = UINT8_MAX;
    
    /* block out all bands for 60s */
#ifndef LORA_DISABLE_START_DELAY                            
    self->band_ready &= ~(1 << (LORA_EVENT_BAND_COMBINED - LORA_EVENT_BAND1));
    LDL_Event_setTimer(&self->events, LORA_EVENT_BAND_COMBINED, 60UL*(LDL_System_tps() + LDL_System_eps()));
#endif            

    LDL_Radio_reset(self->radio, false);

    /* leave reset line alone for 10ms */
    LDL_Event_setTimer(&self->events, LORA_EVENT_WAITA, (LDL_System_tps() + LDL_System_eps())/100UL);
    
    /* the largest even interval */
    LDL_Event_setTimer(&self->events, LORA_EVENT_TIME, (INT32_MAX/LDL_System_tps())*LDL_System_tps());
    
    /* self->state is LORA_STATE_INIT */
}

enum lora_mac_errno LDL_MAC_errno(const struct lora_mac *self)
{
    LORA_PEDANTIC(self != NULL)
    
    return self->errno;
}

enum lora_mac_operation LDL_MAC_op(const struct lora_mac *self)
{
    LORA_PEDANTIC(self != NULL)
    
    return self->op;
}

enum lora_mac_state LDL_MAC_state(const struct lora_mac *self)
{
    LORA_PEDANTIC(self != NULL)
    
    return self->state;
}

bool LDL_MAC_unconfirmedData(struct lora_mac *self, uint8_t port, const void *data, uint8_t len)
{    
    return externalDataCommand(self, false, port, data, len);
}

bool LDL_MAC_confirmedData(struct lora_mac *self, uint8_t port, const void *data, uint8_t len)
{    
    return externalDataCommand(self, true, port, data, len);
}

bool LDL_MAC_otaa(struct lora_mac *self)
{
    LORA_PEDANTIC(self != NULL)
    
    struct lora_frame_join_request f;      
    uint32_t delay;
    struct lora_system_identity identity;
    
    bool retval = false;
    
    self->errno = LORA_ERRNO_NONE;
    
    if(self->state == LORA_STATE_IDLE){
        
        if(self->ctx.joined){
            
            LDL_MAC_forget(self);
        }
        
        self->joinTrial = 0U;
        
        self->tx.rate = LDL_Region_getJoinRate(self->region, self->joinTrial);
        self->tx.power = 0U;
        
        if(LDL_MAC_ticksUntilNextChannel(self) == 0UL){

            LDL_System_getIdentity(self->app, &identity);
            
            (void)memcpy(f.appEUI, identity.appEUI, sizeof(f.appEUI));
            (void)memcpy(f.devEUI, identity.devEUI, sizeof(f.devEUI));
            
            self->devNonce = LDL_System_rand();
            self->devNonce <<= 8U;
            self->devNonce |= LDL_System_rand();
            
            f.devNonce = self->devNonce;

            self->bufferLen = LDL_Frame_putJoinRequest(identity.appKey, &f, self->buffer, sizeof(self->buffer));
                        
            delay = LDL_System_rand();
            delay <<= 8U;
            delay |= LDL_System_rand();
            delay <<= 8U;
            delay |= LDL_System_rand();
            
            delay = delay % (60UL*LDL_System_tps());
            
            LORA_DEBUG("sending join in %"PRIu32" ticks", delay)
                        
            LDL_Event_setTimer(&self->events, LORA_EVENT_WAITA, delay);
            
            self->state = LORA_STATE_WAIT_TX;
            self->op = LORA_OP_JOINING;
            self->firstJoinAttempt = timeNow(self) + (delay / LDL_System_tps());
            
            retval = true;        
        }
        else{
            
            self->errno = LORA_ERRNO_NOCHANNEL;
        }
    }
    else{
        
        self->errno = LORA_ERRNO_BUSY;
    }
    
    return retval;
}

bool LDL_MAC_joined(const struct lora_mac *self)
{
    return self->ctx.joined;
}

void LDL_MAC_forget(struct lora_mac *self)
{
    LORA_PEDANTIC(self != NULL)    
    
    LDL_MAC_cancel(self);    
    
    if(self->ctx.joined){
    
        restoreDefaults(self, true);    
        LDL_System_saveContext(self->app, &self->ctx);
    }
}

void LDL_MAC_cancel(struct lora_mac *self)
{
    LORA_PEDANTIC(self != NULL)
    
    switch(self->state){
    case LORA_STATE_IDLE:
    case LORA_STATE_INIT_RESET:
    case LORA_STATE_INIT_LOCKOUT:
    case LORA_STATE_RECOVERY_RESET:    
    case LORA_STATE_RECOVERY_LOCKOUT:    
    case LORA_STATE_ENTROPY:    
        break;
    default:
        self->state = LORA_STATE_IDLE;
        LDL_Radio_sleep(self->radio);    
        break;
    }   
}

uint32_t LDL_MAC_transmitTimeUp(enum lora_signal_bandwidth bw, enum lora_spreading_factor sf, uint8_t size)
{
    return transmitTime(bw, sf, size, true);
}

uint32_t LDL_MAC_transmitTimeDown(enum lora_signal_bandwidth bw, enum lora_spreading_factor sf, uint8_t size)
{
    return transmitTime(bw, sf, size, false);
}

void LDL_MAC_process(struct lora_mac *self)
{
    LORA_PEDANTIC(self != NULL)
    
    uint32_t error;    
    union lora_mac_response_arg arg;
    struct lora_system_identity identity;
    
    /* handle band duty cycle timeouts */
    {
        uint8_t i;
        
        enum lora_event_timer t[] = {
            LORA_EVENT_BAND1,
            LORA_EVENT_BAND2,
            LORA_EVENT_BAND3,
            LORA_EVENT_BAND4,
            LORA_EVENT_BAND5,                
            LORA_EVENT_BAND_COMBINED                
        };
        
        for(i=0U; i < sizeof(t)/sizeof(*t); i++){
            
            if(LDL_Event_checkTimer(&self->events, t[i], &error)){
                
                self->band_ready |= (1UL << i);                
            }
        }
    }
    
    /* update seconds tracker */
    (void)timeNow(self);
    
    switch(self->state){
    default:
    case LORA_STATE_IDLE:
        break;    
    case LORA_STATE_INIT:
    
        if(LDL_Event_checkTimer(&self->events, LORA_EVENT_WAITA, &error)){
    
            LDL_Radio_reset(self->radio, true);
        
            self->state = LORA_STATE_INIT_RESET;
            self->op = LORA_OP_RESET;
            
            /* hold reset for at least 100us */
            LDL_Event_setTimer(&self->events, LORA_EVENT_WAITA, ((LDL_System_tps() + LDL_System_eps())/10000UL) + 1UL);
            
#ifndef LORA_DISABLE_MAC_RESET_EVENT         
            self->handler(self->app, LORA_MAC_RESET, NULL); 
#endif                    
        }
        break;
        
    case LORA_STATE_INIT_RESET:
    case LORA_STATE_RECOVERY_RESET:
    
        if(LDL_Event_checkTimer(&self->events, LORA_EVENT_WAITA, &error)){
        
            LDL_Radio_reset(self->radio, false);
            
            self->op = LORA_OP_RESET;
            
            switch(self->state){
            default:
            case LORA_STATE_INIT_RESET:
                self->state = LORA_STATE_INIT_LOCKOUT;                
                /* 10ms */
                LDL_Event_setTimer(&self->events, LORA_EVENT_WAITA, ((LDL_System_tps() + LDL_System_eps())/100UL) + 1UL); 
                break;            
            case LORA_STATE_RECOVERY_RESET:
                self->state = LORA_STATE_RECOVERY_LOCKOUT;
                /* 60s */
                LDL_Event_setTimer(&self->events, LORA_EVENT_WAITA, (LDL_System_tps() + LDL_System_eps()) * 60UL);
                break;
            }            
        }    
        break;
        
    case LORA_STATE_INIT_LOCKOUT:
    case LORA_STATE_RECOVERY_LOCKOUT:
    
        if(LDL_Event_checkTimer(&self->events, LORA_EVENT_WAITA, &error)){
            
            self->op = LORA_OP_RESET;
            self->state = LORA_STATE_ENTROPY;
            
            LDL_Radio_entropyBegin(self->radio);                 
            
            /* 100us */
            LDL_Event_setTimer(&self->events, LORA_EVENT_WAITA, ((LDL_System_tps() + LDL_System_eps())/10000UL) + 1UL);
        }
        break;
            
    case LORA_STATE_ENTROPY:
    
        if(LDL_Event_checkTimer(&self->events, LORA_EVENT_WAITA, &error)){
    
            self->op = LORA_OP_RESET;
            
            arg.startup.entropy = LDL_Radio_entropyEnd(self->radio);                    
                
            self->state = LORA_STATE_IDLE;
            self->op = LORA_OP_NONE;
            
#ifndef LORA_DISABLE_MAC_STARTUP_EVENT            
            self->handler(self->app, LORA_MAC_STARTUP, &arg);        
#endif                                
        }
        break;
    
    case LORA_STATE_WAIT_TX:
    
        if(LDL_Event_checkTimer(&self->events, LORA_EVENT_WAITA, &error)){
            
            struct lora_radio_tx_setting radio_setting;
            uint32_t tx_time;
            uint8_t mtu;
            
            (void)selectChannel(self, self->tx.rate, self->tx.chIndex, &self->tx.chIndex, &self->tx.freq);
            
            LDL_Region_convertRate(self->region, self->tx.rate, &radio_setting.sf, &radio_setting.bw, &mtu);
            (void)LDL_Region_getTXPower(self->region, self->tx.power, &radio_setting.dbm);
            
            radio_setting.freq = self->tx.freq;
            
            tx_time = transmitTime(radio_setting.bw, radio_setting.sf, self->bufferLen, true);
            
            LDL_Event_clearInput(&self->events);  
            LDL_Event_setInput(&self->events, LORA_EVENT_INPUT_TX_COMPLETE);  
            
            LDL_Radio_transmit(self->radio, &radio_setting, self->buffer, self->bufferLen);

            registerTime(self, self->tx.freq, tx_time);
            
            self->state = LORA_STATE_TX;
            
            LORA_PEDANTIC((tx_time & 0x80000000UL) != 0x80000000UL)
            
            /* reset the radio if the tx complete interrupt doesn't appear after double the expected time */      
            LDL_Event_setTimer(&self->events, LORA_EVENT_WAITA, tx_time << 1UL);    
            
#ifndef LORA_DISABLE_TX_BEGIN_EVENT            
            arg.tx_begin.freq = self->tx.freq;
            arg.tx_begin.power = self->tx.power;
            arg.tx_begin.sf = radio_setting.sf;
            arg.tx_begin.bw = radio_setting.bw;
            arg.tx_begin.size = self->bufferLen;
            
            self->handler(self->app, LORA_MAC_TX_BEGIN, &arg);
#endif            
        }
        break;
        
    case LORA_STATE_TX:
    
        if(LDL_Event_checkInput(&self->events, LORA_EVENT_INPUT_TX_COMPLETE, &error)){
        
            uint32_t waitSeconds;
            uint32_t waitTicks;    
            uint32_t advance;
            uint32_t advanceA;
            uint32_t advanceB;
            enum lora_spreading_factor sf;
            enum lora_signal_bandwidth bw;
            uint8_t rate;
            uint8_t extra_symbols;
            uint32_t xtal_error;
            uint8_t mtu;
            
            /* the wait interval is always measured in whole seconds */
            waitSeconds = (self->op == LORA_OP_JOINING) ? LDL_Region_getJA1Delay(self->region) : self->ctx.rx1Delay;    
            
            /* add xtal error to ensure the fastest clock will not open before the earliest start time */
            waitTicks = (waitSeconds * LDL_System_tps()) + (waitSeconds * LDL_System_eps());
            
            /* sources of timing advance common to both slots are:
             * 
             * - LDL_System_advance(): interrupt response time + radio RX ramp up
             * - error: ticks since tx complete event
             * 
             * */
            advance = LDL_System_advance() + error;    
            
            /* RX1 */
            {
                LDL_Region_getRX1DataRate(self->region, self->tx.rate, self->ctx.rx1DROffset, &rate);
                LDL_Region_convertRate(self->region, rate, &sf, &bw, &mtu);
                
                xtal_error = (waitSeconds * LDL_System_eps() * 2U);
                
                extra_symbols = extraSymbols(xtal_error, symbolPeriod(sf, bw));
                
                self->rx1_margin = (((3U + extra_symbols) * symbolPeriod(sf, bw)));
                self->rx1_symbols = 8U + extra_symbols;
            
                /* advance timer by time required for extra symbols */
                advanceA = advance + (extra_symbols * symbolPeriod(sf, bw));
            }
            
            /* RX2 */
            {
                LDL_Region_convertRate(self->region, self->ctx.rx2Rate, &sf, &bw, &mtu);
                
                xtal_error = ((waitSeconds + 1UL) * LDL_System_eps() * 2UL);
                
                extra_symbols = extraSymbols(xtal_error, symbolPeriod(sf, bw));
                
                self->rx2_margin = (((3U + extra_symbols) * symbolPeriod(sf, bw)));
                self->rx2_symbols = 8U + extra_symbols;
                
                /* advance timer by time required for extra symbols */
                advanceB = advance + (extra_symbols * symbolPeriod(sf, bw));
            }
                
            if(advanceB <= (waitTicks + (LDL_System_tps() + LDL_System_eps()))){
                
                LDL_Event_setTimer(&self->events, LORA_EVENT_WAITB, waitTicks + (LDL_System_tps() + LDL_System_eps()) - advanceB);
                
                if(advanceA <= waitTicks){
                
                    LDL_Event_setTimer(&self->events, LORA_EVENT_WAITA, waitTicks - advanceA);
                    self->state = LORA_STATE_WAIT_RX1;
                }
                else{
                    
                    LDL_Event_clearTimer(&self->events, LORA_EVENT_WAITA);
                    self->state = LORA_STATE_WAIT_RX2;
                }
            }
            else{
                
                self->state = LORA_STATE_WAIT_RX2;
                LDL_Event_clearTimer(&self->events, LORA_EVENT_WAITA);
                LDL_Event_setTimer(&self->events, LORA_EVENT_WAITB, 0U);
                self->state = LORA_STATE_WAIT_RX2;                
            }    
            
            LDL_Radio_clearInterrupt(self->radio);
            
#ifndef LORA_DISABLE_TX_COMPLETE_EVENT            
            self->handler(self->app, LORA_MAC_TX_COMPLETE, NULL);                        
#endif            
        }
        else{
            
            if(LDL_Event_checkTimer(&self->events, LORA_EVENT_WAITA, &error)){
                
#ifndef LORA_DISABLE_CHIP_ERROR_EVENT                
                self->handler(self->app, LORA_MAC_CHIP_ERROR, NULL);                
#endif                
                LDL_Event_clearInput(&self->events);
                
                self->state = LORA_STATE_RECOVERY_RESET;
                self->op = LORA_OP_RESET;
                
                LDL_Radio_reset(self->radio, true);
                
                /* hold reset for at least 100us */
                LDL_Event_setTimer(&self->events, LORA_EVENT_WAITA, ((LDL_System_tps() + LDL_System_eps())/10000UL) + 1UL);
            }
        }
        break;
        
    case LORA_STATE_WAIT_RX1:
    
        if(LDL_Event_checkTimer(&self->events, LORA_EVENT_WAITA, &error)){
    
            struct lora_radio_rx_setting radio_setting;
            uint32_t freq;    
            uint8_t rate;
            
            LDL_Region_getRX1DataRate(self->region, self->tx.rate, self->ctx.rx1DROffset, &rate);
            LDL_Region_getRX1Freq(self->region, self->tx.freq, self->tx.chIndex, &freq);    
                                    
            LDL_Region_convertRate(self->region, rate, &radio_setting.sf, &radio_setting.bw, &radio_setting.max);
            
            radio_setting.max += LDL_Frame_phyOverhead();
            
            self->state = LORA_STATE_RX1;
            
            if(error <= self->rx1_margin){
                
                radio_setting.freq = freq;
                radio_setting.timeout = self->rx1_symbols;
                
                LDL_Event_clearInput(&self->events);
                LDL_Event_setInput(&self->events, LORA_EVENT_INPUT_RX_READY);
                LDL_Event_setInput(&self->events, LORA_EVENT_INPUT_RX_TIMEOUT);
                
                LDL_Radio_receive(self->radio, &radio_setting);
                
                /* use waitA as a WDT */
                LDL_Event_setTimer(&self->events, LORA_EVENT_WAITA, (LDL_System_tps()) << 4U);                                
            }
            else{
                
                self->state = LORA_STATE_WAIT_RX2;
            }
                
#ifndef LORA_DISABLE_SLOT_EVENT                           
            arg.rx_slot.margin = self->rx1_margin;                
            arg.rx_slot.timeout = self->rx1_symbols;                
            arg.rx_slot.error = error;
            arg.rx_slot.freq = freq;
            arg.rx_slot.bw = radio_setting.bw;
            arg.rx_slot.sf = radio_setting.sf;
                            
            self->handler(self->app, LORA_MAC_RX1_SLOT, &arg);                    
#endif                
        }
        break;
            
    case LORA_STATE_WAIT_RX2:
    
        if(LDL_Event_checkTimer(&self->events, LORA_EVENT_WAITB, &error)){
            
            struct lora_radio_rx_setting radio_setting;
            
            LDL_Region_convertRate(self->region, self->ctx.rx2DataRate, &radio_setting.sf, &radio_setting.bw, &radio_setting.max);
            
            radio_setting.max += LDL_Frame_phyOverhead();
            
            self->state = LORA_STATE_RX2;
            
            if(error <= self->rx2_margin){
                
                radio_setting.freq = self->ctx.rx2Freq;
                radio_setting.timeout = self->rx2_symbols;
                
                LDL_Event_clearInput(&self->events);
                LDL_Event_setInput(&self->events, LORA_EVENT_INPUT_RX_READY);
                LDL_Event_setInput(&self->events, LORA_EVENT_INPUT_RX_TIMEOUT);
                
                LDL_Radio_receive(self->radio, &radio_setting);
                
                /* use waitA as a WDT */
                LDL_Event_setTimer(&self->events, LORA_EVENT_WAITA, (LDL_System_tps()) << 4U);
            }
            else{
                
                adaptRate(self);
                
                self->state = LORA_STATE_IDLE;
            }
                
#ifndef LORA_DISABLE_SLOT_EVENT                                    
            arg.rx_slot.margin = self->rx2_margin;                
            arg.rx_slot.timeout = self->rx2_symbols;                
            arg.rx_slot.error = error;
            arg.rx_slot.freq = self->ctx.rx2Freq;
            arg.rx_slot.bw = radio_setting.bw;
            arg.rx_slot.sf = radio_setting.sf;           
                 
            self->handler(self->app, LORA_MAC_RX2_SLOT, &arg);                    
#endif                
        }
        break;
        
    case LORA_STATE_RX1:    
    case LORA_STATE_RX2:

        if(LDL_Event_checkInput(&self->events, LORA_EVENT_INPUT_RX_READY, &error)){
    
            struct lora_frame frame;
#ifdef LORA_ENABLE_STATIC_RX_BUFFER         
            uint8_t *buffer = self->rx_buffer;
#else   
            uint8_t buffer[LORA_MAX_PACKET];
#endif            
            uint8_t len;
            struct lora_aes_ctx ctx;
            
            struct lora_radio_packet_metadata meta;
            bool retval = false;    
            uint8_t cmd_len = 0U;
            
            LDL_Event_clearTimer(&self->events, LORA_EVENT_WAITA);
            LDL_Event_clearTimer(&self->events, LORA_EVENT_WAITB);
            
            len = LDL_Radio_collect(self->radio, &meta, buffer, LORA_MAX_PACKET);        
            
            LDL_Radio_clearInterrupt(self->radio);
            
            /* notify of a downstream message */
#ifndef LORA_DISABLE_DOWNSTREAM_EVENT            
            arg.downstream.rssi = meta.rssi;
            arg.downstream.snr = meta.snr;
            arg.downstream.size = len;
            
            self->handler(self->app, LORA_MAC_DOWNSTREAM, &arg);      
#endif  
            self->margin = meta.snr;
            self->last_downlink = timeNow(self);
                      
            LDL_System_getIdentity(self->app, &identity);
            
            /* fixme: should this only occur if the frame can be decoded? */
            self->adrAckCounter = 0U;
            self->rxParamSetupAns_pending = false;    
            self->dlChannelAns_pending = false;
            self->rxtimingSetupAns_pending = false;
            self->adrAckReq = false;

            if(LDL_Frame_decode(identity.appKey, self->ctx.nwkSKey, self->ctx.appSKey, buffer, len, &frame)){
                
                if(frame.valid){
                    
                    switch(frame.type){
                    case FRAME_TYPE_JOIN_ACCEPT:
                    
                        if(self->op == LORA_OP_JOINING){

                            restoreDefaults(self, true);
                            self->ctx.joined = true;
                            
                            if(self->ctx.adr){
                                
                                /* keep the joining rate */
                                self->ctx.rate = self->tx.rate;
                            }                
                            
                            self->ctx.rx1DROffset = frame.fields.joinAccept.rx1DataRateOffset;
                            self->ctx.rx2DataRate = frame.fields.joinAccept.rx2DataRate;
                            self->ctx.rx1Delay = frame.fields.joinAccept.rxDelay;
 
                            {
                                size_t i;
                            
                                switch(frame.fields.joinAccept.cfListType){
                                default:
                                case NO_CFLIST:
                                    break;
                                case FREQ_CFLIST:
                                
                                    for(i=0U; i < sizeof(frame.fields.joinAccept.cfList)/sizeof(*frame.fields.joinAccept.cfList); i++){
                                        
                                        uint8_t chIndex = 3U + i;
                                        uint8_t band = 255U;
                                        
                                        if(LDL_Region_getBand(self->region, frame.fields.joinAccept.cfList[i], &band)){
                                            
                                            (void)setChannel(self->ctx.chConfig, self->region, chIndex, frame.fields.joinAccept.cfList[i], 0U, 5U);
                                        }
                                        else{
                                            
                                            LORA_INFO("cflist channel is invalid for the region")
                                        }
                                    }
                                    break;
                                case MASK_CFLIST:
                                
                                    for(i=0U; i < sizeof(frame.fields.joinAccept.cfList)/sizeof(*frame.fields.joinAccept.cfList); i++){
                                        
                                        uint8_t b;
                                        
                                        for(b=0U; b < 16U; b++){
                                            
                                            if((frame.fields.joinAccept.cfList[i] & (1 << b)) > 0U){ 
                                            
                                                (void)unmaskChannel(self->ctx.chMask, self->region, (i * 16U) + b);
                                            }
                                        }
                                    }
                                    break;
                                }
                            }
                            
                            LDL_AES_init(&ctx, identity.appKey);
                            
                            (void)memset(self->ctx.nwkSKey, 0U, sizeof(self->ctx.nwkSKey));
                            
                            self->ctx.nwkSKey[0] = 1U;                
                            self->ctx.nwkSKey[1] = frame.fields.joinAccept.appNonce;
                            self->ctx.nwkSKey[2] = frame.fields.joinAccept.appNonce >> 8;
                            self->ctx.nwkSKey[3] = frame.fields.joinAccept.appNonce >> 16;
                            self->ctx.nwkSKey[4] = frame.fields.joinAccept.netID;
                            self->ctx.nwkSKey[5] = frame.fields.joinAccept.netID >> 8;
                            self->ctx.nwkSKey[6] = frame.fields.joinAccept.netID >> 16;
                            self->ctx.nwkSKey[7] = self->devNonce;
                            self->ctx.nwkSKey[8] = self->devNonce >> 8;
                            
                            (void)memcpy(self->ctx.appSKey, self->ctx.nwkSKey, sizeof(self->ctx.appSKey));
                            
                            self->ctx.appSKey[0] = 2U;                
                            
                            LDL_AES_encrypt(&ctx, self->ctx.nwkSKey);
                            LDL_AES_encrypt(&ctx, self->ctx.appSKey);
                                   
                            self->ctx.devAddr = frame.fields.joinAccept.devAddr;
                            
                            retval = true;
                        }
                        else{

                            LORA_INFO("unexpected join-accept")
                        }
                        break;
                    
                    case FRAME_TYPE_DATA_UNCONFIRMED_DOWN:
                    case FRAME_TYPE_DATA_CONFIRMED_DOWN:
                        
                        if(self->ctx.joined){
                        
                            if(self->ctx.devAddr == frame.fields.data.devAddr){

                                if((uint32_t)frame.fields.data.counter < ((uint32_t)self->ctx.down + (uint32_t)LDL_Region_getMaxFCNTGap(self->region))){
    
                                    self->ctx.down = frame.fields.data.counter;
                                
                                    if(frame.fields.data.data != NULL){
                            
                                        if(frame.fields.data.port > 0U){
#ifndef LORA_DISABLE_RX_EVENT                                            
                                            arg.rx.counter = frame.fields.data.counter;
                                            arg.rx.port = frame.fields.data.port;
                                            arg.rx.data = frame.fields.data.data;
                                            arg.rx.size = frame.fields.data.dataLen;                                            
                                            self->handler(self->app, LORA_MAC_RX, &arg);                                        
#endif                                            
                                            cmd_len = processCommands(self, frame.fields.data.opts, frame.fields.data.optsLen, false, buffer, LORA_MAX_PACKET);
                                        }
                                        else{
                                        
                                            cmd_len = processCommands(self, frame.fields.data.data, frame.fields.data.dataLen, false, buffer, LORA_MAX_PACKET);
                                        }
                                    }
                                    else{
                                        
                                        cmd_len = processCommands(self, frame.fields.data.opts, frame.fields.data.optsLen, false, buffer, LORA_MAX_PACKET);
                                    }
                                    
                                    retval = true;
                                }
                                else{
                                    
                                    LORA_INFO("unexpected counter")
                                }
                            }
                            else{
                                
                                LORA_INFO("unexpected devaddr")        
                            }
                        }
                        else{
                            
                            LORA_INFO("unexpected data")
                        }
                        break;
                    
                    case FRAME_TYPE_JOIN_REQ:
                    case FRAME_TYPE_DATA_UNCONFIRMED_UP:
                    case FRAME_TYPE_DATA_CONFIRMED_UP:            
                    default:
                        LORA_INFO("unexpected direction")
                        break;
                    }
                }
                else{
                    
                    LORA_INFO("unexpected mic")
                }
            }
            else{
                
                LORA_INFO("unexpected encoding")                
            }
            
            switch(self->op){
            default:
            case LORA_OP_NONE:
                break;
            case LORA_OP_DATA_UNCONFIRMED:
            
                self->handler(self->app, LORA_MAC_DATA_COMPLETE, NULL);
                break;
            
            case LORA_OP_JOINING:                        
            
                if(retval){
                    
                    self->handler(self->app, LORA_MAC_JOIN_COMPLETE, NULL);                    
                }
                else{
                    
                    self->handler(self->app, LORA_MAC_JOIN_TIMEOUT, NULL);
                }
                break;    
                
            case LORA_OP_DATA_CONFIRMED:
            
                if(retval && frame.fields.data.ack){
                    
                    self->handler(self->app, LORA_MAC_DATA_COMPLETE, NULL);
                }
                else{
                    
                    self->handler(self->app, LORA_MAC_DATA_NAK, NULL);
                }
                break;
            }    
            
            if(cmd_len > 0U){

                LORA_INFO("sending mac response")
            
                /* MAC command may have masked everything... */
                if(LDL_MAC_ticksUntilNextChannel(self) != UINT32_MAX){
            
                    self->tx.rate = self->ctx.rate;
                    self->tx.power = self->ctx.power;
            
                    struct lora_frame_data f;
                    
                    (void)memset(&f, 0, sizeof(f));
                
                    f.devAddr = self->ctx.devAddr;
                    f.counter = ++self->ctx.up;
                    f.adr = self->ctx.adr;
                    f.adrAckReq = self->adrAckReq;
                    
                    if(cmd_len <= 15U){
                        
                        f.opts = buffer;
                        f.optsLen = cmd_len;
                    }
                    else{
                        
                        f.data = buffer;
                        f.dataLen = cmd_len;
                    }
                    
                    self->bufferLen = LDL_Frame_putData(FRAME_TYPE_DATA_UNCONFIRMED_UP, self->ctx.nwkSKey, self->ctx.appSKey, &f, self->buffer, sizeof(self->buffer));
                    
                    /* LDL_Frame_putData() failed */
                    LORA_PEDANTIC(self->bufferLen > 0)
                    
                    LDL_Event_setTimer(&self->events, LORA_EVENT_WAITA, LDL_MAC_ticksUntilNextChannel(self));
                    
                    self->state = LORA_STATE_WAIT_TX;
                    self->op = LORA_OP_DATA_UNCONFIRMED;
                }
                else{
                    
                    LORA_INFO("cannot send; all channels disabled!")
                }
            }
            else{
                
                self->state = LORA_STATE_IDLE;           
                self->op = LORA_OP_NONE;
            }
            
            LDL_System_saveContext(self->app, &self->ctx);             
        }
        else if(LDL_Event_checkInput(&self->events, LORA_EVENT_INPUT_RX_TIMEOUT, &error)){
                
            LDL_Radio_clearInterrupt(self->radio);
            
            if(self->state == LORA_STATE_RX2){
            
                LDL_Event_clearTimer(&self->events, LORA_EVENT_WAITA);
                
                switch(self->op){
                default:
                case LORA_OP_NONE:
                    break;
                case LORA_OP_DATA_UNCONFIRMED:
                    
                    adaptRate(self);
                    
                    self->handler(self->app, LORA_MAC_DATA_COMPLETE, NULL);
                    self->state = LORA_STATE_IDLE;
                    self->op = LORA_OP_NONE;      
                    
                    break;
                    
                case LORA_OP_DATA_CONFIRMED:
                    
                    adaptRate(self);
                    
                    self->handler(self->app, LORA_MAC_DATA_TIMEOUT, NULL);
                    self->state = LORA_STATE_IDLE;
                    self->op = LORA_OP_NONE;      
                    
                    break;

                case LORA_OP_JOINING:
                    
                    self->msUntilRetry = retryInterval(self, self->firstJoinAttempt);
                    self->joinTrial++;
                    self->tx.rate = LDL_Region_getJoinRate(self->region, self->joinTrial);
                    
                    (void)selectChannel(self, self->tx.rate, self->tx.chIndex, &self->tx.chIndex, &self->tx.freq);
                    
                    LORA_INFO("join timeout")
                    LORA_DEBUG("next join attempt will occur in %"PRIu32" ms", self->msUntilRetry)
                    
                    arg.join_timeout.retry_ms = self->msUntilRetry;
                    self->handler(self->app, LORA_MAC_JOIN_TIMEOUT, &arg);                    
                    
                    self->state = LORA_STATE_WAIT_RETRY;
                    
                    LDL_Event_setTimer(&self->events, LORA_EVENT_WAITA, 0U);                    
    
                    break;
                    
                }        
                
                LDL_System_saveContext(self->app, &self->ctx); 
            }
            else{
                
                self->state = LORA_STATE_WAIT_RX2;
            }   
        }
        else{
            
            if(LDL_Event_checkTimer(&self->events, LORA_EVENT_WAITA, &error) || LDL_Event_checkTimer(&self->events, LORA_EVENT_WAITB, &error)){
                
#ifndef LORA_DISABLE_CHIP_ERROR_EVENT                
                self->handler(self->app, LORA_MAC_CHIP_ERROR, NULL); 
#endif                
                LDL_Event_clearInput(&self->events);
                LDL_Event_clearTimer(&self->events, LORA_EVENT_WAITA);
                LDL_Event_clearTimer(&self->events, LORA_EVENT_WAITB);
                
                self->state = LORA_STATE_RECOVERY_RESET;
                self->op = LORA_OP_RESET;
                
                LDL_Radio_reset(self->radio, true);
                
                /* hold reset for at least 100us */
                LDL_Event_setTimer(&self->events, LORA_EVENT_WAITA, ((LDL_System_tps() + LDL_System_eps())/10000UL) + 1U);                     
            }
        }
        break;
    
    case LORA_STATE_WAIT_RETRY:
        
        if(LDL_Event_checkTimer(&self->events, LORA_EVENT_WAITA, &error)){
            
            if(self->msUntilRetry > 0UL){
            
                if(self->msUntilRetry > (INT32_MAX/(LDL_System_tps()/1000UL))){
                    
                    LDL_Event_setTimer(&self->events, LORA_EVENT_WAITA, (INT32_MAX/(LDL_System_tps()/1000UL))*(LDL_System_tps()/1000UL));
                    self->msUntilRetry -= (INT32_MAX/(LDL_System_tps()/1000UL));
                }
                else{
            
                    LORA_DEBUG("wait another %"PRIu32" ticks", (uint32_t)(self->msUntilRetry * (LDL_System_tps() / 1000UL)));
            
                    LDL_Event_setTimer(&self->events, LORA_EVENT_WAITA, self->msUntilRetry * (LDL_System_tps() / 1000UL));
                    self->msUntilRetry = 0UL;
                }                
            }
            else{
                
                LDL_Event_setTimer(&self->events, LORA_EVENT_WAITA, 0UL);
                self->state = LORA_STATE_WAIT_TX;
            }
        }
        break;    
    }

}

uint32_t LDL_MAC_ticksUntilNextEvent(const struct lora_mac *self)
{
    LORA_PEDANTIC(self != NULL)
    
    return LDL_Event_ticksUntilNext(&self->events);
}

bool LDL_MAC_setRate(struct lora_mac *self, uint8_t rate)
{
    LORA_PEDANTIC(self != NULL)
    
    bool retval = false;    
    self->errno = LORA_ERRNO_NONE;
    
    if(rateSettingIsValid(self->region, rate)){
        
        self->ctx.rate = rate;
        LDL_System_saveContext(self->app, &self->ctx);
        retval = true;        
    }
    else{
        
        self->errno = LORA_ERRNO_RATE;
    }
    
    return retval;
}

uint8_t LDL_MAC_getRate(const struct lora_mac *self)
{
    LORA_PEDANTIC(self != NULL)
    
    return self->ctx.rate;
}

bool LDL_MAC_setPower(struct lora_mac *self, uint8_t power)
{
    LORA_PEDANTIC(self != NULL)
    
    bool retval = false;
    int16_t dbm;
    self->errno = LORA_ERRNO_NONE;
        
    if(LDL_Region_getTXPower(self->region, power, &dbm)){
        
        self->ctx.power = power;
        LDL_System_saveContext(self->app, &self->ctx);        
        retval = true;
    }
    else{
     
        self->errno = LORA_ERRNO_POWER;
    }        
    
    return retval;
}

uint8_t LDL_MAC_getPower(const struct lora_mac *self)
{
    LORA_PEDANTIC(self != NULL)
    
    return self->ctx.power;
}

#ifndef LORA_DISABLE_ADR
bool LDL_MAC_enableADR(struct lora_mac *self)
{
    LORA_PEDANTIC(self != NULL)
    
    self->ctx.adr = true;
    LDL_System_saveContext(self->app, &self->ctx);        
    return true;
}

bool LDL_MAC_adr(const struct lora_mac *self)
{
    LORA_PEDANTIC(self != NULL)
    
    return self->ctx.adr;
}

bool LDL_MAC_disableADR(struct lora_mac *self)
{
    LORA_PEDANTIC(self != NULL)
    
    self->ctx.adr = false;
    LDL_System_saveContext(self->app, &self->ctx);        
    
    return true;
}
#endif

bool LDL_MAC_ready(const struct lora_mac *self)
{
    LORA_PEDANTIC(self != NULL)
    
    bool retval = false;
    
    if(self->state == LORA_STATE_IDLE){
        
        retval = (LDL_MAC_ticksUntilNextChannel(self) == 0UL);        
    }
    
    return retval;
}

#ifndef LORA_DISABLE_CHECK
bool LDL_MAC_check(struct lora_mac *self, bool now)
{
    LORA_PEDANTIC(self != NULL)
    
    bool retval = false;
    uint8_t buf;
    
    self->errno = LORA_ERRNO_NONE;
    self->linkCheckReq_pending = false;                
    
    if(self->state == LORA_STATE_IDLE){
    
        if(self->ctx.joined){
        
            if(now){
                
                if(LDL_MAC_ticksUntilNextChannel(self) == 0U){
        
                    self->linkCheckReq_pending = true; 
                    retval = dataCommand(self, false, 0U, &buf, 0U);                       
                }
                else{
                    
                    self->errno = LORA_ERRNO_NOCHANNEL;
                }
            }
            else{
            
                self->linkCheckReq_pending = true;                
                retval = true;
            }
        }
        else{
            
            self->errno = LORA_ERRNO_NOTJOINED;
        }
    }
    else{
        
        self->errno = LORA_ERRNO_BUSY;
    }
    
    return retval;
}
#endif

uint32_t LDL_MAC_bwToNumber(enum lora_signal_bandwidth bw)
{
    uint32_t retval;
    
    switch(bw){
    default:
    case BW_125:
        retval = 125000UL;
        break;        
    case BW_250:
        retval = 250000UL;
        break;            
    case BW_500:
        retval = 500000UL;
        break;                
    }
    
    return retval;
}

void LDL_MAC_interrupt(struct lora_mac *self, uint8_t n, uint32_t time)
{
    LORA_PEDANTIC(self != NULL)
    
    switch(LDL_Radio_signal(self->radio, n)){
    case LORA_RADIO_EVENT_TX_COMPLETE:
        LDL_Event_signal(&self->events, LORA_EVENT_INPUT_TX_COMPLETE, time);
        break;
    case LORA_RADIO_EVENT_RX_READY:
        LDL_Event_signal(&self->events, LORA_EVENT_INPUT_RX_READY, time);
        break;
    case LORA_RADIO_EVENT_RX_TIMEOUT:
        LDL_Event_signal(&self->events, LORA_EVENT_INPUT_RX_TIMEOUT, time);        
        break;
    case LORA_RADIO_EVENT_NONE:
    default:
        LORA_ERROR("radio cannot translate interrupt code")
        break;
    }     
}

uint32_t LDL_MAC_ticksUntilNextChannel(const struct lora_mac *self)
{
    uint32_t min = UINT32_MAX;
    uint32_t current;
    size_t i;
    
    for(i=0U; i < LDL_Region_numChannels(self->region); i++){
        
        /* which rate?? */
        current = ticksUntilAvailable(self, i, self->ctx.rate);
        
        if(current < min){
            
            min = current;
        }
    }
    
    current = LDL_Event_ticksUntil(&self->events, LORA_EVENT_BAND_COMBINED);
    
    if((current != UINT32_MAX) && (current > min)){
        
        min = current;
    }
    
    return min;    
}

uint8_t LDL_MAC_mtu(const struct lora_mac *self)
{
    LORA_PEDANTIC(self != NULL)
    
    enum lora_signal_bandwidth bw;
    enum lora_spreading_factor sf;
    uint8_t max = 0U;
    uint8_t overhead = LDL_Frame_dataOverhead();    
    
    /* which rate?? */
    LDL_Region_convertRate(self->region, self->ctx.rate, &sf, &bw, &max);
    
    LORA_PEDANTIC(LDL_Frame_dataOverhead() < max)
    
    if(self->dlChannelAns_pending){
        
        overhead += LDL_MAC_sizeofCommandUp(DL_CHANNEL);        
    }
    
    if(self->rxtimingSetupAns_pending){
        
        overhead += LDL_MAC_sizeofCommandUp(RX_TIMING_SETUP);                       
    }
    
    if(self->rxParamSetupAns_pending){
        
        overhead += LDL_MAC_sizeofCommandUp(RX_PARAM_SETUP);
    }
    
    if(self->linkCheckReq_pending){
        
        overhead += LDL_MAC_sizeofCommandUp(LINK_CHECK);
    }
    
    return (overhead > max) ? 0 : max - overhead;    
}

uint32_t LDL_MAC_timeSinceDownlink(struct lora_mac *self)
{
    LORA_PEDANTIC(self != NULL)
    
    return (self->last_downlink == 0) ? UINT32_MAX : timeNow(self) - self->last_downlink;
}

void LDL_MAC_setSendDither(struct lora_mac *self, uint8_t dither)
{
    LORA_PEDANTIC(self != NULL)
    
    self->tx_dither = dither;
}

void LDL_MAC_setAggregatedDutyCycleLimit(struct lora_mac *self, uint8_t limit)
{
    LORA_PEDANTIC(self != NULL)
    
    self->ctx.maxDutyCycle = limit & 0xfU;
}

/* static functions ***************************************************/

static bool externalDataCommand(struct lora_mac *self, bool confirmed, uint8_t port, const void *data, uint8_t len)
{
    bool retval = false;
    uint8_t maxPayload;
    enum lora_signal_bandwidth bw;
    enum lora_spreading_factor sf;
    
    self->errno = LORA_ERRNO_NONE;

    if(self->state == LORA_STATE_IDLE){
        
        if(self->ctx.joined){
        
            if((port > 0U) && (port <= 223U)){    
                
                if(LDL_MAC_ticksUntilNextChannel(self) == 0U){
                 
                    LDL_Region_convertRate(self->region, self->ctx.rate, &sf, &bw, &maxPayload);
                 
                    if(len <= (maxPayload - LDL_Frame_dataOverhead())){
             
                        retval = dataCommand(self, confirmed, port, data, len);
                    }
                    else{
                        
                        self->errno = LORA_ERRNO_SIZE;
                    }                                        
                }
                else{
                    
                    self->errno = LORA_ERRNO_NOCHANNEL;
                }                
            }
            else{
                
                self->errno = LORA_ERRNO_PORT;
            }
        }
        else{
            
            self->errno = LORA_ERRNO_NOTJOINED;
        }
    }
    else{
        
        self->errno = LORA_ERRNO_BUSY;
    }
            
    return retval;
}

static bool dataCommand(struct lora_mac *self, bool confirmed, uint8_t port, const void *data, uint8_t len)
{
    LORA_PEDANTIC(self != NULL)
    LORA_PEDANTIC((len == 0) || (data != NULL))
    
    bool retval = false;
    struct lora_frame_data f;
    struct lora_stream s;
    uint8_t maxPayload;
    uint8_t opts[15U];
    enum lora_signal_bandwidth bw;
    enum lora_spreading_factor sf;
    
    self->tx.rate = self->ctx.rate;
    self->tx.power = self->ctx.power;
    
    /* pending MAC commands take priority over user payload */
    (void)LDL_Stream_init(&s, opts, sizeof(opts));
    
    if(self->dlChannelAns_pending){
    
        struct lora_dl_channel_ans ans;        
        (void)memset(&ans, 0, sizeof(ans));
        (void)LDL_MAC_putDLChannelAns(&s, &ans);                                    
    }
    
    if(self->rxtimingSetupAns_pending){
        
        (void)LDL_MAC_putRXTimingSetupAns(&s);                                    
    }
    
    if(self->rxParamSetupAns_pending){
        
        struct lora_rx_param_setup_ans ans;
        (void)memset(&ans, 0, sizeof(ans));
        (void)LDL_MAC_putRXParamSetupAns(&s, &ans);                                    
    }
    
    if(self->linkCheckReq_pending){

        (void)LDL_MAC_putLinkCheckReq(&s);
        self->linkCheckReq_pending = false;                                            
    }                                
    
    LDL_Region_convertRate(self->region, self->tx.rate, &sf, &bw, &maxPayload);
    
    LORA_PEDANTIC(maxPayload >= LDL_Frame_dataOverhead())
    
    (void)memset(&f, 0, sizeof(f));
    
    f.devAddr = self->ctx.devAddr;
    f.counter = ++self->ctx.up;
    f.adr = self->ctx.adr;
    f.adrAckReq = self->adrAckReq;
    f.opts = opts;
    f.optsLen = LDL_Stream_tell(&s);
    f.port = port;
    
    self->state = LORA_STATE_WAIT_TX;    
    
    /* it's possible the user data doesn't fit after mac command priority */
    if((LDL_Stream_tell(&s) + LDL_Frame_dataOverhead() + len) <= maxPayload){
        
        f.data = (const uint8_t *)data;
        f.dataLen = len;
        self->bufferLen = LDL_Frame_putData(confirmed ? FRAME_TYPE_DATA_CONFIRMED_UP : FRAME_TYPE_DATA_UNCONFIRMED_UP, self->ctx.nwkSKey, self->ctx.appSKey, &f, self->buffer, sizeof(self->buffer));
        self->op = confirmed ? LORA_OP_DATA_CONFIRMED : LORA_OP_DATA_UNCONFIRMED;
        retval = true;
    }
    else{
        
        self->bufferLen = LDL_Frame_putData(FRAME_TYPE_DATA_UNCONFIRMED_UP, self->ctx.nwkSKey, self->ctx.appSKey, &f, self->buffer, sizeof(self->buffer));
        self->op = LORA_OP_DATA_UNCONFIRMED;
        self->errno = LORA_ERRNO_SIZE;  //fixme: special error code to say goalpost moved?
    }
    
    /* putData must have failed for some reason */
    LORA_PEDANTIC(self->bufferLen > 0U)
    
    uint32_t send_delay = 0U;
    
    if(self->tx_dither > 0U){
        
        uint32_t _random; 
        
        _random = LDL_System_rand();
        _random <<= 8;
        _random |= LDL_System_rand();
        _random <<= 8;
        _random |= LDL_System_rand();
        _random <<= 8;
        _random |= LDL_System_rand();
        
        send_delay = (_random % ((uint32_t)self->tx_dither*LDL_System_tps()));
        
        self->tx_dither = 0U;
    }
    
    LDL_Event_setTimer(&self->events, LORA_EVENT_WAITA, send_delay);
    self->tx_dither = 0U;
    
    return retval;
}

static void adaptRate(struct lora_mac *self)
{
    self->adrAckReq = false;
    
    if(self->ctx.adr){
                
        if(self->adrAckCounter < UINT8_MAX){
            
            if(self->adrAckCounter >= ADRAckLimit){
                
                self->adrAckReq = true;
            
                LORA_DEBUG("adr: adrAckCounter=%u (past ADRAckLimit)", self->adrAckCounter)
            
                if(self->adrAckCounter >= (ADRAckLimit + ADRAckDelay)){
                
                    if(((self->adrAckCounter - (ADRAckLimit + ADRAckDelay)) % ADRAckDelay) == 0U){
                
                        if(self->ctx.power == 0U){
                
                            if(self->ctx.rate > LORA_DEFAULT_RATE){
                                                            
                                self->ctx.rate--;
                                LORA_DEBUG("adr: rate reduced to %u", self->ctx.rate)
                            }
                            else{
                                
                                LORA_DEBUG("adr: all channels unmasked")
                                
                                unmaskAllChannels(self->ctx.chMask, self->region);
                                
                                self->adrAckCounter = UINT8_MAX;
                            }
                        }
                        else{
                            
                            LORA_DEBUG("adr: full power enabled")
                            self->ctx.power = 0U;
                        }
                    }
                }    
            }
                
            self->adrAckCounter++;            
        }        
    }
}

static uint32_t transmitTime(enum lora_signal_bandwidth bw, enum lora_spreading_factor sf, uint8_t size, bool crc)
{
    /* from 4.1.1.7 of sx1272 datasheet
     *
     * Ts (symbol period)
     * Rs (symbol rate)
     * PL (payload length)
     * SF (spreading factor
     * CRC (presence of trailing CRC)
     * IH (presence of implicit header)
     * DE (presence of data rate optimize)
     * CR (coding rate 1..4)
     * 
     *
     * Ts = 1 / Rs
     * Tpreamble = ( Npreamble x 4.25 ) x Tsym
     *
     * Npayload = 8 + max( ceil[( 8PL - 4SF + 28 + 16CRC + 20IH ) / ( 4(SF - 2DE) )] x (CR + 4), 0 )
     *
     * Tpayload = Npayload x Ts
     *
     * Tpacket = Tpreamble + Tpayload
     * 
     * */

    bool header;
    bool lowDataRateOptimize;
    uint32_t Tpacket;
    uint32_t Ts;
    uint32_t Tpreamble;
    uint32_t numerator;
    uint32_t denom;
    uint32_t Npayload;
    uint32_t Tpayload;
    
    Tpacket = 0UL;
    
    /* optimise this mode according to the datasheet */
    lowDataRateOptimize = ((bw == BW_125) && ((sf == SF_11) || (sf == SF_12))) ? true : false;    
    
    /* lorawan always uses a header */
    header = true; 

    Ts = symbolPeriod(sf, bw);
    Tpreamble = (Ts * 12UL) +  (Ts / 4UL);

    numerator = (8UL * (uint32_t)size) - (4UL * (uint32_t)sf) + 28UL + ( crc ? 16UL : 0UL ) - ( header ? 20UL : 0UL );
    denom = 4UL * ((uint32_t)sf - ( lowDataRateOptimize ? 2UL : 0UL ));

    Npayload = 8UL + ((((numerator / denom) + (((numerator % denom) != 0UL) ? 1UL : 0UL)) * ((uint32_t)CR_5 + 4UL)));

    Tpayload = Npayload * Ts;

    Tpacket = Tpreamble + Tpayload;

    return Tpacket;
}

static uint32_t symbolPeriod(enum lora_spreading_factor sf, enum lora_signal_bandwidth bw)
{
    return ((((uint32_t)1U) << sf) * LDL_System_tps()) / LDL_MAC_bwToNumber(bw);
}

static uint8_t extraSymbols(uint32_t xtal_error, uint32_t symbol_period)
{
    return (xtal_error / symbol_period) + (((xtal_error % symbol_period) > 0U) ? 1U : 0U);        
}

static uint8_t processCommands(struct lora_mac *self, const uint8_t *in, uint8_t len, bool inFopts, uint8_t *out, uint8_t max)
{
    uint8_t retval = 0U;
    
    struct lora_stream s_in;
    struct lora_stream s_out;
    
    struct lora_downstream_cmd cmd;
    struct lora_mac_session shadow;
    union lora_mac_response_arg arg;
    struct lora_link_adr_ans adr_ans;
    enum lora_mac_cmd_type next_cmd;
    
    enum {
        
        _NO_ADR,
        _ADR_OK,
        _ADR_BAD
        
    } adr_state = _NO_ADR;
    
    (void)memset(&adr_ans, 0, sizeof(adr_ans));
    (void)memcpy(&shadow, &self->ctx, sizeof(shadow));

    LDL_Stream_initReadOnly(&s_in, in, len);
    LDL_Stream_init(&s_out, out, max);
    
    adr_ans.channelMaskOK = true;
    
    while(LDL_Stream_remaining(&s_in) > 0U){
        
        if(LDL_MAC_getDownCommand(&s_in, &cmd)){
            
            switch(cmd.type){
            default:
            case LINK_CHECK:
            {
                const struct lora_link_check_ans *ans = &cmd.fields.linkCheckAns;
                
                arg.link_status.inFOpt = inFopts;
                arg.link_status.margin = ans->margin;
                arg.link_status.gwCount = ans->gwCount;            
                
                LORA_DEBUG("link_check_ans: margin=%u gwCount=%u", 
                    ans->margin,
                    ans->gwCount             
                )
                
                self->handler(self->app, LORA_MAC_LINK_STATUS, &arg);                                                             
            }
                break;
  
            case LINK_ADR:              
            {
                const struct lora_link_adr_req *req = &cmd.fields.linkADRReq;
                
                LORA_DEBUG("link_adr_req: dataRate=%u txPower=%u chMask=%04x chMaskCntl=%u nbTrans=%u",
                    req->dataRate, req->txPower, req->channelMask, req->channelMaskControl, req->nbTrans)
                
                /* ensure only one ADR block is handled */
                if(adr_state != _NO_ADR){
                    
                    LORA_DEBUG("ignoring second run of ADR requests")
                    
                    if(!LDL_MAC_peekNextCommand(&s_in, &next_cmd) || (next_cmd != LINK_ADR)){
                     
                        struct lora_link_adr_ans ans = {
                            .dataRateOK = false,
                            .powerOK = false,
                            .channelMaskOK = false
                        };
                        
                        (void)LDL_MAC_putLinkADRAns(&s_out, &ans);
                    }                    
                }
                else{
                    
                    uint8_t i;
                    
                    if(LDL_Region_isDynamic(self->region)){
                        
                        switch(req->channelMaskControl){
                        case 0U:
                        
                            /* mask/unmask channels 0..15 */
                            for(i=0U; i < sizeof(req->channelMask)*8U; i++){
                                
                                if((req->channelMask & (1U << i)) > 0U){
                                    
                                    (void)unmaskChannel(shadow.chMask, self->region, i);
                                }
                                else{
                                    
                                    (void)maskChannel(shadow.chMask, self->region, i);
                                }
                            }
                            break;            
                            
                        case 6U:
                        
                            unmaskAllChannels(shadow.chMask, self->region);
                            break;           
                             
                        default:
                            adr_ans.channelMaskOK = false;
                            break;
                        }
                    }
                    else{
                        
                        switch(req->channelMaskControl){
                        case 6U:     /* all 125KHz on */
                        case 7U:     /* all 125KHz off */
                        
                            /* fixme: there is probably a more robust way to do this...right
                             * now we only support US and AU fixed channel plans so this works.
                             * */
                            for(i=0U; i < 64U; i++){
                                
                                if(req->channelMaskControl == 6U){
                                    
                                    (void)unmaskChannel(shadow.chMask, self->region, i);
                                }
                                else{
                                    
                                    (void)maskChannel(shadow.chMask, self->region, i);
                                }            
                            }                                  
                            break;
                            
                        default:
                            
                            for(i=0U; i < sizeof(req->channelMask)*8U; i++){
                                
                                if((req->channelMask & (1U << i)) > 0U){
                                    
                                    (void)unmaskChannel(shadow.chMask, self->region, (req->channelMaskControl * 16U) + i);
                                }
                                else{
                                    
                                    (void)maskChannel(shadow.chMask, self->region, (req->channelMaskControl * 16U) + i);
                                }
                            }
                            break;
                        }            
                    }
                    
                    if(!LDL_MAC_peekNextCommand(&s_in, &next_cmd) || (next_cmd != LINK_ADR)){
                     
                        if(self->ctx.adr){
                        
                            adr_ans.dataRateOK = true;
                            adr_ans.powerOK = true;
                         
                            /* nbTrans setting 0 means keep existing */
                            if(req->nbTrans > 0U){
                            
                                shadow.nbTrans = req->nbTrans;
                            }
                            
                            /* ignore rate setting 16 */
                            if(req->dataRate < 0xfU){            
                                
                                // todo: need to pin out of range to maximum
                                if(rateSettingIsValid(self->region, req->dataRate)){
                                
                                    shadow.rate = req->dataRate;            
                                }
                                else{
                                                    
                                    adr_ans.dataRateOK = false;
                                }
                            }
                            
                            /* ignore power setting 16 */
                            if(req->txPower < 0xfU){            
                                
                                int16_t dbm;
                                                    
                                // todo: need to pin out of range to maximum
                                if(LDL_Region_getTXPower(self->region, req->txPower, &dbm)){
                                
                                    shadow.power = req->txPower;        
                                }
                                else{
                                 
                                    adr_ans.powerOK = false;
                                }        
                            }   
                         
                            if(adr_ans.dataRateOK && adr_ans.powerOK && adr_ans.channelMaskOK){
                                
                                adr_state = _ADR_OK;
                            }
                            else{
                                
                                adr_state = _ADR_BAD;
                            }
                        }
                        /* if we are not ADRing then any ADR is bad! */
                        else{
                            
                            LORA_DEBUG("ignoring ADR since not in ADR mode")
                            
                            (void)memset(&adr_ans, 0, sizeof(adr_ans));                            
                            adr_state = _ADR_BAD;                            
                        }
                     
                        LORA_DEBUG("link_adr_ans: powerOK=%s dataRateOK=%s channelMaskOK=%s",
                            adr_ans.dataRateOK ? "true" : "false", 
                            adr_ans.powerOK ? "true" : "false", 
                            adr_ans.channelMaskOK ? "true" : "false"
                        )
                     
                        (void)LDL_MAC_putLinkADRAns(&s_out, &adr_ans);
                    }                
                }
            }
                break;
            
            case DUTY_CYCLE:
            {    
                const struct lora_duty_cycle_req *req = &cmd.fields.dutyCycleReq;
                
                LORA_DEBUG("duty_cycle_req: %u", req->maxDutyCycle)
                
                shadow.maxDutyCycle = req->maxDutyCycle;                        
                (void)LDL_MAC_putDutyCycleAns(&s_out);
            }
                break;
            
            case RX_PARAM_SETUP:     
            {
                const struct lora_rx_param_setup_req *req = &cmd.fields.rxParamSetupReq;
                struct lora_rx_param_setup_ans ans;
             
                LORA_DEBUG("rx_param_setup: rx1DROffset=%u rx2DataRate=%u freq=%"PRIu32,
                    req->rx1DROffset,
                    req->rx2DataRate,
                    req->freq
                )
                
                // todo: validation
                
                shadow.rx1DROffset = req->rx1DROffset;
                shadow.rx2DataRate = req->rx2DataRate;
                shadow.rx2Freq = req->freq;
                
                ans.rx1DROffsetOK = true;
                ans.rx2DataRateOK = true;
                ans.channelOK = true;       
                
                (void)LDL_MAC_putRXParamSetupAns(&s_out, &ans);
            }
                break;
            
            case DEV_STATUS:
            {
                LORA_DEBUG("dev_status_req")
                struct lora_dev_status_ans ans;        
                
                ans.battery = LDL_System_getBatteryLevel(self->app);
                ans.margin = (int8_t)self->margin;
                
                (void)LDL_MAC_putDevStatusAns(&s_out, &ans);
            }
                break;
                
            case NEW_CHANNEL:    
            
                LORA_DEBUG("new_channel_req: ")
                
                if(LDL_Region_isDynamic(self->region)){
                
                    struct lora_new_channel_ans ans;
                
                    ans.dataRateRangeOK = LDL_Region_validateRate(self->region, cmd.fields.newChannelReq.chIndex, cmd.fields.newChannelReq.minDR, cmd.fields.newChannelReq.maxDR);        
                    ans.channelFrequencyOK = LDL_Region_validateFreq(self->region, cmd.fields.newChannelReq.chIndex, cmd.fields.newChannelReq.freq);
                    
                    if(ans.dataRateRangeOK && ans.channelFrequencyOK){
                        
                        (void)setChannel(shadow.chConfig, self->region, cmd.fields.newChannelReq.chIndex, cmd.fields.newChannelReq.freq, cmd.fields.newChannelReq.minDR, cmd.fields.newChannelReq.maxDR);                        
                    }            
                    
                    (void)LDL_MAC_putNewChannelAns(&s_out, &ans);
                }
                break; 
                       
            case DL_CHANNEL:            
                
                LORA_INFO("handing dl_channel")
                
                if(LDL_Region_isDynamic(self->region)){
                    
                    struct lora_dl_channel_ans ans;
                    
                    ans.uplinkFreqOK = true;
                    ans.channelFrequencyOK = LDL_Region_validateFreq(self->region, cmd.fields.dlChannelReq.chIndex, cmd.fields.dlChannelReq.freq);
                    
                    (void)LDL_MAC_putDLChannelAns(&s_out, &ans);            
                }        
                break;
            
            case RX_TIMING_SETUP:
            {        
                LORA_INFO("handing rx_timing_setup")
                
                shadow.rx1Delay = cmd.fields.rxTimingSetupReq.delay;
                
                (void)LDL_MAC_putRXTimingSetupAns(&s_out);
            }
                break;
            
            case TX_PARAM_SETUP:        
                
                LORA_INFO("handing tx_param_setup")    
                break;
            }
            
            retval = (uint8_t)LDL_Stream_tell(&s_out);
        }
        else{
            
            break;
        }
    }

    /* roll back ADR request if not successful */
    if(adr_state == _ADR_BAD){
        
        LORA_DEBUG("bad ADR setting; rollback")
        
        (void)memcpy(shadow.chMask, self->ctx.chMask, sizeof(shadow.chMask));
        
        shadow.rate = self->ctx.rate;
        shadow.power = self->ctx.power;
        shadow.nbTrans = self->ctx.nbTrans;
    }

    /* otherwise apply changes */
    (void)memcpy(&self->ctx, &shadow, sizeof(self->ctx));  
    
    return retval;  
}

static void registerTime(struct lora_mac *self, uint32_t freq, uint32_t airTime)
{
    LORA_PEDANTIC(self != NULL)
    
    uint8_t band;
    
    if(LDL_Region_getBand(self->region, freq, &band)){
    
        if(LDL_Region_getOffTimeFactor(self->region, band) > 0U){
        
            LORA_PEDANTIC( band < (sizeof(self->band_ready)*8U) )
            
            self->band_ready &= ~(1U << band);
            LDL_Event_setTimer(&self->events, LORA_EVENT_BAND1 + band, ( airTime * (uint32_t)LDL_Region_getOffTimeFactor(self->region, band)));            
        }
    }
    
    if(self->ctx.maxDutyCycle > 0U){
        
        self->band_ready &= ~(1 << (LORA_EVENT_BAND_COMBINED - LORA_EVENT_BAND1));
        LDL_Event_setTimer(&self->events, LORA_EVENT_BAND_COMBINED, ( airTime * ( 1UL << (self->ctx.maxDutyCycle & 0xfU))));
    }
}    

static void addDefaultChannel(struct lora_mac *receiver, uint8_t chIndex, uint32_t freq, uint8_t minRate, uint8_t maxRate)
{
    (void)setChannel(receiver->ctx.chConfig, receiver->region, chIndex, freq, minRate, maxRate);
}

static bool selectChannel(const struct lora_mac *self, uint8_t rate, uint8_t prevChIndex, uint8_t *chIndex, uint32_t *freq)
{
    bool retval = false;
    uint8_t i;    
    uint8_t available = 0U;
    uint8_t j = 0U;
    uint8_t minRate;
    uint8_t maxRate;    
    uint8_t except = UINT8_MAX;
    
    if((self->band_ready & (1U << (LORA_EVENT_BAND_COMBINED - LORA_EVENT_BAND1))) > 0U){
    
        for(i=0U; i < LDL_Region_numChannels(self->region); i++){
            
            if(isAvailable(self, i, rate)){
            
                if(i == prevChIndex){
                    
                    except = i;
                }
            
                available++;            
            }            
        }
        
        if(available > 0U){
        
            uint8_t index;
        
            if(except != UINT8_MAX){
        
                if(available == 1U){
                    
                    except = UINT8_MAX;
                }
                else{
                    
                    available--;
                }
            }
        
            index = LDL_System_rand() % available;
            
            available = 0U;
            
            for(i=0U; i < LDL_Region_numChannels(self->region); i++){
            
                if(isAvailable(self, i, rate)){
                
                    if(except != i){
                
                        if(index == j){
                            
                            if(getChannel(self->ctx.chConfig, self->region, i, freq, &minRate, &maxRate)){
                                
                                *chIndex = i;
                                retval = true;
                                break;
                            }                        
                        }
                
                        j++;            
                    }
                }            
            }
        }
    }

    return retval;
}

static bool isAvailable(const struct lora_mac *self, uint8_t chIndex, uint8_t rate)
{
    bool retval = false;
    uint32_t freq;
    uint8_t minRate;    
    uint8_t maxRate;    
    uint8_t band;
    
    if(!channelIsMasked(self->ctx.chMask, self->region, chIndex)){
    
        if(getChannel(self->ctx.chConfig, self->region, chIndex, &freq, &minRate, &maxRate)){
            
            if((rate >= minRate) && (rate <= maxRate)){
            
                if(LDL_Region_getBand(self->region, freq, &band)){
                
                    if((self->band_ready & (1U << band)) > 0U){
                        
                        retval = true;
                    }
                }
            }
        }
    }
    
    return retval;
}

static uint32_t ticksUntilAvailable(const struct lora_mac *self, uint8_t chIndex, uint8_t rate)
{
    uint32_t retval = UINT32_MAX;
    uint32_t freq;
    uint8_t minRate;    
    uint8_t maxRate;    
    uint8_t band;
    
    if(!channelIsMasked(self->ctx.chMask, self->region, chIndex)){
    
        if(getChannel(self->ctx.chConfig, self->region, chIndex, &freq, &minRate, &maxRate)){
            
            if((rate >= minRate) && (rate <= maxRate)){
            
                if(LDL_Region_getBand(self->region, freq, &band)){
                
                    LORA_PEDANTIC( band < (sizeof(self->band_ready)*8U) )
                
                    if((self->band_ready & (1U << band)) > 0U){
                        
                        retval = 0U;
                    }
                    else{
                        
                        retval = LDL_Event_ticksUntil(&self->events, LORA_EVENT_BAND1 + band);                        
                    }                
                }
            }
        }
    }
    
    return retval;
}

static void restoreDefaults(struct lora_mac *self, bool keep)
{
    if(!keep){
        
        (void)memset(&self->ctx, 0, sizeof(self->ctx));
        self->ctx.rate = LORA_DEFAULT_RATE;    
        self->ctx.adr = true;        
    }
    else{
        
        self->ctx.up = 0U;
        self->ctx.down = 0U;
        (void)memset(self->ctx.chConfig, 0, sizeof(self->ctx.chConfig));
        (void)memset(self->ctx.chMask, 0, sizeof(self->ctx.chMask));        
        self->ctx.joined = false;        
    }
    
    LDL_Region_getDefaultChannels(self->region, self, addDefaultChannel);    
    
    self->ctx.rx1DROffset = LDL_Region_getRX1Offset(self->region);
    self->ctx.rx1Delay = LDL_Region_getRX1Delay(self->region);
    self->ctx.rx2DataRate = LDL_Region_getRX2Rate(self->region);
    self->ctx.rx2Freq = LDL_Region_getRX2Freq(self->region);    
}

static bool getChannel(const struct lora_mac_channel *self, enum lora_region region, uint8_t chIndex, uint32_t *freq, uint8_t *minRate, uint8_t *maxRate)
{
    bool retval = false;
    
    if(LDL_Region_isDynamic(region)){
        
        if(chIndex < LDL_Region_numChannels(region)){
            
            *freq = (self[chIndex].freqAndRate >> 8) * 100U;
            *minRate = (self[chIndex].freqAndRate >> 4) & 0xfU;
            *maxRate = self[chIndex].freqAndRate & 0xfU;
            
            retval = true;
        }        
    }
    else{
        
        retval = LDL_Region_getChannel(region, chIndex, freq, minRate, maxRate);                        
    }
     
    return retval;
}

static bool setChannel(struct lora_mac_channel *self, enum lora_region region, uint8_t chIndex, uint32_t freq, uint8_t minRate, uint8_t maxRate)
{
    bool retval = false;
    
    if(chIndex < LDL_Region_numChannels(region)){
        
        self[chIndex].freqAndRate = ((freq/100U) << 8) | ((minRate << 4) & 0xfU) | (maxRate & 0xfU);
        
        retval = true;
    }
    
    return retval;
}

static bool maskChannel(uint8_t *self, enum lora_region region, uint8_t chIndex)
{
    bool retval = false;
    
    if(chIndex < LDL_Region_numChannels(region)){
    
        self[chIndex / 8U] |= (1U << (chIndex % 8U));
        retval = true;
    }
    
    return retval;
}

static bool unmaskChannel(uint8_t *self, enum lora_region region, uint8_t chIndex)
{
    bool retval = false;
    
    if(chIndex < LDL_Region_numChannels(region)){
    
        self[chIndex / 8U] &= ~(1U << (chIndex % 8U));
        retval = true;
    }
    
    return retval;
}

static void unmaskAllChannels(uint8_t *self, enum lora_region region)
{
    uint8_t i;
    
    for(i=0U; i < LDL_Region_numChannels(region); i++){
        
        (void)unmaskChannel(self, region, i);
    }    
}

static bool channelIsMasked(const uint8_t *self, enum lora_region region, uint8_t chIndex)
{
    bool retval = false;
    
    if(chIndex < LDL_Region_numChannels(region)){
    
        retval = ((self[chIndex / 8U] & (1U << (chIndex % 8U))) > 0U);        
    }
    
    return retval;    
}

static bool rateSettingIsValid(enum lora_region region, uint8_t rate)
{
    bool retval = false;
    uint8_t i;
    
    for(i=0U; i < LDL_Region_numChannels(region); i++){
        
        if(LDL_Region_validateRate(region, i, rate, rate)){
            
            retval = true;
            break;
        }
    }
    
    return retval;
}

static uint32_t timeNow(struct lora_mac *self)
{
    uint32_t until;
    uint32_t retval;
    uint32_t error;
    
    const uint32_t max = (INT32_MAX/LDL_System_tps())*LDL_System_tps();
    
    until = LDL_Event_ticksUntil(&self->events, LORA_EVENT_TIME);
    
    if(until == 0UL){
        
        if(LDL_Event_checkTimer(&self->events, LORA_EVENT_TIME, &error)){
            
            self->time += max;
            LDL_Event_setTimer(&self->events, LORA_EVENT_TIME, max - error);
        }    
        
        retval = self->time;
    }
    else{
        
        retval = self->time + ((max - until)/LDL_System_tps());
    }
    
    return retval;
}

/* calculate a retry interval based on V1.1 chapter 7 */
static uint32_t retryInterval(struct lora_mac *self, uint32_t start_time)
{
    enum lora_spreading_factor sf;
    enum lora_signal_bandwidth bw;
    uint32_t delta;
    uint32_t interval;
    uint32_t tx_time;
    uint16_t dither;
    uint32_t next_channel;
    uint8_t mtu;
    
    LORA_PEDANTIC(start_time <= timeNow(self))
    
    delta = timeNow(self) - start_time;
    
    LORA_DEBUG("start_time = %"PRIu32"", start_time)
    LORA_DEBUG("%"PRIu32"s since join initiated", delta)
    
    dither = LDL_System_rand();
    dither <<= 8;
    dither |= LDL_System_rand();
    
    LDL_Region_convertRate(self->region, self->tx.rate, &sf, &bw, &mtu);
    
    tx_time = transmitTime(bw, sf, self->bufferLen, true);
    
    /* convert to ms */
    tx_time /= (LDL_System_tps() / 1000UL);
        
    /* 36/3600 (0.01) */
    if(delta < (60UL*60UL)){
        
        LORA_DEBUG("0.01 retry duty")
        
        interval = (50UL + (dither % 100UL)) * tx_time;                                                
    }
    /* 36/36000 (0.001) */
    else if(delta < (11UL*60UL*60UL)){
     
        LORA_DEBUG("0.001 retry duty")
     
        interval = (500UL + (dither % 1000UL)) * tx_time;                     
    }
    /* 8.7/86400 (0.0001) */
    else{
        
        LORA_DEBUG("0.0001 retry duty")
        
        interval = (5000UL + (dither % 10000UL)) * tx_time;                     
    }

    next_channel = (LDL_MAC_ticksUntilNextChannel(self) / (LDL_System_tps() / 1000UL)) + 1UL;

    if(interval < next_channel){
        
        interval = next_channel;
    }

    return (interval < next_channel) ? next_channel : interval;
}
