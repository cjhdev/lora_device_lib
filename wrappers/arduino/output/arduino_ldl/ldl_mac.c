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

#include "ldl_mac.h"
#include "ldl_radio.h"
#include "ldl_frame.h"
#include "ldl_debug.h"
#include "ldl_system.h"
#include "ldl_mac_commands.h"
#include "ldl_stream.h"
#include "ldl_sm_internal.h"
#include "ldl_ops.h"
#include <string.h>

enum {
    
    ADRAckLimit = 64U,
    ADRAckDelay = 32U,
    ADRAckTimeout = 2U
};

/* static function prototypes *****************************************/

static uint8_t extraSymbols(uint32_t xtal_error, uint32_t symbol_period);
static bool externalDataCommand(struct ldl_mac *self, bool confirmed, uint8_t port, const void *data, uint8_t len, const struct ldl_mac_data_opts *opts);
static bool dataCommand(struct ldl_mac *self, bool confirmed, uint8_t port, const void *data, uint8_t len);
static bool selectChannel(const struct ldl_mac *self, uint8_t rate, uint8_t prevChIndex, uint32_t limit, uint8_t *chIndex, uint32_t *freq);
static void registerTime(struct ldl_mac *self, uint32_t freq, uint32_t airTime);
static bool getChannel(const struct ldl_mac *self, uint8_t chIndex, uint32_t *freq, uint8_t *minRate, uint8_t *maxRate);
static bool isAvailable(const struct ldl_mac *self, uint8_t chIndex, uint8_t rate, uint32_t limit);
static uint32_t transmitTime(enum ldl_signal_bandwidth bw, enum ldl_spreading_factor sf, uint8_t size, bool crc);
static void restoreDefaults(struct ldl_mac *self, bool keep);
static bool setChannel(struct ldl_mac *self, uint8_t chIndex, uint32_t freq, uint8_t minRate, uint8_t maxRate);
static bool maskChannel(uint8_t *mask, uint8_t max, enum ldl_region region, uint8_t chIndex);
static bool unmaskChannel(uint8_t *mask, uint8_t max, enum ldl_region region, uint8_t chIndex);
static void unmaskAllChannels(uint8_t *mask, uint8_t max);
static bool channelIsMasked(const uint8_t *mask, uint8_t max, enum ldl_region region, uint8_t chIndex);
static uint32_t symbolPeriod(enum ldl_spreading_factor sf, enum ldl_signal_bandwidth bw);
static bool msUntilAvailable(const struct ldl_mac *self, uint8_t chIndex, uint8_t rate, uint32_t *ms);
static bool rateSettingIsValid(enum ldl_region region, uint8_t rate);
static void adaptRate(struct ldl_mac *self);
static uint32_t timeNow(struct ldl_mac *self);
static void processBands(struct ldl_mac *self);
static uint32_t nextBandEvent(const struct ldl_mac *self);
static void downlinkMissingHandler(struct ldl_mac *self);
static uint32_t ticksToMS(uint32_t ticks);
static uint32_t ticksToMSCoarse(uint32_t ticks);
static uint32_t msUntilNextChannel(const struct ldl_mac *self, uint8_t rate);
static uint32_t rand32(void *app);
static uint32_t getRetryDuty(uint32_t seconds_since);
static uint32_t timerDelta(uint32_t timeout, uint32_t time);
#ifndef LDL_DISABLE_SESSION_UPDATE
static void pushSessionUpdate(struct ldl_mac *self);
#endif
static void dummyResponseHandler(void *app, enum ldl_mac_response_type type, const union ldl_mac_response_arg *arg);

/* functions **********************************************************/

void LDL_MAC_init(struct ldl_mac *self, enum ldl_region region, const struct ldl_mac_init_arg *arg)
{
    LDL_PEDANTIC(self != NULL)
    LDL_PEDANTIC(arg != NULL)
    LDL_PEDANTIC(LDL_System_tps() >= 1000UL)
    
    LDL_ASSERT(arg->joinEUI != NULL)
    LDL_ASSERT(arg->devEUI != NULL)
    
    (void)memset(self, 0, sizeof(*self));
    
    self->tx.chIndex = UINT8_MAX;
    
    self->region = region;
    
    self->app = arg->app;    
    self->handler = arg->handler ? arg->handler : dummyResponseHandler;
    self->radio = arg->radio;
    self->sm = arg->sm;      
    self->devNonce = arg->devNonce;  
    self->joinNonce = arg->joinNonce;  
    self->gain = arg->gain;
    
    (void)memcpy(self->devEUI, arg->devEUI, sizeof(self->devEUI));
    (void)memcpy(self->joinEUI, arg->joinEUI, sizeof(self->joinEUI));
    
    if(self->radio != NULL){
        
        LDL_Radio_setHandler(self->radio, self, LDL_MAC_radioEvent);
    }
    else{
        
        LDL_INFO(self->app, "radio is undefined")
    }
    
    if(arg->session != NULL){
        
        (void)memcpy(&self->ctx, arg->session, sizeof(self->ctx));
    }
    else{
        
        restoreDefaults(self, false);
    }
        
     
#ifndef LDL_STARTUP_DELAY
#   define LDL_STARTUP_DELAY 0UL
#endif
    
    self->band[LDL_BAND_GLOBAL] = (uint32_t)LDL_STARTUP_DELAY;
    
    self->polled_band_ticks = LDL_System_ticks(self->app);
    self->polled_time_ticks = self->polled_band_ticks;
    
    LDL_Radio_reset(self->radio, false);

    /* leave reset line alone for 10ms */
    LDL_MAC_timerSet(self, LDL_TIMER_WAITA, (LDL_System_tps() + LDL_System_eps())/100UL);
    
    /* self->state is LDL_STATE_INIT */
}

enum ldl_mac_errno LDL_MAC_errno(const struct ldl_mac *self)
{
    LDL_PEDANTIC(self != NULL)
    
    return self->errno;
}

enum ldl_mac_operation LDL_MAC_op(const struct ldl_mac *self)
{
    LDL_PEDANTIC(self != NULL)
    
    return self->op;
}

enum ldl_mac_state LDL_MAC_state(const struct ldl_mac *self)
{
    LDL_PEDANTIC(self != NULL)
    
    return self->state;
}

bool LDL_MAC_unconfirmedData(struct ldl_mac *self, uint8_t port, const void *data, uint8_t len, const struct ldl_mac_data_opts *opts)
{    
    return externalDataCommand(self, false, port, data, len, opts);
}

bool LDL_MAC_confirmedData(struct ldl_mac *self, uint8_t port, const void *data, uint8_t len, const struct ldl_mac_data_opts *opts)
{    
    return externalDataCommand(self, true, port, data, len, opts);
}

bool LDL_MAC_otaa(struct ldl_mac *self)
{
    LDL_PEDANTIC(self != NULL)
    
    uint32_t delay;
    struct ldl_frame_join_request f;
    
    bool retval = false;
    
    self->errno = LDL_ERRNO_NONE;
    
    if(self->state == LDL_STATE_IDLE){
        
        if(self->ctx.joined){
            
            LDL_MAC_forget(self);
        }
        
        self->trials = 0U;
        
        self->tx.rate = LDL_Region_getJoinRate(self->region, self->trials);
        self->band[LDL_BAND_RETRY] = 0U;
        self->tx.power = 0U;
        
        if(self->band[LDL_BAND_GLOBAL] == 0UL){
        
            if(selectChannel(self, self->tx.rate, self->tx.chIndex, 0UL, &self->tx.chIndex, &self->tx.freq)){
            
                f.joinEUI = self->joinEUI;
                f.devEUI = self->devEUI;
                
#ifdef LDL_ENABLE_RANDOM_DEV_NONCE
                /* LoRAWAN 1.0 uses random nonce */
                self->devNonce = rand32(self->app);
#endif                                
                f.devNonce = self->devNonce;

                self->bufferLen = LDL_OPS_prepareJoinRequest(self, &f, self->buffer, sizeof(self->buffer));

                delay = rand32(self->app) % (60UL*LDL_System_tps());
                
                LDL_DEBUG(self->app, "sending join in %"PRIu32" ticks", delay)
                            
                LDL_MAC_timerSet(self, LDL_TIMER_WAITA, delay);
                
                self->state = LDL_STATE_WAIT_TX;
                self->op = LDL_OP_JOINING;            
                self->service_start_time = timeNow(self) + (delay / LDL_System_tps());            
                retval = true;        
            }
            else{
                
                self->errno = LDL_ERRNO_NOCHANNEL;
            }
        }
        else{
            
            self->errno = LDL_ERRNO_NOCHANNEL;
        }
    }
    else{
        
        self->errno = LDL_ERRNO_BUSY;
    }
    
    return retval;
}

bool LDL_MAC_joined(const struct ldl_mac *self)
{
    return self->ctx.joined;
}

void LDL_MAC_forget(struct ldl_mac *self)
{
    LDL_PEDANTIC(self != NULL)    
    
    LDL_MAC_cancel(self);    
    
    if(self->ctx.joined){
    
        restoreDefaults(self, true);    
        
#ifndef LDL_DISABLE_SESSION_UPDATE        
        pushSessionUpdate(self);   
#endif
    }
}

void LDL_MAC_cancel(struct ldl_mac *self)
{
    LDL_PEDANTIC(self != NULL)
    
    switch(self->state){
    case LDL_STATE_IDLE:
    case LDL_STATE_INIT_RESET:
    case LDL_STATE_INIT_LOCKOUT:
    case LDL_STATE_RECOVERY_RESET:    
    case LDL_STATE_RECOVERY_LOCKOUT:    
    case LDL_STATE_ENTROPY:    
        break;
    default:
        self->state = LDL_STATE_IDLE;
        LDL_Radio_sleep(self->radio);    
        break;
    }   
}

uint32_t LDL_MAC_transmitTimeUp(enum ldl_signal_bandwidth bw, enum ldl_spreading_factor sf, uint8_t size)
{
    return transmitTime(bw, sf, size, true);
}

uint32_t LDL_MAC_transmitTimeDown(enum ldl_signal_bandwidth bw, enum ldl_spreading_factor sf, uint8_t size)
{
    return transmitTime(bw, sf, size, false);
}

void LDL_MAC_process(struct ldl_mac *self)
{
    LDL_PEDANTIC(self != NULL)
    
    uint32_t error;    
    union ldl_mac_response_arg arg;

    (void)timeNow(self);    
    
    processBands(self);
    
    switch(self->state){
    default:
    case LDL_STATE_IDLE:
        /* do nothing */
        break;    
    case LDL_STATE_INIT:
    
        if(LDL_MAC_timerCheck(self, LDL_TIMER_WAITA, &error)){
    
            LDL_Radio_reset(self->radio, true);
        
            self->state = LDL_STATE_INIT_RESET;
            self->op = LDL_OP_RESET;
            
            /* hold reset for at least 100us */
            LDL_MAC_timerSet(self, LDL_TIMER_WAITA, ((LDL_System_tps() + LDL_System_eps())/10000UL) + 1UL);
            
#ifndef LDL_DISABLE_MAC_RESET_EVENT         
            self->handler(self->app, LDL_MAC_RESET, NULL); 
#endif                    
        }
        break;
        
    case LDL_STATE_INIT_RESET:
    case LDL_STATE_RECOVERY_RESET:
    
        if(LDL_MAC_timerCheck(self, LDL_TIMER_WAITA, &error)){
        
            LDL_Radio_reset(self->radio, false);
            
            self->op = LDL_OP_RESET;
            
            switch(self->state){
            default:
            case LDL_STATE_INIT_RESET:
                self->state = LDL_STATE_INIT_LOCKOUT;                
                /* 10ms */
                LDL_MAC_timerSet(self, LDL_TIMER_WAITA, ((LDL_System_tps() + LDL_System_eps())/100UL) + 1UL); 
                break;            
            case LDL_STATE_RECOVERY_RESET:
                self->state = LDL_STATE_RECOVERY_LOCKOUT;
                /* 60s */
                LDL_MAC_timerSet(self, LDL_TIMER_WAITA, (LDL_System_tps() + LDL_System_eps()) * 60UL);
                break;
            }            
        }    
        break;
        
    case LDL_STATE_INIT_LOCKOUT:
    case LDL_STATE_RECOVERY_LOCKOUT:
    
        if(LDL_MAC_timerCheck(self, LDL_TIMER_WAITA, &error)){
            
            self->op = LDL_OP_RESET;
            self->state = LDL_STATE_ENTROPY;
            
            LDL_Radio_entropyBegin(self->radio);                 
            
            /* 100us */
            LDL_MAC_timerSet(self, LDL_TIMER_WAITA, ((LDL_System_tps() + LDL_System_eps())/10000UL) + 1UL);
        }
        break;
            
    case LDL_STATE_ENTROPY:
    
        if(LDL_MAC_timerCheck(self, LDL_TIMER_WAITA, &error)){
    
            self->op = LDL_OP_RESET;
            
            arg.startup.entropy = LDL_Radio_entropyEnd(self->radio);                    
                
            self->state = LDL_STATE_IDLE;
            self->op = LDL_OP_NONE;
            
#ifndef LDL_DISABLE_MAC_STARTUP_EVENT            
            self->handler(self->app, LDL_MAC_STARTUP, &arg);        
#endif                                
        }
        break;
    
    case LDL_STATE_WAIT_TX:
    
        if(LDL_MAC_timerCheck(self, LDL_TIMER_WAITA, &error)){
            
            struct ldl_radio_tx_setting radio_setting;
            uint32_t tx_time;
            uint8_t mtu;
            
            LDL_Region_convertRate(self->region, self->tx.rate, &radio_setting.sf, &radio_setting.bw, &mtu);
            
            radio_setting.dbm = LDL_Region_getTXPower(self->region, self->tx.power) + self->gain;
            
            radio_setting.freq = self->tx.freq;
            
            tx_time = transmitTime(radio_setting.bw, radio_setting.sf, self->bufferLen, true);
            
            LDL_MAC_inputClear(self);  
            LDL_MAC_inputArm(self, LDL_INPUT_TX_COMPLETE);  
            
            LDL_Radio_transmit(self->radio, &radio_setting, self->buffer, self->bufferLen);

            registerTime(self, self->tx.freq, tx_time);
            
            self->state = LDL_STATE_TX;
            
            LDL_PEDANTIC((tx_time & 0x80000000UL) != 0x80000000UL)
            
            /* reset the radio if the tx complete interrupt doesn't appear after double the expected time */      
            LDL_MAC_timerSet(self, LDL_TIMER_WAITA, tx_time << 1UL);    
            
#ifndef LDL_DISABLE_TX_BEGIN_EVENT            
            arg.tx_begin.freq = self->tx.freq;
            arg.tx_begin.power = self->tx.power;
            arg.tx_begin.sf = radio_setting.sf;
            arg.tx_begin.bw = radio_setting.bw;
            arg.tx_begin.size = self->bufferLen;
            
            self->handler(self->app, LDL_MAC_TX_BEGIN, &arg);
#endif            
        }
        break;
        
    case LDL_STATE_TX:
    
        if(LDL_MAC_inputCheck(self, LDL_INPUT_TX_COMPLETE, &error)){
        
            LDL_MAC_inputClear(self);
        
            uint32_t waitSeconds;
            uint32_t waitTicks;    
            uint32_t advance;
            uint32_t advanceA;
            uint32_t advanceB;
            enum ldl_spreading_factor sf;
            enum ldl_signal_bandwidth bw;
            uint8_t rate;
            uint8_t extra_symbols;
            uint32_t xtal_error;
            uint8_t mtu;
            
            /* the wait interval is always measured in whole seconds */
            waitSeconds = (self->op == LDL_OP_JOINING) ? LDL_Region_getJA1Delay(self->region) : self->ctx.rx1Delay;    
            
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
                
                LDL_MAC_timerSet(self, LDL_TIMER_WAITB, waitTicks + (LDL_System_tps() + LDL_System_eps()) - advanceB);
                
                if(advanceA <= waitTicks){
                
                    LDL_MAC_timerSet(self, LDL_TIMER_WAITA, waitTicks - advanceA);
                    self->state = LDL_STATE_WAIT_RX1;
                }
                else{
                    
                    LDL_MAC_timerClear(self, LDL_TIMER_WAITA);
                    self->state = LDL_STATE_WAIT_RX2;
                }
            }
            else{
                
                self->state = LDL_STATE_WAIT_RX2;
                LDL_MAC_timerClear(self, LDL_TIMER_WAITA);
                LDL_MAC_timerSet(self, LDL_TIMER_WAITB, 0U);
                self->state = LDL_STATE_WAIT_RX2;                
            }    
            
            LDL_Radio_clearInterrupt(self->radio);
            
#ifndef LDL_DISABLE_TX_COMPLETE_EVENT            
            self->handler(self->app, LDL_MAC_TX_COMPLETE, NULL);                        
#endif            
        }
        else{
            
            if(LDL_MAC_timerCheck(self, LDL_TIMER_WAITA, &error)){
                
#ifndef LDL_DISABLE_CHIP_ERROR_EVENT                
                self->handler(self->app, LDL_MAC_CHIP_ERROR, NULL);                
#endif                
                LDL_MAC_inputClear(self);
                
                self->state = LDL_STATE_RECOVERY_RESET;
                self->op = LDL_OP_RESET;
                
                LDL_Radio_reset(self->radio, true);
                
                /* hold reset for at least 100us */
                LDL_MAC_timerSet(self, LDL_TIMER_WAITA, ((LDL_System_tps() + LDL_System_eps())/10000UL) + 1UL);
            }
        }
        break;
        
    case LDL_STATE_WAIT_RX1:
    
        if(LDL_MAC_timerCheck(self, LDL_TIMER_WAITA, &error)){
    
            struct ldl_radio_rx_setting radio_setting;
            uint32_t freq;    
            uint8_t rate;
            
            LDL_Region_getRX1DataRate(self->region, self->tx.rate, self->ctx.rx1DROffset, &rate);
            LDL_Region_getRX1Freq(self->region, self->tx.freq, self->tx.chIndex, &freq);    
                                    
            LDL_Region_convertRate(self->region, rate, &radio_setting.sf, &radio_setting.bw, &radio_setting.max);
            
            radio_setting.max += LDL_Frame_phyOverhead();
            
            self->state = LDL_STATE_RX1;
            
            if(error <= self->rx1_margin){
                
                radio_setting.freq = freq;
                radio_setting.timeout = self->rx1_symbols;
                
                LDL_MAC_inputClear(self);
                LDL_MAC_inputArm(self, LDL_INPUT_RX_READY);
                LDL_MAC_inputArm(self, LDL_INPUT_RX_TIMEOUT);
                
                LDL_Radio_receive(self->radio, &radio_setting);
                
                /* use waitA as a guard */
                LDL_MAC_timerSet(self, LDL_TIMER_WAITA, (LDL_System_tps()) << 4U);                                
            }
            else{
                
                self->state = LDL_STATE_WAIT_RX2;
            }
                
#ifndef LDL_DISABLE_SLOT_EVENT                           
            arg.rx_slot.margin = self->rx1_margin;                
            arg.rx_slot.timeout = self->rx1_symbols;                
            arg.rx_slot.error = error;
            arg.rx_slot.freq = freq;
            arg.rx_slot.bw = radio_setting.bw;
            arg.rx_slot.sf = radio_setting.sf;
                            
            self->handler(self->app, LDL_MAC_RX1_SLOT, &arg);                    
#endif                
        }
        break;
            
    case LDL_STATE_WAIT_RX2:
    
        if(LDL_MAC_timerCheck(self, LDL_TIMER_WAITB, &error)){
            
            struct ldl_radio_rx_setting radio_setting;
            
            LDL_Region_convertRate(self->region, self->ctx.rx2DataRate, &radio_setting.sf, &radio_setting.bw, &radio_setting.max);
            
            radio_setting.max += LDL_Frame_phyOverhead();
            
            self->state = LDL_STATE_RX2;
            
            if(error <= self->rx2_margin){
                
                radio_setting.freq = self->ctx.rx2Freq;
                radio_setting.timeout = self->rx2_symbols;
                
                LDL_MAC_inputClear(self);
                LDL_MAC_inputArm(self, LDL_INPUT_RX_READY);
                LDL_MAC_inputArm(self, LDL_INPUT_RX_TIMEOUT);
                
                LDL_Radio_receive(self->radio, &radio_setting);
                
                /* use waitA as a guard */
                LDL_MAC_timerSet(self, LDL_TIMER_WAITA, (LDL_System_tps()) << 4U);
            }
            else{
                
                adaptRate(self);
                
                self->state = LDL_STATE_IDLE;
            }
                
#ifndef LDL_DISABLE_SLOT_EVENT                                    
            arg.rx_slot.margin = self->rx2_margin;                
            arg.rx_slot.timeout = self->rx2_symbols;                
            arg.rx_slot.error = error;
            arg.rx_slot.freq = self->ctx.rx2Freq;
            arg.rx_slot.bw = radio_setting.bw;
            arg.rx_slot.sf = radio_setting.sf;           
                 
            self->handler(self->app, LDL_MAC_RX2_SLOT, &arg);                    
#endif                
        }
        break;
        
    case LDL_STATE_RX1:    
    case LDL_STATE_RX2:

        if(LDL_MAC_inputCheck(self, LDL_INPUT_RX_READY, &error)){
        
            LDL_MAC_inputClear(self);
    
            struct ldl_frame_down frame;
#ifdef LDL_ENABLE_STATIC_RX_BUFFER         
            uint8_t *buffer = self->rx_buffer;
#else   
            uint8_t buffer[LDL_MAX_PACKET];
#endif            
            uint8_t len;
            
            struct ldl_radio_packet_metadata meta;            
            uint8_t cmd_len = 0U;
            
            const uint8_t *fopts;
            uint8_t foptsLen;
            
            LDL_MAC_timerClear(self, LDL_TIMER_WAITA);
            LDL_MAC_timerClear(self, LDL_TIMER_WAITB);
            
            len = LDL_Radio_collect(self->radio, &meta, buffer, LDL_MAX_PACKET);        
            
            LDL_Radio_clearInterrupt(self->radio);
            
            /* notify of a downstream message */
#ifndef LDL_DISABLE_DOWNSTREAM_EVENT            
            arg.downstream.rssi = meta.rssi;
            arg.downstream.snr = meta.snr;
            arg.downstream.size = len;
            
            self->handler(self->app, LDL_MAC_DOWNSTREAM, &arg);      
#endif  
            self->margin = meta.snr;
                      
            if(LDL_OPS_receiveFrame(self, &frame, buffer, len)){
                
                self->last_valid_downlink = timeNow(self);
                
                switch(frame.type){
                default:
                case FRAME_TYPE_JOIN_ACCEPT:

                    restoreDefaults(self, true);
                    
                    self->ctx.joined = true;
                    
                    if(self->ctx.adr){
                        
                        /* keep the joining rate */
                        self->ctx.rate = self->tx.rate;
                    }                
                    
                    self->ctx.rx1DROffset = frame.rx1DataRateOffset;
                    self->ctx.rx2DataRate = frame.rx2DataRate;
                    self->ctx.rx1Delay = frame.rxDelay;
                    
                    if(frame.cfList != NULL){
                        
                        LDL_Region_processCFList(self->region, self, frame.cfList, frame.cfListLen);                        
                    }
                    
                    self->ctx.devAddr = frame.devAddr;    
                    self->ctx.netID = frame.netID;
                    self->joinNonce = frame.joinNonce;
                    
                    self->ctx.version = (frame.optNeg) ? 1U : 0U;
                    
                    self->rekeyConf_pending = (self->ctx.version > 0U) ? true : false;
                    
                    LDL_OPS_deriveKeys(self);
                    
                    self->devNonce++;
                    
#ifndef LDL_DISABLE_JOIN_COMPLETE_EVENT                    
                    arg.join_complete.joinNonce = self->joinNonce;
                    arg.join_complete.nextDevNonce = self->devNonce;
                    arg.join_complete.netID = self->ctx.netID;
                    arg.join_complete.devAddr = self->ctx.devAddr;
                    self->handler(self->app, LDL_MAC_JOIN_COMPLETE, NULL);                    
#endif                                      
                    self->state = LDL_STATE_IDLE;           
                    self->op = LDL_OP_NONE;                                
                    break;
                
                case FRAME_TYPE_DATA_UNCONFIRMED_DOWN:
                case FRAME_TYPE_DATA_CONFIRMED_DOWN:
                                
                    LDL_OPS_syncDownCounter(self, frame.port, frame.counter);
                        
                    self->adrAckCounter = 0U;
                    self->rxParamSetupAns_pending = false;    
                    self->dlChannelAns_pending = false;
                    self->rxtimingSetupAns_pending = false;
                    self->adrAckReq = false;
                        
                    fopts = frame.opts;
                    foptsLen = frame.optsLen;
                                        
                    if((frame.data != NULL) && (frame.port == 0U)){
                    
                        if(frame.port == 0U){
                    
                            fopts = frame.data;
                            foptsLen = frame.dataLen;                        
                        }
                        else{
                            
                            /* this MUST come before processing MAC commands
                             * because the MAC command response will clobber
                             * the RX buffer (i.e. what frame.data points to) */
#ifndef LDL_DISABLE_RX_EVENT                                            
                            arg.rx.counter = frame.counter;
                            arg.rx.port = frame.port;
                            arg.rx.data = frame.data;
                            arg.rx.size = frame.dataLen;                                            
                            
                            self->handler(self->app, LDL_MAC_RX, &arg);                                        
#endif                                                   
                        }
                    }
                    
                    /* Process MAC commands
                     * 
                     * This MUST come after handling LDL_MAC_RX or else
                     * you will clobber the RX buffer with the MAC response.
                     * 
                     * Yes it seems crazy to dump this here instead
                     * of shifting it to a static function. Trying this
                     * out because constrained targets are running
                     * out of stack.
                     * 
                     * */
                    if(foptsLen > 0U){
                        
                        uint8_t pos = 0U;
                        
                        struct ldl_stream s_in;
                        struct ldl_stream s_out;
                        
                        /* rollback cache */
                        uint8_t chMask[sizeof(self->ctx.chMask)];
                        uint8_t nbTrans;
                        uint8_t power;
                        uint8_t rate;
                        
                        struct ldl_downstream_cmd cmd;                                                
                        enum ldl_mac_cmd_type next_cmd;
                        
                        enum {
                            
                            _NO_ADR,
                            _ADR_OK,
                            _ADR_BAD
                            
                        } adr_state = _NO_ADR;
                        
                        /* these things can be rolled back */
                        nbTrans = self->ctx.nbTrans;
                        power = self->ctx.power;
                        rate = self->ctx.rate;
                        (void)memcpy(&chMask, self->ctx.chMask, sizeof(chMask));
                        
                        LDL_Stream_initReadOnly(&s_in, fopts, foptsLen);
                        LDL_Stream_init(&s_out, buffer, LDL_MAX_PACKET);
                        
                        /* this is up here because adr may be spread over
                         * multiple commands */
                        struct ldl_link_adr_ans adr_ans;
                        
                        adr_ans.channelMaskOK = true;
                        
                        while(LDL_MAC_getDownCommand(&s_in, &cmd)){
                            
                            /* we use this position to ensure only whole
                             * MAC responses are sent */
                            pos = LDL_Stream_tell(&s_out);
                            
                            switch(cmd.type){
                            default:
                                LDL_DEBUG(self->app, "not handling type %u", cmd.type)
                                break;     
#ifndef LDL_DISABLE_CHECK                                   
                            case LDL_CMD_LINK_CHECK:    
                            {                
                                const struct ldl_link_check_ans *ans = &cmd.fields.linkCheck;
                                
                                arg.link_status.margin = ans->margin;
                                arg.link_status.gwCount = ans->gwCount;            
                                
                                LDL_DEBUG(self->app, "link_check_ans: margin=%u gwCount=%u", 
                                    ans->margin,
                                    ans->gwCount             
                                )
                                
                                self->handler(self->app, LDL_MAC_LINK_STATUS, &arg);                                                             
                            }
                                break;
#endif                            
                            case LDL_CMD_LINK_ADR:              
                            {
                                const struct ldl_link_adr_req *req = &cmd.fields.linkADR;
                                
                                LDL_DEBUG(self->app, "link_adr_req: dataRate=%u txPower=%u chMask=%04x chMaskCntl=%u nbTrans=%u",
                                    req->dataRate, req->txPower, req->channelMask, req->channelMaskControl, req->nbTrans)
                                
                                uint8_t i;
                                
                                if(LDL_Region_isDynamic(self->region)){
                                    
                                    switch(req->channelMaskControl){
                                    case 0U:
                                    
                                        /* mask/unmask channels 0..15 */
                                        for(i=0U; i < (sizeof(req->channelMask)*8U); i++){
                                            
                                            if((req->channelMask & (1U << i)) > 0U){
                                                
                                                (void)unmaskChannel(self->ctx.chMask, sizeof(self->ctx.chMask), self->region, i);
                                            }
                                            else{
                                                
                                                (void)maskChannel(self->ctx.chMask, sizeof(self->ctx.chMask), self->region, i);
                                            }
                                        }
                                        break;            
                                        
                                    case 6U:
                                    
                                        unmaskAllChannels(self->ctx.chMask, sizeof(self->ctx.chMask));
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
                                                
                                                (void)unmaskChannel(self->ctx.chMask, sizeof(self->ctx.chMask), self->region, i);
                                            }
                                            else{
                                                
                                                (void)maskChannel(self->ctx.chMask, sizeof(self->ctx.chMask), self->region, i);
                                            }            
                                        }                                  
                                        break;
                                        
                                    default:
                                        
                                        for(i=0U; i < (sizeof(req->channelMask)*8U); i++){
                                            
                                            if((req->channelMask & (1U << i)) > 0U){
                                                
                                                (void)unmaskChannel(self->ctx.chMask, sizeof(self->ctx.chMask), self->region, (req->channelMaskControl * 16U) + i);
                                            }
                                            else{
                                                
                                                (void)maskChannel(self->ctx.chMask, sizeof(self->ctx.chMask), self->region, (req->channelMaskControl * 16U) + i);
                                            }
                                        }
                                        break;
                                    }            
                                }
                                
                                if(!LDL_MAC_peekNextCommand(&s_in, &next_cmd) || (next_cmd != LDL_CMD_LINK_ADR)){
                                 
                                    adr_ans.dataRateOK = true;
                                    adr_ans.powerOK = true;
                                 
                                    /* nbTrans setting 0 means keep existing */
                                    if(req->nbTrans > 0U){
                                    
                                        self->ctx.nbTrans = req->nbTrans & 0xfU;
                                        
                                        if(self->ctx.nbTrans > LDL_REDUNDANCY_MAX){
                                            
                                            self->ctx.nbTrans = LDL_REDUNDANCY_MAX;
                                        }
                                    }
                                    
                                    /* ignore rate setting 16 */
                                    if(req->dataRate < 0xfU){            
                                        
                                        // todo: need to pin out of range to maximum
                                        if(rateSettingIsValid(self->region, req->dataRate)){
                                        
                                            self->ctx.rate = req->dataRate;            
                                        }
                                        else{
                                                            
                                            adr_ans.dataRateOK = false;
                                        }
                                    }
                                    
                                    /* ignore power setting 16 */
                                    if(req->txPower < 0xfU){            
                                        
                                        if(LDL_Region_validateTXPower(self->region, req->txPower)){
                                        
                                            self->ctx.power = req->txPower;        
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
                                   
                                 
                                    LDL_DEBUG(self->app, "link_adr_ans: powerOK=%s dataRateOK=%s channelMaskOK=%s",
                                        adr_ans.dataRateOK ? "true" : "false", 
                                        adr_ans.powerOK ? "true" : "false", 
                                        adr_ans.channelMaskOK ? "true" : "false"
                                    )
                                 
                                    LDL_MAC_putLinkADRAns(&s_out, &adr_ans);
                                }                
                                
                            }
                                break;
                            
                            case LDL_CMD_DUTY_CYCLE:
                            {    
                                const struct ldl_duty_cycle_req *req = &cmd.fields.dutyCycle;
                                
                                LDL_DEBUG(self->app, "duty_cycle_req: %u", req->maxDutyCycle)
                            
                                LDL_DEBUG(self->app, "duty_cycle_ans")
                                
                                self->ctx.maxDutyCycle = req->maxDutyCycle;                        
                                LDL_MAC_putDutyCycleAns(&s_out);
                            }
                                break;
                            
                            case LDL_CMD_RX_PARAM_SETUP:     
                            {
                                const struct ldl_rx_param_setup_req *req = &cmd.fields.rxParamSetup;
                                struct ldl_rx_param_setup_ans ans;
                             
                                LDL_DEBUG(self->app, "rx_param_setup_req: rx1DROffset=%u rx2DataRate=%u freq=%"PRIu32,
                                    req->rx1DROffset,
                                    req->rx2DataRate,
                                    req->freq
                                )
                                
                                // todo: validation
                                
                                self->ctx.rx1DROffset = req->rx1DROffset;
                                self->ctx.rx2DataRate = req->rx2DataRate;
                                self->ctx.rx2Freq = req->freq;
                                
                                ans.rx1DROffsetOK = true;
                                ans.rx2DataRateOK = true;
                                ans.channelOK = true;       
                                
                                LDL_DEBUG(self->app, "rx_param_setup_ans: rx1DROffsetOK=%s rx2DataRate=%s rx2Freq=%s",                
                                    ans.rx1DROffsetOK ? "true" : "false",
                                    ans.rx2DataRateOK ? "true" : "false",
                                    ans.channelOK ? "true" : "false"       
                                )
                                
                                LDL_MAC_putRXParamSetupAns(&s_out, &ans);
                            }
                                break;
                            
                            case LDL_CMD_DEV_STATUS:
                            {
                                LDL_DEBUG(self->app, "dev_status_req")
                                struct ldl_dev_status_ans ans;        
                                
                                ans.battery = LDL_System_getBatteryLevel(self->app);
                                ans.margin = (int8_t)self->margin;
                                
                                LDL_DEBUG(self->app, "dev_status_ans: battery=%u margin=%u",
                                    ans.battery,
                                    ans.margin
                                )
                                 
                                LDL_MAC_putDevStatusAns(&s_out, &ans);
                            }
                                break;
                                
                            case LDL_CMD_NEW_CHANNEL:    
                                        
                                LDL_DEBUG(self->app, "new_channel_req: chIndex=%u freq=%"PRIu32" maxDR=%u minDR=%u",
                                    cmd.fields.newChannel.chIndex,
                                    cmd.fields.newChannel.freq,
                                    cmd.fields.newChannel.maxDR,
                                    cmd.fields.newChannel.minDR
                                )
                            
                                if(LDL_Region_isDynamic(self->region)){
                                
                                    struct ldl_new_channel_ans ans;
                                
                                    ans.dataRateRangeOK = LDL_Region_validateRate(self->region, cmd.fields.newChannel.chIndex, cmd.fields.newChannel.minDR, cmd.fields.newChannel.maxDR);        
                                    ans.channelFreqOK = LDL_Region_validateFreq(self->region, cmd.fields.newChannel.chIndex, cmd.fields.newChannel.freq);
                                    
                                    if(ans.dataRateRangeOK && ans.channelFreqOK){
                                        
                                        (void)setChannel(self, cmd.fields.newChannel.chIndex, cmd.fields.newChannel.freq, cmd.fields.newChannel.minDR, cmd.fields.newChannel.maxDR);                        
                                    }            
                                 
                                    LDL_DEBUG(self->app, "new_channel_ans: dataRateRangeOK=%s channelFreqOK=%s", 
                                        ans.dataRateRangeOK ? "true" : "false",
                                        ans.channelFreqOK ? "true" : "false"
                                    )
                                    
                                    LDL_MAC_putNewChannelAns(&s_out, &ans);
                                }
                                else{
                                    
                                    LDL_DEBUG(self->app, "new_channel_req not processed in this region")                
                                }
                                break; 
                                       
                            case LDL_CMD_DL_CHANNEL:            
                                
                                LDL_DEBUG(self->app, "dl_channel_req: chIndex=%u freq=%"PRIu32,
                                    cmd.fields.dlChannel.chIndex,
                                    cmd.fields.dlChannel.freq
                                )

                                if(LDL_Region_isDynamic(self->region)){

                                    struct ldl_dl_channel_ans ans;
                                    
#ifdef LDL_DISABLE_CMD_DL_CHANNEL                            
                                    ans.uplinkFreqOK = false;
                                    ans.channelFreqOK = false;
#else
                                    ans.uplinkFreqOK = true;
                                    ans.channelFreqOK = LDL_Region_validateFreq(self->region, cmd.fields.dlChannel.chIndex, cmd.fields.dlChannel.freq);
                                    
                                    LDL_DEBUG(self->app, "dl_channel_ans: uplinkFreqOK=%s channelFreqOK=%s",
                                        ans.uplinkFreqOK ? "true" : "false",
                                        ans.channelFreqOK ? "true" : "false"
                                    )
#endif                
                                    LDL_MAC_putDLChannelAns(&s_out, &ans);            
                                }
                                else{
                                    
                                    LDL_DEBUG(self->app, "dl_channel_req not processed in this region")                
                                }
                                break;
                            
                            case LDL_CMD_RX_TIMING_SETUP:
                            {        
                                LDL_DEBUG(self->app, "rx_timing_setup_req: delay=%u",
                                    cmd.fields.rxTimingSetup.delay
                                )
                                
                                self->ctx.rx1Delay = cmd.fields.rxTimingSetup.delay;
                                
                                LDL_DEBUG(self->app, "rx_timing_setup_ans")
                                
                                LDL_MAC_putRXTimingSetupAns(&s_out);
                            }
                                break;
                            
                            case LDL_CMD_TX_PARAM_SETUP:        
                                             
                                LDL_DEBUG(self->app, "tx_param_setup_req: downlinkDwellTime=%s uplinkDwellTime=%s maxEIRP=%u",
                                    cmd.fields.txParamSetup.downlinkDwell ? "true" : "false",
                                    cmd.fields.txParamSetup.uplinkDwell ? "true" : "false",
                                    cmd.fields.txParamSetup.maxEIRP
                                )    
                            
                                if(LDL_Region_isDynamic(self->region)){
                                    
                                    LDL_DEBUG(self->app, "tx_param_setup_ans")
                                }
                                else{
                                    
                                    LDL_DEBUG(self->app, "tx_param_setup_req not processed in this region")
                                }
                                break;
                             
#ifndef LDL_DISABLE_DEVICE_TIME
                            case LDL_CMD_DEVICE_TIME:
                            {
                                arg.device_time.seconds = cmd.fields.deviceTime.seconds;
                                arg.device_time.fractions = cmd.fields.deviceTime.fractions;
                                
                                LDL_DEBUG(self->app, "device_time_ans: seconds=%"PRIu32" fractions=%u", 
                                    arg.device_time.seconds,
                                    arg.device_time.fractions             
                                )
                                
                                self->handler(self->app, LDL_MAC_DEVICE_TIME, &arg);                                                                                     
                            }
                                break;
#endif            
                            case LDL_CMD_ADR_PARAM_SETUP:
                            
                                LDL_DEBUG(self->app, "adr_param_setup: limit_exp=%u delay_exp=%u", 
                                    cmd.fields.adrParamSetup.limit_exp,
                                    cmd.fields.adrParamSetup.delay_exp        
                                )
                                
                                self->ctx.adr_ack_limit = (1U << cmd.fields.adrParamSetup.limit_exp);
                                self->ctx.adr_ack_delay = (1U << cmd.fields.adrParamSetup.delay_exp);
                                
                                LDL_DEBUG(self->app, "adr_param_ans")
                                
                                LDL_MAC_putADRParamSetupAns(&s_out);
                                break;
                                
                            case LDL_CMD_REKEY:
                            
                                LDL_DEBUG(self->app, "rekey_conf: version=%u", cmd.fields.rekey.version)
                            
                                /* The server version must be greater than 0 (0 is not allowed), and smaller or equal (<=) to the
                                 * device’s LoRaWAN version. Therefore for a LoRaWAN1.1 device the only valid value is 1. If
                                 * the server’s version is invalid the device SHALL discard the RekeyConf command and
                                 * retransmit the RekeyInd in the next uplink frame */
                                if(cmd.fields.rekey.version == 1U){
                                    
                                    self->rekeyConf_pending = false;
                                }
                                break;
                                
                            case LDL_CMD_FORCE_REJOIN:
                                    
                                LDL_DEBUG(self->app, "force_rejoin_req: max_retries=%u rejoin_type=%u period=% dr=%u",
                                    cmd.fields.forceRejoin.max_retries,
                                    cmd.fields.forceRejoin.rejoin_type,
                                    cmd.fields.forceRejoin.period,
                                    cmd.fields.forceRejoin.dr
                                )
                            
                                /* no reply - you are meant to start doing a rejoin */
                                    
                                break;
                                
                            case LDL_CMD_REJOIN_PARAM_SETUP:
                            
                                LDL_DEBUG(self->app, "rejoin_param_setup_req: maxTimeN=%u maxCountN=%u",
                                    cmd.fields.rejoinParamSetup.maxTimeN,
                                    cmd.fields.rejoinParamSetup.maxCountN
                                )
                                
                                {
                                    struct ldl_rejoin_param_setup_ans ans;
                                    ans.timeOK = false;
                                    
                                    LDL_DEBUG(self->app, "rejoin_param_setup_ans: timeOK=%s", ans.timeOK ? "true" : "false")
                                    
                                    LDL_MAC_putRejoinParamSetupAns(&s_out, &ans);
                                }
                            
                                break;        
                            }
                            
                            /* this ensures the output stream doesn't contain part of a MAC command */
                            if(LDL_Stream_error(&s_out)){
                                
                                (void)LDL_Stream_seekSet(&s_out, pos);
                            }        
                        }
                        
                        /* roll back ADR request if not successful */
                        if(adr_state == _ADR_BAD){
                            
                            LDL_DEBUG(self->app, "bad ADR setting; rollback")
                            
                            (void)memcpy(self->ctx.chMask, chMask, sizeof(self->ctx.chMask));
                            
                            self->ctx.rate = rate;
                            self->ctx.power = power;
                            self->ctx.nbTrans = nbTrans;
                        }

                        /* return the length of data written to the output buffer */
                        cmd_len = LDL_Stream_tell(&s_out);
                    }
                    
                    switch(self->op){
                    default:
                    case LDL_OP_DATA_UNCONFIRMED:
#ifndef LDL_DISABLE_DATA_COMPLETE_EVENT
                        self->handler(self->app, LDL_MAC_DATA_COMPLETE, NULL);
#endif              
                        break;
                    case LDL_OP_DATA_CONFIRMED:
                    
                        if(frame.ack){
#ifndef LDL_DISABLE_DATA_COMPLETE_EVENT
                            self->handler(self->app, LDL_MAC_DATA_COMPLETE, NULL);
#endif                            
                        }
                        else{
                            
                            /* fixme: not handling this correctly */
                            
#ifndef LDL_DISABLE_DATA_NAK_EVENT
                            self->handler(self->app, LDL_MAC_DATA_COMPLETE, NULL);
#endif                                                 
                        }
                        break;                    
                    
                    case LDL_OP_REJOINING:
                        break;
                    }
                      
                    /* respond to MAC command 
                     * 
                     * fixme: we don't bother if we are rejoining since that requires some refactoring here 
                     * 
                     * fixme: would be good to use the common dataCommand somehow
                     * 
                     * */
                    if((cmd_len > 0U) && (self->op != LDL_OP_REJOINING)){

                        LDL_DEBUG(self->app, "sending mac response")
                        
                        self->tx.rate = self->ctx.rate;
                        self->tx.power = self->ctx.power;
                    
                        uint32_t ms_until_next = msUntilNextChannel(self, self->tx.rate);
                    
                        /* MAC command may have masked everything... */
                        if(ms_until_next != UINT32_MAX){
                    
                            struct ldl_frame_data f;
                            
                            (void)memset(&f, 0, sizeof(f));
                        
                            f.devAddr = self->ctx.devAddr;
                            f.counter = self->ctx.up;
                            f.adr = self->ctx.adr;
                            f.adrAckReq = self->adrAckReq;
                            f.type = FRAME_TYPE_DATA_UNCONFIRMED_UP;
                            
                            if(cmd_len <= 15U){
                                
                                f.opts = buffer;
                                f.optsLen = cmd_len;
                            }
                            else{
                                
                                f.data = buffer;
                                f.dataLen = cmd_len;
                            }
                            
                            self->bufferLen = LDL_OPS_prepareData(self, &f, self->buffer, sizeof(self->buffer));
                            
                            self->ctx.up++;
                            
                            self->op = LDL_OP_DATA_UNCONFIRMED;
                            self->state = LDL_STATE_WAIT_SEND;
                            self->band[LDL_BAND_RETRY] = ms_until_next;                        
                        }
                        else{
                            
                            LDL_DEBUG(self->app, "cannot send, all channels are masked!")
                            
                            self->state = LDL_STATE_IDLE;           
                            self->op = LDL_OP_NONE;
                        }
                    }
                    else{
                        
                        self->state = LDL_STATE_IDLE;           
                        self->op = LDL_OP_NONE;
                    }
                    
                    
                    break;
                }
                
#ifndef LDL_DISABLE_SESSION_UPDATE
                pushSessionUpdate(self);    
#endif                
            }
            else{
                
                downlinkMissingHandler(self);
            }
        }
        else if(LDL_MAC_inputCheck(self, LDL_INPUT_RX_TIMEOUT, &error)){
            
            LDL_MAC_inputClear(self);
            
            LDL_Radio_clearInterrupt(self->radio);
            
            if(self->state == LDL_STATE_RX2){
            
                LDL_MAC_timerClear(self, LDL_TIMER_WAITB);
                
                uint8_t mtu;
                enum ldl_spreading_factor sf;
                enum ldl_signal_bandwidth bw;
                
                LDL_Region_convertRate(self->region, self->tx.rate, &sf, &bw, &mtu);                        
                
                LDL_MAC_timerSet(self, LDL_TIMER_WAITA, transmitTime(bw, sf, mtu, false));
                
                self->state = LDL_STATE_RX2_LOCKOUT;
            }
            else{
                
                LDL_MAC_timerClear(self, LDL_TIMER_WAITA);
                
                self->state = LDL_STATE_WAIT_RX2;
            }   
        }
        else{
            
            /* this is a hardware failure condition */
            if(LDL_MAC_timerCheck(self, LDL_TIMER_WAITA, &error) || LDL_MAC_timerCheck(self, LDL_TIMER_WAITB, &error)){
                
#ifndef LDL_DISABLE_CHIP_ERROR_EVENT                
                self->handler(self->app, LDL_MAC_CHIP_ERROR, NULL); 
#endif                
                LDL_MAC_inputClear(self);
                LDL_MAC_timerClear(self, LDL_TIMER_WAITA);
                LDL_MAC_timerClear(self, LDL_TIMER_WAITB);
                
                self->state = LDL_STATE_RECOVERY_RESET;
                self->op = LDL_OP_RESET;
                
                LDL_Radio_reset(self->radio, true);
                
                /* hold reset for at least 100us */
                LDL_MAC_timerSet(self, LDL_TIMER_WAITA, ((LDL_System_tps() + LDL_System_eps())/10000UL) + 1U);                     
            }
        }
        break;
    
    case LDL_STATE_RX2_LOCKOUT:
    
        if(LDL_MAC_timerCheck(self, LDL_TIMER_WAITA, &error)){
            
            downlinkMissingHandler(self);  
            
#ifndef LDL_DISABLE_SESSION_UPDATE              
            pushSessionUpdate(self);
#endif            
        }
        break;
    
    case LDL_STATE_WAIT_RETRY:
    case LDL_STATE_WAIT_SEND:
        
        if(self->band[LDL_BAND_RETRY] == 0U){
            
            if(msUntilNextChannel(self, self->tx.rate) != UINT32_MAX){
            
                if(self->band[LDL_BAND_GLOBAL] == 0UL){
            
                    if(selectChannel(self, self->tx.rate, self->tx.chIndex, 0UL, &self->tx.chIndex, &self->tx.freq)){
                                
                        uint32_t delay = (self->state == LDL_STATE_WAIT_SEND) ? 0UL : (rand32(self->app) % (LDL_System_tps()*30UL));
                                
                        LDL_DEBUG(self->app, "dither retry by %"PRIu32" ticks", delay)
                                
                        LDL_MAC_timerSet(self, LDL_TIMER_WAITA, delay);
                        self->state = LDL_STATE_WAIT_TX;
                    }            
                }
            }
            else{
                
                LDL_DEBUG(self->app, "no channels for retry")
                
                self->op = LDL_OP_NONE;
                self->state = LDL_STATE_IDLE;
            }
        }
        break;    
    }
    
    {
        uint32_t next = nextBandEvent(self);     
            
        if(next < ticksToMSCoarse(60UL*LDL_System_tps())){
            
            LDL_MAC_timerSet(self, LDL_TIMER_BAND, LDL_System_tps() / 1000UL * (next+1U));                    
        }
        else{
        
            LDL_MAC_timerSet(self, LDL_TIMER_BAND, 60UL*LDL_System_tps());                    
        }
    }       
}

uint32_t LDL_MAC_ticksUntilNextEvent(const struct ldl_mac *self)
{
    LDL_PEDANTIC(self != NULL)
    
    uint32_t retval = 0UL;
    
    if(!LDL_MAC_inputPending(self)){
     
        retval = LDL_MAC_timerTicksUntilNext(self);    
    }
    
    return retval;
}

bool LDL_MAC_setRate(struct ldl_mac *self, uint8_t rate)
{
    LDL_PEDANTIC(self != NULL)
    
    bool retval = false;    
    self->errno = LDL_ERRNO_NONE;
    
    if(rateSettingIsValid(self->region, rate)){
        
        self->ctx.rate = rate;
        
#ifndef LDL_DISABLE_SESSION_UPDATE        
        pushSessionUpdate(self);
#endif        
        retval = true;        
    }
    else{
        
        self->errno = LDL_ERRNO_RATE;
    }
    
    return retval;
}

uint8_t LDL_MAC_getRate(const struct ldl_mac *self)
{
    LDL_PEDANTIC(self != NULL)
    
    return self->ctx.rate;
}

bool LDL_MAC_setPower(struct ldl_mac *self, uint8_t power)
{
    LDL_PEDANTIC(self != NULL)
    
    bool retval = false;
    self->errno = LDL_ERRNO_NONE;
        
    if(LDL_Region_validateTXPower(self->region, power)){
        
        self->ctx.power = power;
#ifndef LDL_DISABLE_SESSION_UPDATE        
        pushSessionUpdate(self);        
#endif        
        retval = true;
    }
    else{
     
        self->errno = LDL_ERRNO_POWER;
    }        
    
    return retval;
}

uint8_t LDL_MAC_getPower(const struct ldl_mac *self)
{
    LDL_PEDANTIC(self != NULL)
    
    return self->ctx.power;
}

void LDL_MAC_enableADR(struct ldl_mac *self)
{
    LDL_PEDANTIC(self != NULL)
    
    self->ctx.adr = true;
    
#ifndef LDL_DISABLE_SESSION_UPDATE    
    pushSessionUpdate(self);       
#endif    
}

bool LDL_MAC_adr(const struct ldl_mac *self)
{
    LDL_PEDANTIC(self != NULL)
    
    return self->ctx.adr;
}

void LDL_MAC_disableADR(struct ldl_mac *self)
{
    LDL_PEDANTIC(self != NULL)
    
    self->ctx.adr = false;
    
#ifndef LDL_DISABLE_SESSION_UPDATE    
    pushSessionUpdate(self);       
#endif     
}

bool LDL_MAC_ready(const struct ldl_mac *self)
{
    LDL_PEDANTIC(self != NULL)
    
    bool retval = false;
    
    if(self->state == LDL_STATE_IDLE){
        
        retval = (msUntilNextChannel(self, self->ctx.rate) == 0UL);
    }
    
    return retval;
}

uint32_t LDL_MAC_bwToNumber(enum ldl_signal_bandwidth bw)
{
    uint32_t retval;
    
    switch(bw){
    default:
    case LDL_BW_125:
        retval = 125000UL;
        break;        
    case LDL_BW_250:
        retval = 250000UL;
        break;            
    case LDL_BW_500:
        retval = 500000UL;
        break;                
    }
    
    return retval;
}

void LDL_MAC_radioEvent(struct ldl_mac *self, enum ldl_radio_event event)
{
    LDL_PEDANTIC(self != NULL)
    
    switch(event){
    case LDL_RADIO_EVENT_TX_COMPLETE:
        LDL_MAC_inputSignal(self, LDL_INPUT_TX_COMPLETE);
        break;
    case LDL_RADIO_EVENT_RX_READY:
        LDL_MAC_inputSignal(self, LDL_INPUT_RX_READY);
        break;
    case LDL_RADIO_EVENT_RX_TIMEOUT:
        LDL_MAC_inputSignal(self, LDL_INPUT_RX_TIMEOUT);        
        break;
    case LDL_RADIO_EVENT_NONE:
    default:
        break;
    }     
}

uint8_t LDL_MAC_mtu(const struct ldl_mac *self)
{
    LDL_PEDANTIC(self != NULL)
    
    enum ldl_signal_bandwidth bw;
    enum ldl_spreading_factor sf;
    uint8_t max = 0U;
    uint8_t overhead = LDL_Frame_dataOverhead();    
    
    /* which rate?? */
    LDL_Region_convertRate(self->region, self->ctx.rate, &sf, &bw, &max);
    
    LDL_PEDANTIC(LDL_Frame_dataOverhead() < max)
    
    if(self->rekeyConf_pending){
        
        overhead += LDL_MAC_sizeofCommandUp(LDL_CMD_REKEY);
    }
    
    if(self->dlChannelAns_pending){
        
        overhead += LDL_MAC_sizeofCommandUp(LDL_CMD_DL_CHANNEL);        
    }
    
    if(self->rxtimingSetupAns_pending){
        
        overhead += LDL_MAC_sizeofCommandUp(LDL_CMD_RX_TIMING_SETUP);                       
    }
    
    if(self->rxParamSetupAns_pending){
        
        overhead += LDL_MAC_sizeofCommandUp(LDL_CMD_RX_PARAM_SETUP);
    }
    
    if(self->linkCheckReq_pending){
        
        overhead += LDL_MAC_sizeofCommandUp(LDL_CMD_LINK_CHECK);
    }
    
    return (overhead > max) ? 0 : (max - overhead);    
}

uint32_t LDL_MAC_timeSinceValidDownlink(struct ldl_mac *self)
{
    LDL_PEDANTIC(self != NULL)
    
    return (self->last_valid_downlink == 0) ? UINT32_MAX : (timeNow(self) - self->last_valid_downlink);
}

void LDL_MAC_setMaxDCycle(struct ldl_mac *self, uint8_t maxDCycle)
{
    LDL_PEDANTIC(self != NULL)
    
    self->ctx.maxDutyCycle = maxDCycle & 0xfU;
    
#ifndef LDL_DISABLE_SESSION_UPDATE    
    pushSessionUpdate(self);        
#endif    
}

uint8_t LDL_MAC_getMaxDCycle(const struct ldl_mac *self)
{
    LDL_PEDANTIC(self != NULL)
    
    return self->ctx.maxDutyCycle;
}

void LDL_MAC_setNbTrans(struct ldl_mac *self, uint8_t nbTrans)
{
    LDL_PEDANTIC(self != NULL)
    
    self->ctx.nbTrans = nbTrans & 0xfU;
    
#ifndef LDL_DISABLE_SESSION_UPDATE
    pushSessionUpdate(self);     
#endif        
}

uint8_t LDL_MAC_getNbTrans(const struct ldl_mac *self)
{
    LDL_PEDANTIC(self != NULL)
    
    return self->ctx.nbTrans;        
}

bool LDL_MAC_addChannel(struct ldl_mac *self, uint8_t chIndex, uint32_t freq, uint8_t minRate, uint8_t maxRate)
{
    LDL_PEDANTIC(self != NULL)
    
    LDL_DEBUG(self->app, "adding chIndex=%u freq=%"PRIu32" minRate=%u maxRate=%u", chIndex, freq, minRate, maxRate)
    
    return setChannel(self, chIndex, freq, minRate, maxRate);
}

bool LDL_MAC_maskChannel(struct ldl_mac *self, uint8_t chIndex)
{
    LDL_PEDANTIC(self != NULL)
    
    return maskChannel(self->ctx.chMask, sizeof(self->ctx.chMask), self->region, chIndex);
}

bool LDL_MAC_unmaskChannel(struct ldl_mac *self, uint8_t chIndex)
{
    LDL_PEDANTIC(self != NULL)
    
    return unmaskChannel(self->ctx.chMask, sizeof(self->ctx.chMask), self->region, chIndex);
}

void LDL_MAC_timerSet(struct ldl_mac *self, enum ldl_timer_inst timer, uint32_t timeout)
{
    LDL_SYSTEM_ENTER_CRITICAL(self->app)
   
    self->timers[timer].time = LDL_System_ticks(self->app) + (timeout & INT32_MAX);
    self->timers[timer].armed = true;
    
    LDL_SYSTEM_LEAVE_CRITICAL(self->app)
}

void LDL_MAC_timerIncrement(struct ldl_mac *self, enum ldl_timer_inst timer, uint32_t timeout)
{
    LDL_SYSTEM_ENTER_CRITICAL(self->app)
   
    self->timers[timer].time += (timeout & INT32_MAX);
    self->timers[timer].armed = true;
    
    LDL_SYSTEM_LEAVE_CRITICAL(self->app)
}

bool LDL_MAC_timerCheck(struct ldl_mac *self, enum ldl_timer_inst timer, uint32_t *error)
{
    bool retval = false;
    uint32_t time;
    
    LDL_SYSTEM_ENTER_CRITICAL(self->app)
        
    if(self->timers[timer].armed){
        
        time = LDL_System_ticks(self->app);
        
        if(timerDelta(self->timers[timer].time, time) < INT32_MAX){
    
            self->timers[timer].armed = false;            
            *error = timerDelta(self->timers[timer].time, time);
            retval = true;
        }
    }    
    
    LDL_SYSTEM_LEAVE_CRITICAL(self->app)
    
    return retval;
}

void LDL_MAC_timerClear(struct ldl_mac *self, enum ldl_timer_inst timer)
{
    self->timers[timer].armed = false;
}

uint32_t LDL_MAC_timerTicksUntilNext(const struct ldl_mac *self)
{
    size_t i;
    uint32_t retval = UINT32_MAX;
    uint32_t time;
    
    time = LDL_System_ticks(self->app);

    for(i=0U; i < (sizeof(self->timers)/sizeof(*self->timers)); i++){

        LDL_SYSTEM_ENTER_CRITICAL(self->app)

        if(self->timers[i].armed){
            
            if(timerDelta(self->timers[i].time, time) <= INT32_MAX){
                
                retval = 0U;
            }
            else{
                
                if(timerDelta(time, self->timers[i].time) < retval){
                    
                    retval = timerDelta(time, self->timers[i].time);
                }
            }
        }
        
        LDL_SYSTEM_LEAVE_CRITICAL(self->app)
        
        if(retval == 0U){
            
            break;
        }
    }
    
    return retval;
}

uint32_t LDL_MAC_timerTicksUntil(const struct ldl_mac *self, enum ldl_timer_inst timer, uint32_t *error)
{
    uint32_t retval = UINT32_MAX;
    uint32_t time;
    
    LDL_SYSTEM_ENTER_CRITICAL(self->app)
    
    if(self->timers[timer].armed){
        
        time = LDL_System_ticks(self->app);
    
        *error = timerDelta(self->timers[timer].time, time);
        
        if(*error <= INT32_MAX){
            
            retval = 0U;
        }
        else{
            
            retval = timerDelta(time, self->timers[timer].time);
        }
    }
    
    LDL_SYSTEM_LEAVE_CRITICAL(self->app)
    
    return retval;
}

void LDL_MAC_inputSignal(struct ldl_mac *self, enum ldl_input_type type)
{
    LDL_SYSTEM_ENTER_CRITICAL(self->app)
    
    if(self->inputs.state == 0U){
    
        if((self->inputs.armed & (1U << type)) > 0U){
    
            self->inputs.time = LDL_System_ticks(self->app);
            self->inputs.state = (1U << type);
        }
    }
    
    LDL_SYSTEM_LEAVE_CRITICAL(self->app)
}

void LDL_MAC_inputArm(struct ldl_mac *self, enum ldl_input_type type)
{
    LDL_SYSTEM_ENTER_CRITICAL(self->app) 
    
    self->inputs.armed |= (1U << type);
    
    LDL_SYSTEM_LEAVE_CRITICAL(self->app)     
}

bool LDL_MAC_inputCheck(const struct ldl_mac *self, enum ldl_input_type type, uint32_t *error)
{
    bool retval = false;
    
    LDL_SYSTEM_ENTER_CRITICAL(self->app)     
    
    if((self->inputs.state & (1U << type)) > 0U){
        
        *error = timerDelta(self->inputs.time, LDL_System_ticks(self->app));
        retval = true;
    }
    
    LDL_SYSTEM_LEAVE_CRITICAL(self->app)    
    
    return retval;
}

void LDL_MAC_inputClear(struct ldl_mac *self)
{
    LDL_SYSTEM_ENTER_CRITICAL(self->app)     
    
    self->inputs.state = 0U;
    self->inputs.armed = 0U;
    
    LDL_SYSTEM_LEAVE_CRITICAL(self->app)   
}

bool LDL_MAC_inputPending(const struct ldl_mac *self)
{
    return (self->inputs.state != 0U);
}

bool LDL_MAC_priority(const struct ldl_mac *self, uint8_t interval)
{
    LDL_PEDANTIC(self != NULL)
    
    bool retval;
    uint32_t error;
    
    /* todo */
    (void)interval; 
    
    LDL_MAC_timerTicksUntil(self, LDL_TIMER_WAITA, &error);
    
    switch(self->state){
    default:
        retval = false;
        break;
    case LDL_STATE_TX:    
    case LDL_STATE_WAIT_RX1:
    case LDL_STATE_RX1:
    case LDL_STATE_WAIT_RX2:
    case LDL_STATE_RX2:
        retval = true;
        break;
    }
    
    return retval;
}

/* static functions ***************************************************/

static bool externalDataCommand(struct ldl_mac *self, bool confirmed, uint8_t port, const void *data, uint8_t len, const struct ldl_mac_data_opts *opts)
{
    bool retval = false;
    uint8_t maxPayload;
    enum ldl_signal_bandwidth bw;
    enum ldl_spreading_factor sf;
    
    self->errno = LDL_ERRNO_NONE;

    if(self->state == LDL_STATE_IDLE){
        
        if(self->ctx.joined){
        
            if((port > 0U) && (port <= 223U)){    
                
                if(self->band[LDL_BAND_GLOBAL] == 0UL){
                
                    if(selectChannel(self, self->ctx.rate, self->tx.chIndex, 0UL, &self->tx.chIndex, &self->tx.freq)){
                     
                        LDL_Region_convertRate(self->region, self->ctx.rate, &sf, &bw, &maxPayload);
                                            
                        if(opts == NULL){
                            
                            (void)memset(&self->opts, 0, sizeof(self->opts));
                        }
                        else{
                            
                            (void)memcpy(&self->opts, opts, sizeof(self->opts));
                        }

                        self->opts.nbTrans = self->opts.nbTrans & 0xfU;
                        
                        
                        retval = dataCommand(self, confirmed, port, data, len);
                        
                    }
                    else{
                        
                        self->errno = LDL_ERRNO_NOCHANNEL;
                    }                
                }
                else{
                    
                    self->errno = LDL_ERRNO_NOCHANNEL;
                }
            }
            else{
                
                self->errno = LDL_ERRNO_PORT;
            }
        }
        else{
            
            self->errno = LDL_ERRNO_NOTJOINED;
        }
    }
    else{
        
        self->errno = LDL_ERRNO_BUSY;
    }
            
    return retval;
}

static bool dataCommand(struct ldl_mac *self, bool confirmed, uint8_t port, const void *data, uint8_t len)
{
    LDL_PEDANTIC(self != NULL)
    LDL_PEDANTIC((len == 0) || (data != NULL))
    
    bool retval = false;
    struct ldl_frame_data f;
    struct ldl_stream s;
    uint8_t maxPayload;
    uint8_t opts[15U];
    enum ldl_signal_bandwidth bw;
    enum ldl_spreading_factor sf;
    
    self->trials = 0U;
    
    self->tx.rate = self->ctx.rate;
    self->tx.power = self->ctx.power;
    
    /* pending MAC commands take priority over user payload */
    LDL_Stream_init(&s, opts, sizeof(opts));
    
    if(self->rekeyConf_pending){
        
        struct ldl_rekey_ind ind;
        ind.version = self->ctx.version;
        LDL_MAC_putRekeyInd(&s, &ind);
    }
    
    if(self->dlChannelAns_pending){
    
        struct ldl_dl_channel_ans ans;        
        (void)memset(&ans, 0, sizeof(ans));
        LDL_MAC_putDLChannelAns(&s, &ans);                                    
    }
    
    if(self->rxtimingSetupAns_pending){
        
        (void)LDL_MAC_putRXTimingSetupAns(&s);                                    
    }
    
    if(self->rxParamSetupAns_pending){
        
        struct ldl_rx_param_setup_ans ans;
        (void)memset(&ans, 0, sizeof(ans));
        LDL_MAC_putRXParamSetupAns(&s, &ans);                                    
    }
    
#ifndef LDL_DISABLE_CHECK    
    if(self->opts.check){

        LDL_MAC_putLinkCheckReq(&s);
    }                   
#endif    
    
#ifndef LDL_DISABLE_DEVICE_TIME    
    if(self->opts.getTime){
        
        LDL_MAC_putDeviceTimeReq(&s);
    }             
#endif    
    
    LDL_Region_convertRate(self->region, self->tx.rate, &sf, &bw, &maxPayload);
    
    LDL_PEDANTIC(maxPayload >= LDL_Frame_dataOverhead())
    
    (void)memset(&f, 0, sizeof(f));
    
    f.type = confirmed ? FRAME_TYPE_DATA_CONFIRMED_UP : FRAME_TYPE_DATA_UNCONFIRMED_UP;
    f.devAddr = self->ctx.devAddr;
    f.counter = self->ctx.up;
    f.adr = self->ctx.adr;
    f.adrAckReq = self->adrAckReq;
    f.opts = opts;
    f.optsLen = LDL_Stream_tell(&s);
    f.port = port;
    
    self->state = LDL_STATE_WAIT_TX;    
    
    /* it's possible the user data doesn't fit after mac command priority */
    if((LDL_Stream_tell(&s) + LDL_Frame_dataOverhead() + len) <= maxPayload){
        
        f.data = data;
        f.dataLen = len;
        
        self->bufferLen = LDL_OPS_prepareData(self, &f, self->buffer, sizeof(self->buffer));
        
        self->op = confirmed ? LDL_OP_DATA_CONFIRMED : LDL_OP_DATA_UNCONFIRMED;
                
        retval = true;
    }
    /* no room for data, prioritise data */
    else{
        
        f.type = FRAME_TYPE_DATA_UNCONFIRMED_UP;
        f.port = 0U;
        f.data = opts;
        f.dataLen = f.optsLen;
        f.opts = NULL;
        f.optsLen = 0U;
        
        self->bufferLen = LDL_OPS_prepareData(self, &f, self->buffer, sizeof(self->buffer));
        
        self->op = LDL_OP_DATA_UNCONFIRMED;
        self->errno = LDL_ERRNO_SIZE;          //fixme: special error code to say goalpost moved?                
    }
    
    self->ctx.up++;
    
    /* putData must have failed for some reason */
    LDL_PEDANTIC(self->bufferLen > 0U)
    
    uint32_t send_delay = 0U;
    
    if(self->opts.dither > 0U){
        
        send_delay = (rand32(self->app) % ((uint32_t)self->opts.dither * LDL_System_tps()));
    }
    
    self->service_start_time = timeNow(self) + (send_delay / LDL_System_tps());            
                
    LDL_MAC_timerSet(self, LDL_TIMER_WAITA, send_delay);
    
    return retval;
}

static void adaptRate(struct ldl_mac *self)
{
    self->adrAckReq = false;
    
    if(self->ctx.adr){
                
        if(self->adrAckCounter < UINT8_MAX){
            
            if(self->adrAckCounter >= self->ctx.adr_ack_limit){
                
                self->adrAckReq = true;
            
                LDL_DEBUG(self->app, "adr: adrAckCounter=%u (past ADRAckLimit)", self->adrAckCounter)
            
                if(self->adrAckCounter >= (self->ctx.adr_ack_limit + self->ctx.adr_ack_delay)){
                
                    if(((self->adrAckCounter - (self->ctx.adr_ack_limit + self->ctx.adr_ack_delay)) % self->ctx.adr_ack_delay) == 0U){
                
                        if(self->ctx.power == 0U){
                
                            if(self->ctx.rate > LDL_DEFAULT_RATE){
                                                            
                                self->ctx.rate--;
                                LDL_DEBUG(self->app, "adr: rate reduced to %u", self->ctx.rate)
                            }
                            else{
                                
                                LDL_DEBUG(self->app, "adr: all channels unmasked")
                                
                                unmaskAllChannels(self->ctx.chMask, sizeof(self->ctx.chMask));
                                
                                self->adrAckCounter = UINT8_MAX;
                            }
                        }
                        else{
                            
                            LDL_DEBUG(self->app, "adr: full power enabled")
                            self->ctx.power = 0U;
                        }
                    }
                }    
            }
                
            self->adrAckCounter++;            
        }        
    }
}

static uint32_t transmitTime(enum ldl_signal_bandwidth bw, enum ldl_spreading_factor sf, uint8_t size, bool crc)
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
    
    /* optimise this mode according to the datasheet */
    lowDataRateOptimize = ((bw == LDL_BW_125) && ((sf == LDL_SF_11) || (sf == LDL_SF_12))) ? true : false;    
    
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

static uint32_t symbolPeriod(enum ldl_spreading_factor sf, enum ldl_signal_bandwidth bw)
{
    return ((((uint32_t)1U) << sf) * LDL_System_tps()) / LDL_MAC_bwToNumber(bw);
}

static uint8_t extraSymbols(uint32_t xtal_error, uint32_t symbol_period)
{
    return (xtal_error / symbol_period) + (((xtal_error % symbol_period) > 0U) ? 1U : 0U);        
}

static void registerTime(struct ldl_mac *self, uint32_t freq, uint32_t airTime)
{
    uint8_t band;
    uint32_t offtime;
    
    if(LDL_Region_getBand(self->region, freq, &band)){
    
        offtime = LDL_Region_getOffTimeFactor(self->region, band);
    
        if(offtime > 0U){
        
            LDL_PEDANTIC( band < LDL_BAND_MAX )
            
            offtime = ticksToMS(airTime) * offtime;
            
            if((self->band[band] + offtime) < self->band[band]){
                
                self->band[band] = UINT32_MAX;
            }
            else{
                
                self->band[band] += offtime; 
            }
        }
    }
    
    if((self->op != LDL_OP_JOINING) && (self->ctx.maxDutyCycle > 0U)){
        
        offtime = ticksToMS(airTime) * ( 1UL << (self->ctx.maxDutyCycle & 0xfU));
        
        if((self->band[LDL_BAND_GLOBAL] + offtime) < self->band[LDL_BAND_GLOBAL]){
            
            self->band[LDL_BAND_GLOBAL] = UINT32_MAX;
        }
        else{
            
            self->band[LDL_BAND_GLOBAL] += offtime; 
        }
    }
}    

static bool selectChannel(const struct ldl_mac *self, uint8_t rate, uint8_t prevChIndex, uint32_t limit, uint8_t *chIndex, uint32_t *freq)
{
    bool retval = false;
    uint8_t i;    
    uint8_t selection;    
    uint8_t available = 0U;
    uint8_t j = 0U;
    uint8_t minRate;
    uint8_t maxRate;    
    uint8_t except = UINT8_MAX;
    
    uint8_t mask[sizeof(self->ctx.chMask)];
    
    (void)memset(mask, 0, sizeof(mask));
    
    /* count number of available channels for this rate */
    for(i=0U; i < LDL_Region_numChannels(self->region); i++){
        
        if(isAvailable(self, i, rate, limit)){
        
            if(i == prevChIndex){
                
                except = i;
            }
        
            (void)maskChannel(mask, sizeof(mask), self->region, i);        
            available++;            
        }            
    }
        
    if(available > 0U){
    
        if(except != UINT8_MAX){
    
            if(available == 1U){
                
                except = UINT8_MAX;
            }
            else{
                
                available--;
            }
        }
    
        selection = LDL_System_rand(self->app) % available;
        
        for(i=0U; i < LDL_Region_numChannels(self->region); i++){
        
            if(channelIsMasked(mask, sizeof(mask), self->region, i)){
        
                if(except != i){
            
                    if(selection == j){
                        
                        if(getChannel(self, i, freq, &minRate, &maxRate)){
                            
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
    
    return retval;
}

static bool isAvailable(const struct ldl_mac *self, uint8_t chIndex, uint8_t rate, uint32_t limit)
{
    bool retval = false;
    uint32_t freq;
    uint8_t minRate;    
    uint8_t maxRate;    
    uint8_t band;
    
    if(!channelIsMasked(self->ctx.chMask, sizeof(self->ctx.chMask), self->region, chIndex)){
    
        if(getChannel(self, chIndex, &freq, &minRate, &maxRate)){
            
            if((rate >= minRate) && (rate <= maxRate)){
            
                if(LDL_Region_getBand(self->region, freq, &band)){
                
                    LDL_PEDANTIC( band < LDL_BAND_MAX )
                
                    if(self->band[band] <= limit){
                
                        retval = true;              
                    }
                }
            }
        }
    }
    
    return retval;
}

static bool msUntilAvailable(const struct ldl_mac *self, uint8_t chIndex, uint8_t rate, uint32_t *ms)
{
    bool retval = false;
    uint32_t freq;
    uint8_t minRate;    
    uint8_t maxRate;    
    uint8_t band;
    
    if(!channelIsMasked(self->ctx.chMask, sizeof(self->ctx.chMask), self->region, chIndex)){
    
        if(getChannel(self, chIndex, &freq, &minRate, &maxRate)){
            
            if((rate >= minRate) && (rate <= maxRate)){
            
                if(LDL_Region_getBand(self->region, freq, &band)){
                
                    LDL_PEDANTIC( band < LDL_BAND_MAX )
                
                    *ms = (self->band[band] > self->band[LDL_BAND_GLOBAL]) ? self->band[band] : self->band[LDL_BAND_GLOBAL];
                    
                    retval = true;                
                }
            }
        }
    }
    
    return retval;
}

static void restoreDefaults(struct ldl_mac *self, bool keep)
{
    if(!keep){
        
        (void)memset(&self->ctx, 0, sizeof(self->ctx));
        self->ctx.rate = LDL_DEFAULT_RATE;    
        self->ctx.adr = true;        
    }
    else{
        
        self->ctx.up = 0U;
        self->ctx.nwkDown = 0U;
        self->ctx.appDown = 0U;
        
        (void)memset(self->ctx.chConfig, 0, sizeof(self->ctx.chConfig));
        (void)memset(self->ctx.chMask, 0, sizeof(self->ctx.chMask));        
        self->ctx.joined = false;        
    }
    
    LDL_Region_getDefaultChannels(self->region, self);    
    
    self->ctx.rx1DROffset = LDL_Region_getRX1Offset(self->region);
    self->ctx.rx1Delay = LDL_Region_getRX1Delay(self->region);
    self->ctx.rx2DataRate = LDL_Region_getRX2Rate(self->region);
    self->ctx.rx2Freq = LDL_Region_getRX2Freq(self->region);    
    self->ctx.version = 0U;

    self->ctx.adr_ack_limit = ADRAckLimit;
    self->ctx.adr_ack_delay = ADRAckDelay;
}

static bool getChannel(const struct ldl_mac *self, uint8_t chIndex, uint32_t *freq, uint8_t *minRate, uint8_t *maxRate)
{
    bool retval;
    const struct ldl_mac_channel *chConfig;
    
    retval = false;
    
    if(LDL_Region_isDynamic(self->region)){
        
        if(chIndex < LDL_Region_numChannels(self->region)){
            
            if(chIndex < sizeof(self->ctx.chConfig)/sizeof(*self->ctx.chConfig)){
            
                chConfig = &self->ctx.chConfig[chIndex];
                
                *freq = (chConfig->freqAndRate >> 8) * 100U;
                *minRate = (chConfig->freqAndRate >> 4) & 0xfU;
                *maxRate = chConfig->freqAndRate & 0xfU;
                
                retval = true;
            }
        }        
    }
    else{
        
        retval = LDL_Region_getChannel(self->region, chIndex, freq, minRate, maxRate);                        
    }
     
    return retval;
}

static bool setChannel(struct ldl_mac *self, uint8_t chIndex, uint32_t freq, uint8_t minRate, uint8_t maxRate)
{
    bool retval;
    
    retval = false;
    
    if(chIndex < LDL_Region_numChannels(self->region)){
        
        if(chIndex < sizeof(self->ctx.chConfig)/sizeof(*self->ctx.chConfig)){
        
            self->ctx.chConfig[chIndex].freqAndRate = ((freq/100U) << 8) | ((minRate << 4) & 0xfU) | (maxRate & 0xfU);
            retval = true;
        }        
    }
    
    return retval;
}

static bool maskChannel(uint8_t *mask, uint8_t max, enum ldl_region region, uint8_t chIndex)
{
    bool retval = false;
    
    if(chIndex < LDL_Region_numChannels(region)){
        
        if(chIndex < (max*8U)){
    
            mask[chIndex / 8U] |= (1U << (chIndex % 8U));
            retval = true;
        }
    }
    
    return retval;
}

static bool unmaskChannel(uint8_t *mask, uint8_t max, enum ldl_region region, uint8_t chIndex)
{
    bool retval = false;
    
    if(chIndex < LDL_Region_numChannels(region)){
    
        if(chIndex < (max*8U)){
    
            mask[chIndex / 8U] &= ~(1U << (chIndex % 8U));
            retval = true;
        }
    }
    
    return retval;
}

static void unmaskAllChannels(uint8_t *mask, uint8_t max)
{
    (void)memset(mask, 0, max);    
}

static bool channelIsMasked(const uint8_t *mask, uint8_t max, enum ldl_region region, uint8_t chIndex)
{
    bool retval = false;
    
    if(chIndex < LDL_Region_numChannels(region)){
        
        if(chIndex < (max*8U)){
        
            retval = ((mask[chIndex / 8U] & (1U << (chIndex % 8U))) > 0U);        
        }
    }
    
    return retval;    
}

static bool rateSettingIsValid(enum ldl_region region, uint8_t rate)
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

static uint32_t timeNow(struct ldl_mac *self)
{
    uint32_t seconds;
    uint32_t ticks;
    uint32_t since;
    uint32_t part;
    
    ticks = LDL_System_ticks(self->app);
    since = timerDelta(self->polled_time_ticks, ticks);
    
    seconds = since / LDL_System_tps();
    
    if(seconds > 0U){
    
        part = since % LDL_System_tps();        
        self->polled_time_ticks = (ticks - part);    
        self->time += seconds;
    }
    
    return self->time;        
}

static uint32_t getRetryDuty(uint32_t seconds_since)
{
    /* reset after one day */
    uint32_t delta = seconds_since % (60UL*60UL*24UL);
    
    /* 36/3600 (0.01) */
    if(delta < (60UL*60UL)){
        
        return 100UL;
    }
    /* 36/36000 (0.001) */
    else if(delta < (11UL*60UL*60UL)){
     
        return 1000UL;
    }
    /* 8.7/86400 (0.0001) */
    else{
        
        return 10000UL;    
    }
}

static uint32_t timerDelta(uint32_t timeout, uint32_t time)
{
    return (timeout <= time) ? (time - timeout) : (UINT32_MAX - timeout + time);
}

static void processBands(struct ldl_mac *self)
{
    uint32_t since;
    uint32_t ticks;
    uint32_t diff;
    size_t i;
    
    ticks = LDL_System_ticks(self->app);
    diff = timerDelta(self->polled_band_ticks, ticks);    
    since = diff / LDL_System_tps() * 1000UL;
    
    if(since > 0U){
    
        self->polled_band_ticks += LDL_System_tps() / 1000U * since;
        
        for(i=0U; i < (sizeof(self->band)/sizeof(*self->band)); i++){

            if(self->band[i] > 0U){
            
                if(self->band[i] < since){
                   
                  self->band[i] = 0U;  
                }
                else{
                   
                   self->band[i] -= since;
                }                                    
            }            
        }        
    }
}

static uint32_t nextBandEvent(const struct ldl_mac *self)
{
    uint32_t retval = UINT32_MAX;
    size_t i;
    
    for(i=0U; i < (sizeof(self->band)/sizeof(*self->band)); i++){
        
        if(self->band[i] > 0U){
        
            if(self->band[i] < retval){
                
                retval = self->band[i];
            }        
        }
    }
    
    return retval;
}

static void downlinkMissingHandler(struct ldl_mac *self)
{
    union ldl_mac_response_arg arg;
    uint8_t nbTrans;
    enum ldl_spreading_factor sf;
    enum ldl_signal_bandwidth bw;
    uint32_t delta;
    uint32_t tx_time;
    uint8_t mtu;
    
    (void)memset(&arg, 0, sizeof(arg));
    
    if(self->opts.nbTrans > 0U){
        
        nbTrans = self->opts.nbTrans;
    }
    else{
        
        nbTrans = (self->ctx.nbTrans > 0) ? self->ctx.nbTrans : 1U;
    }
    
    self->trials++;
    
    delta = (timeNow(self) - self->service_start_time);
    
    LDL_Region_convertRate(self->region, self->tx.rate, &sf, &bw, &mtu);
    
    tx_time = ticksToMS(transmitTime(bw, sf, self->bufferLen, true));
        
    switch(self->op){
    default:
    case LDL_OP_NONE:
        break;
    case LDL_OP_DATA_CONFIRMED:

        if(self->trials < nbTrans){

            self->band[LDL_BAND_RETRY] = tx_time * getRetryDuty(delta);
            self->state = LDL_STATE_WAIT_RETRY;            
        }
        else{
            
            adaptRate(self);
         
            self->tx.rate = self->ctx.rate;
            self->tx.power = self->ctx.power;
                
#ifndef LDL_DISABLE_DATA_CONFIRMED_EVENT                
            self->handler(self->app, LDL_MAC_DATA_TIMEOUT, NULL);
#endif                  
            self->state = LDL_STATE_IDLE;
            self->op = LDL_OP_NONE;
        }
        break;
        
    case LDL_OP_DATA_UNCONFIRMED:
    
        if(self->trials < nbTrans){
            
            if((self->band[LDL_BAND_GLOBAL] < LDL_Region_getMaxDCycleOffLimit(self->region)) && selectChannel(self, self->tx.rate, self->tx.chIndex, LDL_Region_getMaxDCycleOffLimit(self->region), &self->tx.chIndex, &self->tx.freq)){
            
                LDL_MAC_timerSet(self, LDL_TIMER_WAITA, 0U);
                self->state = LDL_STATE_WAIT_TX;
            }
            else{
                
                LDL_DEBUG(self->app, "no channel available for retry")
                
#ifndef LDL_DISABLE_DATA_COMPLETE_EVENT                    
                self->handler(self->app, LDL_MAC_DATA_COMPLETE, NULL);
#endif                    
                self->state = LDL_STATE_IDLE;
                self->op = LDL_OP_NONE;                      
            }            
        }
        else{
        
            adaptRate(self);
         
            self->tx.rate = self->ctx.rate;
            self->tx.power = self->ctx.power;
                                
#ifndef LDL_DISABLE_DATA_COMPLETE_EVENT                    
            self->handler(self->app, LDL_MAC_DATA_COMPLETE, NULL);
#endif  
            self->state = LDL_STATE_IDLE;
            self->op = LDL_OP_NONE;                      
        }
        break;

    case LDL_OP_JOINING:
        
        self->band[LDL_BAND_RETRY] = tx_time * getRetryDuty(delta);
        
        self->tx.rate = LDL_Region_getJoinRate(self->region, self->trials);
        
#ifndef LDL_DISABLE_JOIN_TIMEOUT_EVENT                               
        self->handler(self->app, LDL_MAC_JOIN_TIMEOUT, &arg);                    
#endif                            
        self->state = LDL_STATE_WAIT_RETRY;        
        break;                    
    }        
}

static uint32_t ticksToMS(uint32_t ticks)
{
    return ticks * 1000UL / LDL_System_tps();
}

static uint32_t ticksToMSCoarse(uint32_t ticks)
{
    return ticks / LDL_System_tps() * 1000UL;
}

static uint32_t msUntilNextChannel(const struct ldl_mac *self, uint8_t rate)
{
    uint8_t i;
    uint32_t min = UINT32_MAX;
    uint32_t ms;
    
    for(i=0U; i < LDL_Region_numChannels(self->region); i++){
        
        if(msUntilAvailable(self, i, rate, &ms)){
            
            if(ms < min){
                
                min = ms;
            }
        }
    }
    
    return min;
}

static uint32_t rand32(void *app)
{
    uint32_t retval;
    
    retval = LDL_System_rand(app);
    retval <<= 8;
    retval |= LDL_System_rand(app);
    retval <<= 8;
    retval |= LDL_System_rand(app);
    retval <<= 8;
    retval |= LDL_System_rand(app);
    
    return retval;
}

#ifndef LDL_DISABLE_SESSION_UPDATE
static void pushSessionUpdate(struct ldl_mac *self)
{
    union ldl_mac_response_arg arg;     
    arg.session_updated.session = &self->ctx;
    self->handler(self->app, LDL_MAC_SESSION_UPDATED, &arg);        
}
#endif

static void dummyResponseHandler(void *app, enum ldl_mac_response_type type, const union ldl_mac_response_arg *arg)
{
    (void)app;
    (void)type;
    (void)arg;
}
