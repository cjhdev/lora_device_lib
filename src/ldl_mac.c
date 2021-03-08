/* Copyright (c) 2019-2021 Cameron Harper
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
#include "ldl_internal.h"
#include <string.h>

enum {

    ADRAckLimit = 64U,
    ADRAckDelay = 32U,
    ADRAckTimeout = 2U
};

#ifdef LDL_PARAM_TPS
    #define GET_TPS() U32(LDL_PARAM_TPS)
#else
    #define GET_TPS() self->tps
#endif

#ifdef LDL_PARAM_A
    #define GET_A() U32(LDL_PARAM_A)
#else
    #define GET_A() self->a
#endif

#ifdef LDL_PARAM_B
    #define GET_B() U32(LDL_PARAM_B)
#else
    #define GET_B() self->b
#endif

#ifdef LDL_PARAM_ADVANCE
    #define GET_ADVANCE() U32(LDL_PARAM_ADVANCE)
#else
    #define GET_ADVANCE() self->advance
#endif

#ifdef LDL_DISABLE_SF12
    #define MIN_RATE 1
#else
    #define MIN_RATE 0
#endif

/* static function prototypes *****************************************/


static void processInit(struct ldl_mac *self);
static void processRadioReset(struct ldl_mac *self, enum ldl_mac_sme event);

static void processRadioBoot(struct ldl_mac *self, enum ldl_mac_sme event);

static void processWait(struct ldl_mac *self, enum ldl_mac_sme event, uint32_t lag);

static void processWaitOTAA(struct ldl_mac *self, enum ldl_mac_sme event);

static void processStartRadioForEntropy(struct ldl_mac *self, enum ldl_mac_sme event);
static void processEntropy(struct ldl_mac *self, enum ldl_mac_sme event);

static void processStartRadioForTX(struct ldl_mac *self, enum ldl_mac_sme event);
static void processTX(struct ldl_mac *self, enum ldl_mac_sme event, uint32_t lag);

static void processStartRadioForRX1(struct ldl_mac *self, enum ldl_mac_sme event, uint32_t lag);
static void processStartRadioForRX2(struct ldl_mac *self, enum ldl_mac_sme event, uint32_t lag);
static void processRX(struct ldl_mac *self, enum ldl_mac_sme event);

static void processRX2Lockout(struct ldl_mac *self, enum ldl_mac_sme event);

static void debugSession(struct ldl_mac *self);
static uint32_t extraSymbols(uint32_t xtal_error, uint32_t symbol_period);
static enum ldl_mac_status externalDataCommand(struct ldl_mac *self, bool confirmed, uint8_t port, const void *data, uint8_t len, const struct ldl_mac_data_opts *opts);
static void processCommands(struct ldl_mac *self, const uint8_t *in, uint8_t len);
static bool selectChannel(const struct ldl_mac *self, uint8_t desired_rate, uint32_t limit, struct ldl_mac_tx *tx);
static uint8_t requiredRate(uint8_t desired, uint8_t min, uint8_t max);
static void selectJoinChannelAndRate(struct ldl_mac *self, struct ldl_mac_tx *tx);
static void registerTime(struct ldl_mac *self, const struct ldl_mac_tx *tx);
static bool getChannel(const struct ldl_mac *self, uint8_t chIndex, uint32_t *freq, uint8_t *minRate, uint8_t *maxRate);
static bool isAvailable(const struct ldl_mac *self, uint8_t chIndex, uint32_t limit);
static void initSession(struct ldl_mac *self, enum ldl_region region);
static void forgetNetwork(struct ldl_mac *self);
static bool setChannel(struct ldl_mac *self, uint8_t chIndex, uint32_t freq, uint8_t minRate, uint8_t maxRate);
static bool maskChannel(uint8_t *mask, size_t max, enum ldl_region region, uint8_t chIndex);
static bool unmaskChannel(uint8_t *mask, size_t max, enum ldl_region region, uint8_t chIndex);
static void unmaskAllChannels(uint8_t *mask, size_t max);
static bool channelIsMasked(const uint8_t *mask, size_t max, enum ldl_region region, uint8_t chIndex);
static uint32_t symbolPeriod(uint32_t tps, enum ldl_spreading_factor sf, enum ldl_signal_bandwidth bw);
static uint32_t timeUntilAvailable(const struct ldl_mac *self, uint8_t chIndex);
static bool rateSettingIsValid(enum ldl_region region, uint8_t rate);
static bool adaptRate(struct ldl_mac *self);
static bool processBands(struct ldl_mac *self);
static void setNextBandEvent(struct ldl_mac *self);
static void downlinkMissingHandler(struct ldl_mac *self);
static uint32_t timeUntilNextChannel(const struct ldl_mac *self);
static uint32_t timerDelta(uint32_t timeout, uint32_t time);
static void pushSessionUpdate(struct ldl_mac *self);
static void dummyResponseHandler(void *app, enum ldl_mac_response_type type, const union ldl_mac_response_arg *arg);
static bool allChannelsAreMasked(const uint8_t *mask, size_t max);
static bool commandIsPending(const struct ldl_mac *self, enum ldl_mac_cmd_type type);
static void clearPendingCommand(struct ldl_mac *self, enum ldl_mac_cmd_type type);
static void setPendingCommand(struct ldl_mac *self, enum ldl_mac_cmd_type type);
static uint32_t defaultRand(void *app);
static uint8_t defaultBatteryLevel(void *app);
static uint32_t getOTAAOffTime(const struct ldl_mac *self);
static void handleRadioError(struct ldl_mac *self);
#ifndef LDL_DISABLE_TX_PARAM_SETUP
static bool uplinkDwell(uint8_t tx_param_setup);
#endif
static void fillJoinBuffer(struct ldl_mac *self, uint16_t devNonce);

static uint32_t msToTime(uint32_t ms);
static uint32_t msToTicks(const struct ldl_mac *self, uint32_t ms);

static void inputArm(struct ldl_mac *self);
static void inputDisarm(struct ldl_mac *self);
static bool inputCheck(struct ldl_mac *self, uint32_t *lag);
static void inputSignal(struct ldl_mac *self, uint32_t ticks);
static bool inputPending(const struct ldl_mac *self);

static const uint32_t timeTPS = U32(0x100);
static const uint8_t sessionMagicNumber = 0xdbU;

/* functions **********************************************************/

void LDL_MAC_init(struct ldl_mac *self, enum ldl_region region, const struct ldl_mac_init_arg *arg)
{
    LDL_PEDANTIC(self != NULL)
    LDL_PEDANTIC(arg != NULL)
    LDL_PEDANTIC(arg->ticks != NULL)
    LDL_PEDANTIC(arg->radio_interface != NULL);
    LDL_PEDANTIC(arg->sm_interface != NULL);

    (void)memset(self, 0, sizeof(*self));

    self->tx.chIndex = UINT8_MAX;

#ifndef LDL_PARAM_TPS
    self->tps = arg->tps;
#endif
#ifndef LDL_PARAM_A
    self->a = arg->a;
#endif
#ifndef LDL_PARAM_B
    self->b = arg->b;
#endif
#ifndef LDL_PARAM_ADVANCE
    self->advance = arg->advance;
#endif
#ifdef LDL_ENABLE_OTAA_DITHER
    self->otaaDither = arg->otaaDither;
#endif

    /* 1M >= tps >= 1K */
    LDL_PEDANTIC((GET_TPS() >= U32(1000)) && (GET_TPS() <= U32(1000000)))

    self->ticks = arg->ticks;
    self->rand = (arg->rand != NULL) ? arg->rand : defaultRand;
    self->get_battery_level = (arg->get_battery_level != NULL) ? arg->get_battery_level : defaultBatteryLevel;

    self->app = arg->app;
    self->handler = (arg->handler != NULL) ? arg->handler : dummyResponseHandler;

    self->radio = arg->radio;
    self->radio_interface = arg->radio_interface;

    self->sm = arg->sm;
    self->sm_interface = arg->sm_interface;

    self->devNonce = arg->devNonce;
    self->joinNonce = arg->joinNonce;

    if(arg->devEUI != NULL){

        (void)memcpy(self->devEUI, arg->devEUI, sizeof(self->devEUI));
    }

    if(arg->joinEUI != NULL){

        (void)memcpy(self->joinEUI, arg->joinEUI, sizeof(self->joinEUI));
    }

    if((arg->session != NULL) && (arg->session->magic == sessionMagicNumber) && (arg->session->region == region)){

        (void)memcpy(&self->ctx, arg->session, sizeof(self->ctx));

        /* re-derive keys from:
         *
         * - root keys
         * - self->joinEUI
         * - self->session
         *
         *  */
        LDL_OPS_deriveKeys(self);
    }
    else{

        initSession(self, region);
    }

    self->band[LDL_BAND_GLOBAL] = msToTime(U32(LDL_STARTUP_DELAY));

    self->time.ticks = self->ticks(self->app);

    LDL_MAC_timerSet(self, LDL_TIMER_WAITA, 0);

    debugSession(self);
}

enum ldl_mac_status LDL_MAC_entropy(struct ldl_mac *self)
{
    LDL_PEDANTIC(self != NULL)

    enum ldl_mac_status retval;

    if(self->op == LDL_OP_NONE){

        self->op = LDL_OP_ENTROPY;

        if(self->state == LDL_STATE_IDLE){

            self->state = LDL_STATE_RADIO_BOOT;
            LDL_MAC_timerSet(self, LDL_TIMER_WAITA, 0);
        }

        retval = LDL_STATUS_OK;
    }
    else{

        retval = LDL_STATUS_BUSY;
    }

    return retval;
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

enum ldl_mac_status LDL_MAC_unconfirmedData(struct ldl_mac *self, uint8_t port, const void *data, uint8_t len, const struct ldl_mac_data_opts *opts)
{
    return externalDataCommand(self, false, port, data, len, opts);
}

enum ldl_mac_status LDL_MAC_confirmedData(struct ldl_mac *self, uint8_t port, const void *data, uint8_t len, const struct ldl_mac_data_opts *opts)
{
    return externalDataCommand(self, true, port, data, len, opts);
}

enum ldl_mac_status LDL_MAC_otaa(struct ldl_mac *self)
{
    enum ldl_mac_status retval;
    union ldl_mac_response_arg arg;

    LDL_PEDANTIC(self != NULL)

    if(self->ctx.joined){

        retval = LDL_STATUS_JOINED;
    }
    else if(self->op == LDL_OP_NONE){

        if(self->devNonce <= U32(UINT16_MAX)){

            forgetNetwork(self);

            self->trials = 0;

            self->day = U32(60) * U32(60) * U32(24) * timeTPS;

#if defined(LDL_ENABLE_L2_1_1)
            LDL_OPS_deriveJoinKeys(self);
#endif
            fillJoinBuffer(self, U16(self->devNonce));

            self->devNonce++;

            arg.dev_nonce_updated.nextDevNonce = self->devNonce;

            self->handler(self->app, LDL_MAC_DEV_NONCE_UPDATED, &arg);

            self->tx.power = 0;

            self->op = LDL_OP_JOINING;

            if(self->state == LDL_STATE_IDLE){

                self->state = LDL_STATE_WAIT_OTAA;
                LDL_MAC_timerSet(self, LDL_TIMER_WAITA, 0);
            }

            retval = LDL_STATUS_OK;

            LDL_DEBUG("OTAA is pending")
        }
        else{

            /* need to re-init with a different JoinEUI */
            retval = LDL_STATUS_DEVNONCE;
        }
    }
    else{

        retval = LDL_STATUS_BUSY;
    }

    return retval;
}

#ifdef LDL_ENABLE_ABP
enum ldl_mac_status LDL_MAC_abp(struct ldl_mac *self, uint32_t devAddr)
{
    enum ldl_mac_status retval;

    LDL_PEDANTIC(self != NULL)

    if(self->ctx.joined){

        retval = LDL_STATUS_JOINED;
    }
    else if(self->op == LDL_OP_NONE){

        forgetNetwork(self);

        self->ctx.joined = true;
        self->ctx.devAddr = devAddr;

        self->band[LDL_BAND_GLOBAL] = 0;
        self->day = 0;

        retval = LDL_STATUS_OK;
    }
    else{

        retval = LDL_STATUS_BUSY;
    }

    return retval;
}
#endif

bool LDL_MAC_joined(const struct ldl_mac *self)
{
    return self->ctx.joined;
}

void LDL_MAC_forget(struct ldl_mac *self)
{
    LDL_PEDANTIC(self != NULL)

    LDL_MAC_cancel(self);

    if(self->ctx.joined){

        self->band[LDL_BAND_GLOBAL] = 0;

        forgetNetwork(self);

        pushSessionUpdate(self);
    }
}

void LDL_MAC_cancel(struct ldl_mac *self)
{
    LDL_PEDANTIC(self != NULL)

    enum ldl_mac_operation op = self->op;

    self->op = LDL_OP_NONE;

    switch(self->state){
    default:

        LDL_MAC_timerClear(self, LDL_TIMER_WAITA);
        LDL_MAC_timerClear(self, LDL_TIMER_WAITB);
        inputDisarm(self);

        /* ensure the radio will return to a useful state */
        self->state = LDL_STATE_RADIO_RESET;
        self->radio_interface->set_mode(self->radio, LDL_RADIO_MODE_RESET);
        break;

    /* no need to touch radio in these states */
    case LDL_STATE_INIT:
    case LDL_STATE_RADIO_RESET:
    case LDL_STATE_RADIO_BOOT:
        break;
    }

    if(self->state == LDL_STATE_TX){

#ifdef LDL_ENABLE_TEST_MODE
        if(!self->unlimitedDutyCycle)
#endif
        {
            registerTime(self, &self->tx);
        }
    }

    switch(op){
    default:
    case LDL_OP_NONE:
        /* do nothing */
        break;
    case LDL_OP_ENTROPY:
    case LDL_OP_JOINING:
    case LDL_OP_REJOINING:
    case LDL_OP_DATA_UNCONFIRMED:
    case LDL_OP_DATA_CONFIRMED:

        self->handler(self->app, LDL_MAC_OP_CANCELLED, NULL);
        break;
    }
}

void LDL_MAC_process(struct ldl_mac *self)
{
    LDL_PEDANTIC(self != NULL)

    enum ldl_mac_sme event;
    uint32_t lag = 0;
    bool channel_ready;

    channel_ready = processBands(self);

    if(inputCheck(self, &lag)){

        event = LDL_SME_INTERRUPT;
    }
    else if(LDL_MAC_timerCheck(self, LDL_TIMER_WAITA, &lag)){

        event = LDL_SME_TIMER_A;
    }
    else if(LDL_MAC_timerCheck(self, LDL_TIMER_WAITB, &lag)){

        event = LDL_SME_TIMER_B;
    }
    else if(channel_ready){

        event = LDL_SME_BAND;
    }
    else{

        event = LDL_SME_NONE;
    }

    if(event != LDL_SME_NONE){

        switch(self->state){
        default:
        case LDL_STATE_IDLE:
            /* do nothing */
            break;

        case LDL_STATE_INIT:

            processInit(self);
            break;

        case LDL_STATE_RADIO_RESET:

            processRadioReset(self, event);
            break;

        case LDL_STATE_RADIO_BOOT:

            processRadioBoot(self, event);
            break;

        case LDL_STATE_WAIT_ENTROPY:
        case LDL_STATE_WAIT_TX:
        case LDL_STATE_WAIT_RX1:
        case LDL_STATE_WAIT_RX2:

            processWait(self, event, lag);
            break;

        case LDL_STATE_START_RADIO_FOR_ENTROPY:

            processStartRadioForEntropy(self, event);
            break;

        case LDL_STATE_ENTROPY:

            processEntropy(self, event);
            break;

        case LDL_STATE_WAIT_OTAA:

            processWaitOTAA(self, event);
            break;

        case LDL_STATE_START_RADIO_FOR_TX:

            processStartRadioForTX(self, event);
            break;

        case LDL_STATE_TX:

            processTX(self, event, lag);
            break;

        case LDL_STATE_START_RADIO_FOR_RX1:

            processStartRadioForRX1(self, event, lag);
            break;

        case LDL_STATE_START_RADIO_FOR_RX2:

            processStartRadioForRX2(self, event, lag);
            break;

        case LDL_STATE_RX1:
        case LDL_STATE_RX2:

            processRX(self, event);
            break;

        case LDL_STATE_RX2_LOCKOUT:

            processRX2Lockout(self, event);
            break;
        }
    }

    setNextBandEvent(self);
}

uint32_t LDL_MAC_ticksUntilNextEvent(const struct ldl_mac *self)
{
    LDL_PEDANTIC(self != NULL)

    uint32_t retval = 0U;

    if(!inputPending(self)){

        retval = LDL_MAC_timerTicksUntilNext(self);
    }

    return retval;
}

enum ldl_mac_status LDL_MAC_setRate(struct ldl_mac *self, uint8_t rate)
{
    LDL_PEDANTIC(self != NULL)

    enum ldl_mac_status retval;

    if(rateSettingIsValid(self->ctx.region, rate)){

        self->ctx.rate = rate;

        pushSessionUpdate(self);

        retval = LDL_STATUS_OK;
    }
    else{

        retval = LDL_STATUS_RATE;
    }

    return retval;
}

uint8_t LDL_MAC_getRate(const struct ldl_mac *self)
{
    LDL_PEDANTIC(self != NULL)

    return self->ctx.rate;
}

enum ldl_mac_status LDL_MAC_setPower(struct ldl_mac *self, uint8_t power)
{
    LDL_PEDANTIC(self != NULL)

    enum ldl_mac_status retval;

    if(LDL_Region_validateTXPower(self->ctx.region, power)){

        self->ctx.power = power;

        pushSessionUpdate(self);

        retval = LDL_STATUS_OK;
    }
    else{

        retval = LDL_STATUS_POWER;
    }

    return retval;
}

uint8_t LDL_MAC_getPower(const struct ldl_mac *self)
{
    LDL_PEDANTIC(self != NULL)

    return self->ctx.power;
}

void LDL_MAC_setADR(struct ldl_mac *self, bool value)
{
    LDL_PEDANTIC(self != NULL)

    self->ctx.adr = value;

    pushSessionUpdate(self);
}

bool LDL_MAC_getADR(const struct ldl_mac *self)
{
    LDL_PEDANTIC(self != NULL)

    return self->ctx.adr;
}

bool LDL_MAC_ready(const struct ldl_mac *self)
{
    LDL_PEDANTIC(self != NULL)

    bool retval = false;

    if(self->state == LDL_STATE_IDLE){

        retval = (timeUntilNextChannel(self) == 0U);
    }

    return retval;
}

void LDL_MAC_radioEvent(struct ldl_mac *self)
{
    LDL_MAC_radioEventWithTicks(self, self->ticks(self->app));
}

void LDL_MAC_radioEventWithTicks(struct ldl_mac *self, uint32_t ticks)
{
    LDL_PEDANTIC(self != NULL)

    inputSignal(self, ticks);
}

uint8_t LDL_MAC_mtu(const struct ldl_mac *self)
{
    LDL_PEDANTIC(self != NULL)

    enum ldl_signal_bandwidth bw;
    enum ldl_spreading_factor sf;
    uint8_t max = 0U;
    uint8_t overhead = LDL_Frame_dataOverhead();
    size_t i;
    uint8_t min_rate;
    uint8_t max_rate;
    uint8_t rate = self->ctx.rate;
    uint32_t freq;

    for(i=0; i<LDL_Region_numChannels(self->ctx.region); i++){

        if(!channelIsMasked(self->ctx.chMask, sizeof(self->ctx.chMask), self->ctx.region, U8(i))){

            if(getChannel(self, U8(i), &freq, &min_rate, &max_rate)){

                if(freq > 0U){

                    rate = requiredRate(rate, min_rate, max_rate);
                }
            }
        }
    }

    LDL_Region_convertRate(self->ctx.region, rate, &sf, &bw, &max);

    LDL_PEDANTIC(LDL_Frame_dataOverhead() < max)

    if(self->ctx.joined){

        overhead += commandIsPending(self, LDL_CMD_LINK_ADR) ? LDL_MAC_sizeofCommandUp(LDL_CMD_LINK_ADR) : 0U;
        overhead += commandIsPending(self, LDL_CMD_DUTY_CYCLE) ? LDL_MAC_sizeofCommandUp(LDL_CMD_DUTY_CYCLE) : 0U;
        overhead += commandIsPending(self, LDL_CMD_RX_PARAM_SETUP) ? LDL_MAC_sizeofCommandUp(LDL_CMD_RX_PARAM_SETUP) : 0U;
        overhead += commandIsPending(self, LDL_CMD_DEV_STATUS) ? LDL_MAC_sizeofCommandUp(LDL_CMD_DEV_STATUS) : 0U;
        overhead += commandIsPending(self, LDL_CMD_NEW_CHANNEL) ? LDL_MAC_sizeofCommandUp(LDL_CMD_NEW_CHANNEL) : 0U;
        overhead += commandIsPending(self, LDL_CMD_RX_TIMING_SETUP) ? LDL_MAC_sizeofCommandUp(LDL_CMD_RX_TIMING_SETUP) : 0U;
        overhead += commandIsPending(self, LDL_CMD_DL_CHANNEL) ? LDL_MAC_sizeofCommandUp(LDL_CMD_DL_CHANNEL) : 0U;
#if defined(LDL_ENABLE_L2_1_1)
        overhead += commandIsPending(self, LDL_CMD_REKEY) ? LDL_MAC_sizeofCommandUp(LDL_CMD_REKEY) : 0U;
        overhead += commandIsPending(self, LDL_CMD_ADR_PARAM_SETUP) ? LDL_MAC_sizeofCommandUp(LDL_CMD_ADR_PARAM_SETUP) : 0U;
        overhead += commandIsPending(self, LDL_CMD_REJOIN_PARAM_SETUP) ? LDL_MAC_sizeofCommandUp(LDL_CMD_REJOIN_PARAM_SETUP) : 0U;
#endif
#ifdef LDL_ENABLE_CLASS_B
        overhead += commandIsPending(self, LDL_CMD_PING_SLOT_INFO) ? LDL_MAC_sizeofCommandUp(LDL_CMD_PING_SLOT_INFO) : 0U;
        overhead += commandIsPending(self, LDL_CMD_PING_SLOT_CHANNEL) ? LDL_MAC_sizeofCommandUp(LDL_CMD_PING_SLOT_CHANNEL) : 0U;
        overhead += commandIsPending(self, LDL_CMD_BEACON_FREQ) ? LDL_MAC_sizeofCommandUp(LDL_CMD_BEACON_FREQ) : 0U;
#endif
    }

    return (overhead > max) ? 0U : U8(max - overhead);
}

void LDL_MAC_setMaxDCycle(struct ldl_mac *self, uint8_t maxDCycle)
{
    LDL_PEDANTIC(self != NULL)

    self->maxDutyCycle = maxDCycle & 0xfU;
    self->ctx.maxDutyCycle = self->maxDutyCycle;

    pushSessionUpdate(self);
}

uint8_t LDL_MAC_getMaxDCycle(const struct ldl_mac *self)
{
    LDL_PEDANTIC(self != NULL)

    return self->ctx.maxDutyCycle;
}

bool LDL_MAC_addChannel(struct ldl_mac *self, uint8_t chIndex, uint32_t freq, uint8_t minRate, uint8_t maxRate)
{
    LDL_PEDANTIC(self != NULL)

    LDL_DEBUG("chIndex=%u freq=%" PRIu32 " minRate=%u maxRate=%u", chIndex, freq, minRate, maxRate)

    return setChannel(self, chIndex, freq, minRate, maxRate);
}

bool LDL_MAC_maskChannel(struct ldl_mac *self, uint8_t chIndex)
{
    LDL_PEDANTIC(self != NULL)

    LDL_DEBUG("chIndex=%u", chIndex)

    return maskChannel(self->ctx.chMask, sizeof(self->ctx.chMask), self->ctx.region, chIndex);
}

bool LDL_MAC_unmaskChannel(struct ldl_mac *self, uint8_t chIndex)
{
    LDL_PEDANTIC(self != NULL)

    LDL_DEBUG("chIndex=%u", chIndex)

    return unmaskChannel(self->ctx.chMask, sizeof(self->ctx.chMask), self->ctx.region, chIndex);
}

void LDL_MAC_timerSet(struct ldl_mac *self, enum ldl_timer_inst timer, uint32_t timeout)
{
    LDL_SYSTEM_ENTER_CRITICAL(self->app)

    self->timers[timer].time = self->ticks(self->app) + (timeout & U32(INT32_MAX));
    self->timers[timer].armed = true;

    LDL_SYSTEM_LEAVE_CRITICAL(self->app)
}

void LDL_MAC_timerAppend(struct ldl_mac *self, enum ldl_timer_inst timer, uint32_t timeout)
{
    LDL_SYSTEM_ENTER_CRITICAL(self->app)

    self->timers[timer].time += (timeout & U32(INT32_MAX));
    self->timers[timer].armed = true;

    LDL_SYSTEM_LEAVE_CRITICAL(self->app)
}

bool LDL_MAC_timerCheck(struct ldl_mac *self, enum ldl_timer_inst timer, uint32_t *lag)
{
    bool retval = false;
    uint32_t time;

    LDL_SYSTEM_ENTER_CRITICAL(self->app)

    if(self->timers[timer].armed){

        time = self->ticks(self->app);

        if(timerDelta(self->timers[timer].time, time) < U32(INT32_MAX)){

            self->timers[timer].armed = false;
            *lag = timerDelta(self->timers[timer].time, time);
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

    time = self->ticks(self->app);

    for(i=0U; i < sizeof(self->timers)/sizeof(*self->timers); i++){

        LDL_SYSTEM_ENTER_CRITICAL(self->app)

        if(self->timers[i].armed){

            if(timerDelta(self->timers[i].time, time) <= U32(INT32_MAX)){

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

uint32_t LDL_MAC_timerTicksUntil(const struct ldl_mac *self, enum ldl_timer_inst timer, uint32_t *lag)
{
    uint32_t retval = UINT32_MAX;
    uint32_t time;

    LDL_SYSTEM_ENTER_CRITICAL(self->app)

    if(self->timers[timer].armed){

        time = self->ticks(self->app);

        *lag = timerDelta(self->timers[timer].time, time);

        if(*lag <= U32(INT32_MAX)){

            retval = 0U;
        }
        else{

            retval = timerDelta(time, self->timers[timer].time);
        }
    }

    LDL_SYSTEM_LEAVE_CRITICAL(self->app)

    return retval;
}

bool LDL_MAC_priority(const struct ldl_mac *self, uint8_t interval)
{
    LDL_PEDANTIC(self != NULL)

    bool retval;

    /* todo */
    (void)interval;

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

uint32_t LDL_MAC_getTicks(struct ldl_mac *self)
{
    LDL_PEDANTIC(self != NULL)

    return self->ticks(self->app);
}

#ifdef LDL_ENABLE_TEST_MODE
void LDL_MAC_setUnlimitedDutyCycle(struct ldl_mac *self, bool value)
{
    LDL_PEDANTIC(self != NULL)

    self->unlimitedDutyCycle = value;
}
#endif

bool LDL_MAC_getFPending(const struct ldl_mac *self)
{
    LDL_PEDANTIC(self != NULL)

    return self->fPending;
}

bool LDL_MAC_getAckPending(const struct ldl_mac *self)
{
    LDL_PEDANTIC(self != NULL)

    return self->pendingACK;
}

/* static functions ***************************************************/

static void processInit(struct ldl_mac *self)
{
    self->state = LDL_STATE_RADIO_RESET;

    self->radio_interface->set_mode(self->radio, LDL_RADIO_MODE_RESET);

    /* >100us */
    LDL_MAC_timerSet(self, LDL_TIMER_WAITA, GET_TPS()/U32(1024));

    LDL_DEBUG("set radio reset: ticks=%" PRIu32 "",
        self->ticks(self->app)
    )
}

static void processRadioReset(struct ldl_mac *self, enum ldl_mac_sme event)
{
    if(event == LDL_SME_TIMER_A){

        self->state = LDL_STATE_RADIO_BOOT;
        self->radio_interface->set_mode(self->radio, LDL_RADIO_MODE_BOOT);

        /* >5ms to startup */
        LDL_MAC_timerSet(self, LDL_TIMER_WAITA, GET_TPS()/U32(128));

        LDL_DEBUG("clear radio reset: ticks=%" PRIu32 "",
            self->ticks(self->app)
        )
    }
}

static void processRadioBoot(struct ldl_mac *self, enum ldl_mac_sme event)
{
    if(event == LDL_SME_TIMER_A){

        switch(self->op){
        case LDL_OP_ENTROPY:

            self->radio_interface->set_mode(self->radio, LDL_RADIO_MODE_SLEEP);
            self->state = LDL_STATE_WAIT_ENTROPY;
            LDL_MAC_timerSet(self, LDL_TIMER_WAITA, 0);
            break;

        case LDL_OP_JOINING:

            self->radio_interface->set_mode(self->radio, LDL_RADIO_MODE_SLEEP);
            self->state = LDL_STATE_WAIT_OTAA;
            LDL_MAC_timerSet(self, LDL_TIMER_WAITA, 0);
            break;

        case LDL_OP_DATA_CONFIRMED:
        case LDL_OP_DATA_UNCONFIRMED:

            self->radio_interface->set_mode(self->radio, LDL_RADIO_MODE_SLEEP);
            self->state = LDL_STATE_WAIT_TX;
            LDL_MAC_timerSet(self, LDL_TIMER_WAITA, 0);
            break;

        default:
            self->radio_interface->set_mode(self->radio, LDL_RADIO_MODE_SLEEP);
            self->state = LDL_STATE_IDLE;
            break;
        }
    }
}

static void processStartRadioForEntropy(struct ldl_mac *self, enum ldl_mac_sme event)
{
    if(event == LDL_SME_TIMER_A){

        self->radio_interface->receive_entropy(self->radio);

        self->state = LDL_STATE_ENTROPY;

        /* ~1ms */
        LDL_MAC_timerSet(self, LDL_TIMER_WAITA, GET_TPS()/U32(1024));

        LDL_DEBUG("listen for entropy: ticks=%" PRIu32 "", self->ticks(self->app))
    }
}

static void processEntropy(struct ldl_mac *self, enum ldl_mac_sme event)
{
    union ldl_mac_response_arg arg;

    if(event == LDL_SME_TIMER_A){

        arg.entropy.value = self->radio_interface->read_entropy(self->radio);

        self->radio_interface->set_mode(self->radio, LDL_RADIO_MODE_SLEEP);

        self->state = LDL_STATE_IDLE;
        self->op = LDL_OP_NONE;

        LDL_DEBUG("read entropy: ticks=%" PRIu32 " entropy=%" PRIu32 "",
            self->ticks(self->app),
            arg.entropy.value
        )

        self->handler(self->app, LDL_MAC_ENTROPY, &arg);
    }
}

static void processWaitOTAA(struct ldl_mac *self, enum ldl_mac_sme event)
{
    uint32_t delay;

    (void)event;

    if(self->band[LDL_BAND_GLOBAL] == 0U){

#ifdef LDL_ENABLE_OTAA_DITHER
        if(self->otaaDither > 0U){

            delay = self->rand(self->app) % (GET_TPS()*U32(self->otaaDither));
        }
        else{

            delay = 0;
        }
#else
        delay = self->rand(self->app) % (GET_TPS()*U32(30));
#endif
        LDL_DEBUG("add dither to otaa: ticks=%" PRIu32 " delay=%" PRIu32 "",
            self->ticks(self->app),
            delay
        )

        LDL_MAC_timerSet(self, LDL_TIMER_WAITA, delay);
        self->state = LDL_STATE_WAIT_TX;
    }
}

static void processWait(struct ldl_mac *self, enum ldl_mac_sme event, uint32_t lag)
{
    uint32_t delay;
    enum ldl_timer_inst timer;

    switch(event){
    case LDL_SME_TIMER_A:
    case LDL_SME_TIMER_B:

        delay = msToTicks(self, LDL_PARAM_XTAL_DELAY);

        switch(self->state){
        default:
        case LDL_STATE_WAIT_TX:
            self->state = LDL_STATE_START_RADIO_FOR_TX;
            self->radio_interface->set_mode(self->radio, LDL_RADIO_MODE_TX);
            timer = LDL_TIMER_WAITA;
            break;
        case LDL_STATE_WAIT_RX1:
            self->state = LDL_STATE_START_RADIO_FOR_RX1;
            self->radio_interface->set_mode(self->radio, LDL_RADIO_MODE_RX);
            timer = LDL_TIMER_WAITA;
            break;
        case LDL_STATE_WAIT_RX2:
            self->state = LDL_STATE_START_RADIO_FOR_RX2;
            self->radio_interface->set_mode(self->radio, LDL_RADIO_MODE_RX);
            timer = LDL_TIMER_WAITB;
            break;
        case LDL_STATE_WAIT_ENTROPY:
            self->state = LDL_STATE_START_RADIO_FOR_ENTROPY;
            self->radio_interface->set_mode(self->radio, LDL_RADIO_MODE_RX);
            timer = LDL_TIMER_WAITA;
            break;
        }

        delay = (lag > delay) ? 0U : (delay - lag);

        LDL_MAC_timerAppend(self, timer, delay);
#if 0
        LDL_DEBUG("start xtal: ticks=%" PRIu32 " delay=%" PRIu32 "",
            self->ticks(self->app),
            delay
        )
#endif
        break;

    default:
        /* nothing */
        break;
    }
}

static void processStartRadioForTX(struct ldl_mac *self, enum ldl_mac_sme event)
{
    struct ldl_radio_tx_setting setting;
    uint8_t mtu;
    uint32_t ms;

    if(event == LDL_SME_TIMER_A){

        LDL_Region_convertRate(self->ctx.region, self->tx.rate, &setting.sf, &setting.bw, &mtu);

        setting.eirp = LDL_Region_getTXPower(self->ctx.region, self->tx.power);

#ifndef LDL_DISABLE_TX_PARAM_SETUP
        if(LDL_Region_txParamSetupImplemented(self->ctx.region)){

            static const int8_t maxEIRP[] = {
                8,
                10,
                12,
                13,
                14,
                16,
                18,
                20,
                21,
                24,
                26,
                27,
                29,
                30,
                33,
                36
            };

            int16_t max_eirp = (int16_t)maxEIRP[self->ctx.tx_param_setup & 0xfU];

            max_eirp *= 100;

            if(setting.eirp > max_eirp){

                setting.eirp = max_eirp;
            }
        }
#endif
        setting.freq = self->tx.freq;

        ms = LDL_Radio_getAirTime(setting.bw, setting.sf, self->bufferLen, true);

        self->tx.airTime = msToTime(ms);

        inputArm(self);

        self->radio_interface->transmit(self->radio, &setting, self->buffer, self->bufferLen);

        self->state = LDL_STATE_TX;

        /* reset the radio if the tx complete interrupt doesn't appear after double the expected time */
        LDL_MAC_timerSet(self, LDL_TIMER_WAITA, msToTicks(self, ms) << 1);

        LDL_INFO("tx begin")
        LDL_DEBUG("ticks=%" PRIu32 " freq=%" PRIu32 " power=%u  bw=%" PRIu32 " sf=%u size=%u",
            self->ticks(self->app),
            self->tx.freq,
            self->tx.power,
            LDL_Radio_bwToNumber(setting.bw),
            U8(setting.sf),
            self->bufferLen
        )
    }
}

static void processTX(struct ldl_mac *self, enum ldl_mac_sme event, uint32_t lag)
{
    uint32_t waitSeconds;
    uint32_t waitTicks;
    uint32_t advance;
    uint32_t advanceA;
    uint32_t advanceB;
    enum ldl_spreading_factor sf;
    enum ldl_signal_bandwidth bw;
    uint8_t rate;
    uint32_t extra_symbols;
    uint32_t xtal_error;
    uint8_t mtu;
    uint32_t margin;
    struct ldl_radio_status status;

    (void)memset(&status, 0, sizeof(status));

    if(event == LDL_SME_INTERRUPT){

        self->radio_interface->get_status(self->radio, &status);
    }

    if((event == LDL_SME_INTERRUPT) || (event == LDL_SME_TIMER_A)){

#ifdef LDL_ENABLE_TEST_MODE
        if(!self->unlimitedDutyCycle)
#endif
        {
            registerTime(self, &self->tx);
        }
    }

    if(event == LDL_SME_TIMER_A){

        LDL_ERROR("interrupt fault")
        LDL_DEBUG("ticks=%" PRIu32 "", self->ticks(self->app))
        handleRadioError(self);
    }
    else if((event == LDL_SME_INTERRUPT) && !status.tx){

        LDL_ERROR("unexpected status")
        LDL_DEBUG("ticks=%" PRIu32 "", self->ticks(self->app))
        handleRadioError(self);
    }
    else if((event == LDL_SME_INTERRUPT) && status.tx){

        self->pendingACK = false;

        /* the wait interval is always measured in whole seconds */
        waitSeconds = (self->op == LDL_OP_JOINING) ? U32(LDL_Region_getJA1Delay(self->ctx.region)) : U32(self->ctx.rx1Delay);

        /* the ideal interval measured in ticks */
        waitTicks = waitSeconds * GET_TPS();

#ifndef LDL_DISABLE_DEVICE_TIME
        self->ticks_at_tx = self->ticks(self->app) - lag;
#endif
        advance = GET_ADVANCE() + lag + msToTicks(self, LDL_PARAM_XTAL_DELAY);

        /* RX1 */
        {
            LDL_Region_getRX1DataRate(self->ctx.region, self->tx.rate, self->ctx.rx1DROffset, &rate);
            LDL_Region_convertRate(self->ctx.region, rate, &sf, &bw, &mtu);

            xtal_error = (waitSeconds * GET_A() * U32(2)) + GET_B();

            extra_symbols = extraSymbols(xtal_error, symbolPeriod(GET_TPS(), sf, bw));

            /* we need a minimum of 3 extra symbols */
            extra_symbols = (extra_symbols < U32(3)) ? U32(3) : extra_symbols;

            margin = extra_symbols * symbolPeriod(GET_TPS(), sf, bw);
            self->rx1_symbols = U16(5) + U16(extra_symbols);

            /* advance timer by time required for extra symbols */
            advanceA = advance + (margin/U32(2));
        }

        /* RX2 */
        {
            LDL_Region_convertRate(self->ctx.region, self->ctx.rx2DataRate, &sf, &bw, &mtu);

            xtal_error += (GET_A() * U32(2));

            extra_symbols = extraSymbols(xtal_error, symbolPeriod(GET_TPS(), sf, bw));

            /* we need a minimum of 3 extra symbols */
            extra_symbols = (extra_symbols < U32(3)) ? U32(3) : extra_symbols;

            margin = extra_symbols * symbolPeriod(GET_TPS(), sf, bw);
            self->rx2_symbols = U16(5) + U16(extra_symbols);

            /* advance timer by time required for extra symbols */
            advanceB = advance + (margin/U32(2));
        }

        if(advanceB <= (waitTicks + GET_TPS())){

            LDL_MAC_timerSet(self, LDL_TIMER_WAITB, waitTicks + GET_TPS() - advanceB);

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

        self->radio_interface->set_mode(self->radio, LDL_RADIO_MODE_HOLD);

        LDL_INFO("tx complete")
        LDL_DEBUG("ticks=%" PRIu32 "", self->ticks(self->app))
    }
    else{

        /* nothing */
    }
}

static void processStartRadioForRX1(struct ldl_mac *self, enum ldl_mac_sme event, uint32_t lag)
{
    struct ldl_radio_rx_setting setting;
    uint32_t freq;
    uint8_t rate;

    (void)lag;

    if(event == LDL_SME_TIMER_A){

        LDL_Region_getRX1DataRate(self->ctx.region, self->tx.rate, self->ctx.rx1DROffset, &rate);
        LDL_Region_getRX1Freq(self->ctx.region, self->tx.freq, self->tx.chIndex, &freq);

        LDL_Region_convertRate(self->ctx.region, rate, &setting.sf, &setting.bw, &setting.max);

        setting.max += LDL_Frame_phyOverhead();

        self->state = LDL_STATE_RX1;

        setting.freq = freq;
        setting.timeout = self->rx1_symbols;

        inputArm(self);

        self->radio_interface->receive(self->radio, &setting);

        /* use waitA as a guard (timeout after ~4 seconds) */
        LDL_MAC_timerSet(self, LDL_TIMER_WAITA, (GET_TPS() + GET_A()) << 2U);

        LDL_INFO("rx1 slot")
        LDL_DEBUG("ticks=%" PRIu32 " timeout=%" PRIu16 " lag=%" PRIu32 " freq=%" PRIu32 " bw=%" PRIu32 " sf=%u",
            self->ticks(self->app),
            self->rx1_symbols,
            lag,
            freq,
            LDL_Radio_bwToNumber(setting.bw),
            U8(setting.sf)
        )
    }
}

static void processStartRadioForRX2(struct ldl_mac *self, enum ldl_mac_sme event, uint32_t lag)
{
    struct ldl_radio_rx_setting setting;

    if(event == LDL_SME_TIMER_B){

        LDL_Region_convertRate(self->ctx.region, self->ctx.rx2DataRate, &setting.sf, &setting.bw, &setting.max);

        setting.max += LDL_Frame_phyOverhead();

        self->state = LDL_STATE_RX2;

        setting.freq = self->ctx.rx2Freq;
        setting.timeout = self->rx2_symbols;

        inputArm(self);

        self->radio_interface->receive(self->radio, &setting);

        /* use waitA as a guard */
        LDL_MAC_timerSet(self, LDL_TIMER_WAITA, (GET_TPS() + GET_A()) * 4U);

        LDL_INFO("rx2 slot")
        LDL_DEBUG("ticks=%" PRIu32 " timeout=%" PRIu16 " lag=%" PRIu32 " freq=%" PRIu32 " bw=%" PRIu32 " sf=%u",
            self->ticks(self->app),
            self->rx2_symbols,
            lag,
            self->ctx.rx2Freq,
            LDL_Radio_bwToNumber(setting.bw),
            U8(setting.sf)
        )
    }
}

static void processRX(struct ldl_mac *self, enum ldl_mac_sme event)
{
    struct ldl_frame_down frame;
#ifdef LDL_ENABLE_STATIC_RX_BUFFER
    uint8_t *buffer = self->rx_buffer;
#else
    uint8_t buffer[LDL_MAX_PACKET];
#endif
    uint8_t len;
    struct ldl_radio_packet_metadata meta;
    uint8_t mtu;
    enum ldl_spreading_factor sf;
    enum ldl_signal_bandwidth bw;
    union ldl_mac_response_arg arg;
    uint32_t ms;

    struct ldl_radio_status status;

    (void)memset(&status, 0, sizeof(status));

    if(event == LDL_SME_INTERRUPT){

        self->radio_interface->get_status(self->radio, &status);
    }

    if(event == LDL_SME_TIMER_A){

        LDL_ERROR("interrupt fault")
        LDL_DEBUG("ticks=%" PRIu32 "", self->ticks(self->app))

        self->radio_interface->get_status(self->radio, &status);

        handleRadioError(self);
    }
    else if((event == LDL_SME_INTERRUPT) && !status.rx && !status.timeout){

        LDL_ERROR("unexpected status")
        LDL_DEBUG("ticks=%" PRIu32 "", self->ticks(self->app))

        handleRadioError(self);
    }
    else if((event == LDL_SME_INTERRUPT) && status.rx){

        LDL_MAC_timerClear(self, LDL_TIMER_WAITA);
        LDL_MAC_timerClear(self, LDL_TIMER_WAITB);

        len = self->radio_interface->read_buffer(self->radio, &meta, buffer, LDL_MAX_PACKET);

        self->radio_interface->set_mode(self->radio, LDL_RADIO_MODE_SLEEP);

        self->rx_snr = meta.snr;

        LDL_DEBUG("downlink: ticks=%" PRIu32 " rssi=%d snr=%d size=%u",
            self->ticks(self->app),
            meta.rssi,
            meta.snr,
            len
        )

        if(LDL_OPS_receiveFrame(self, &frame, buffer, len)){

            switch(frame.type){
            default:
            case FRAME_TYPE_JOIN_ACCEPT:

                self->ctx.joined = true;

                /* keep the joining rate */
                self->ctx.rate = self->ctx.adr ? LDL_Region_getJoinRate(self->ctx.region, self->trials) : self->ctx.rate;

                self->ctx.rx1DROffset = frame.rx1DataRateOffset;
                self->ctx.rx2DataRate = frame.rx2DataRate;
                self->ctx.rx1Delay = frame.rxDelay;

                if(frame.cfList != NULL){

                    LDL_Region_processCFList(self->ctx.region, self, frame.cfList, frame.cfListLen);
                }

                self->ctx.devAddr = frame.devAddr;

#if defined(LDL_ENABLE_L2_1_1)
                self->ctx.version = (frame.optNeg) ? 1U : 0U;

                if(SESS_VERSION(self->ctx) > 0U){

                    setPendingCommand(self, LDL_CMD_REKEY);
                }
#endif
                /* cache this so that the session keys can be re-derived */
                self->ctx.netID = frame.netID;
                self->ctx.joinNonce = frame.joinNonce;
                /* self->ctx.devNonce is already set */

                self->joinNonce = frame.joinNonce;

                LDL_OPS_deriveKeys(self);

                self->joinNonce++;

                LDL_INFO("join accept: joinNonce=%" PRIu32 " devNonce=%" PRIu16 " netID=%" PRIu32 " devAddr=%" PRIu32 " rx1Delay=%u",
                    self->ctx.joinNonce,
                    self->ctx.devNonce,
                    self->ctx.netID,
                    self->ctx.devAddr,
                    self->ctx.rx1Delay
                )

                self->band[LDL_BAND_GLOBAL] = 0;
                self->day = 0;
                self->state = LDL_STATE_IDLE;
                self->op = LDL_OP_NONE;

                arg.join_complete.joinNonce = self->joinNonce;
                arg.join_complete.netID = self->ctx.netID;
                arg.join_complete.devAddr = self->ctx.devAddr;

                self->handler(self->app, LDL_MAC_JOIN_COMPLETE, &arg);
                break;

            case FRAME_TYPE_DATA_CONFIRMED_DOWN:
            case FRAME_TYPE_DATA_UNCONFIRMED_DOWN:

                /* if set it means network has more data to send */
                self->fPending = frame.pending;

                self->pendingACK = (frame.type == FRAME_TYPE_DATA_CONFIRMED_DOWN);

                LDL_OPS_syncDownCounter(self, frame.port, frame.counter);

                clearPendingCommand(self, LDL_CMD_RX_PARAM_SETUP);
                clearPendingCommand(self, LDL_CMD_DL_CHANNEL);
                clearPendingCommand(self, LDL_CMD_RX_TIMING_SETUP);

                self->adrAckCounter = 0;
                self->adrAckReq = false;

                if(frame.opts != NULL){

                    processCommands(self, frame.opts, frame.optsLen);
                }

                if(frame.data != NULL){

                    if(frame.port == 0U){

                        processCommands(self, frame.data, frame.dataLen);
                    }
                    else{

                        arg.rx.port = frame.port;
                        arg.rx.data = frame.data;
                        arg.rx.size = frame.dataLen;

                        self->handler(self->app, LDL_MAC_RX, &arg);
                    }
                }

                switch(self->op){
                default:
                case LDL_OP_DATA_UNCONFIRMED:

                    self->handler(self->app, LDL_MAC_DATA_COMPLETE, NULL);
                    break;

                case LDL_OP_DATA_CONFIRMED:

                    if(frame.ack){

                        self->handler(self->app, LDL_MAC_DATA_COMPLETE, NULL);
                    }
                    else{

                        LDL_DEBUG("NAK received in response to confirmed uplink")

                        /* I don't see how this would ever happen
                         * in practice since downlinks are sent
                         * in response to having receied an uplink.
                         *
                         * For this reason simply handle it as a timeout
                         * regardless of the number of attempts requested.
                         *
                         *  */
                        self->handler(self->app, LDL_MAC_DATA_TIMEOUT, NULL);
                    }
                    break;

                case LDL_OP_REJOINING:
                    break;
                }

                self->state = LDL_STATE_IDLE;
                self->op = LDL_OP_NONE;
                break;
            }

            pushSessionUpdate(self);
        }
        else{

            downlinkMissingHandler(self);
        }
    }
    else if((event == LDL_SME_INTERRUPT) && status.timeout){

        if(self->state == LDL_STATE_RX2){

            self->radio_interface->set_mode(self->radio, LDL_RADIO_MODE_SLEEP);

            LDL_MAC_timerClear(self, LDL_TIMER_WAITB);

            LDL_Region_convertRate(self->ctx.region, self->tx.rate, &sf, &bw, &mtu);

            ms = LDL_Radio_getAirTime(bw, sf, mtu, false);

            LDL_MAC_timerSet(self, LDL_TIMER_WAITA, msToTicks(self, ms));

            self->state = LDL_STATE_RX2_LOCKOUT;
        }
        else{

            self->radio_interface->set_mode(self->radio, LDL_RADIO_MODE_HOLD);

            LDL_MAC_timerClear(self, LDL_TIMER_WAITA);

            self->state = LDL_STATE_WAIT_RX2;
        }
    }
    else{

        /* nothing */
    }
}

static void processRX2Lockout(struct ldl_mac *self, enum ldl_mac_sme event)
{
    if(event == LDL_SME_TIMER_A){

        downlinkMissingHandler(self);
    }
}

static void handleRadioError(struct ldl_mac *self)
{
    inputDisarm(self);
    LDL_MAC_timerClear(self, LDL_TIMER_WAITA);
    LDL_MAC_timerClear(self, LDL_TIMER_WAITB);

    switch(self->op){
    default:
    case LDL_OP_NONE:
    case LDL_OP_DATA_CONFIRMED:
    case LDL_OP_DATA_UNCONFIRMED:
    case LDL_OP_ENTROPY:
        self->op = LDL_OP_NONE;
        self->handler(self->app, LDL_MAC_OP_ERROR, NULL);
        break;

    case LDL_OP_JOINING:

        /* joining will continue after the radio is reset so we need
         * to setup the next channel
         *
         * */
        downlinkMissingHandler(self);
        break;
    }


    self->state = LDL_STATE_RADIO_RESET;

    self->radio_interface->set_mode(self->radio, LDL_RADIO_MODE_RESET);

    /* >100us */
    LDL_MAC_timerSet(self, LDL_TIMER_WAITA, (GET_TPS()/U32(1024)));


    LDL_DEBUG("radio fault detected, initiating radio reset")
}


static enum ldl_mac_status externalDataCommand(struct ldl_mac *self, bool confirmed, uint8_t port, const void *data, uint8_t len, const struct ldl_mac_data_opts *opts)
{
    enum ldl_mac_status retval;
    uint8_t maxPayload;
    enum ldl_signal_bandwidth bw;
    enum ldl_spreading_factor sf;
    struct ldl_frame_data f;
    struct ldl_stream s;
    uint8_t macs[30]; // large enough for all possible MAC commands
    size_t desired_len = len + (size_t)LDL_Frame_dataOverhead();

    if(self->ctx.joined){

        if(self->op == LDL_OP_NONE){

            if((port > 0U) && (port <= 223U)){

                if(self->band[LDL_BAND_GLOBAL] == 0U){

                    /* set desired power and rate */
                    self->tx.power = self->ctx.power;
#ifdef LDL_DISABLE_TX_PARAM_SETUP
                    self->tx.rate = self->ctx.rate;
#else
                    self->tx.rate = LDL_Region_applyUplinkDwell(self->ctx.region, uplinkDwell(self->ctx.tx_param_setup), self->ctx.rate);
#endif
                    if(selectChannel(self, self->ctx.rate, 0U, &self->tx)){

                        LDL_Region_convertRate(self->ctx.region, self->ctx.rate, &sf, &bw, &maxPayload);

                        if(desired_len <= (size_t)maxPayload){

                            (void)memset(&self->opts, 0, sizeof(self->opts));

                            if(opts != NULL){

                                (void)memcpy(&self->opts, opts, sizeof(self->opts));
                            }

                            self->opts.nbTrans = self->opts.nbTrans & 0xfU;

                            self->trials = 0;

                            (void)memset(&f, 0, sizeof(f));

                            self->op = confirmed ? LDL_OP_DATA_CONFIRMED : LDL_OP_DATA_UNCONFIRMED;

                            f.type = confirmed ? FRAME_TYPE_DATA_CONFIRMED_UP : FRAME_TYPE_DATA_UNCONFIRMED_UP;
                            f.devAddr = self->ctx.devAddr;
                            f.counter = U16(self->ctx.up);
                            f.adr = self->ctx.adr;
                            f.adrAckReq = self->adrAckReq;
                            f.port = port;

                            /* this might be wrong - have one try at
                             * responding to a confirmed downlink */
                            f.ack = self->pendingACK;

                            /* 1.1 has to awkwardly re-calculate the MIC when a frame is retried on a
                             * different channel and the counter is a parameter */
                            self->tx.counter = self->ctx.up;

                            self->ctx.up++;

                            /* serialise pending MAC commands */

                            LDL_Stream_init(&s, macs, U8(sizeof(macs)));

                            LDL_DEBUG("preparing data frame")

                            /* sticky commands */
#if defined(LDL_ENABLE_L2_1_1)
                            if(commandIsPending(self, LDL_CMD_REKEY)){

                                struct ldl_rekey_ind ind = {
                                    .version = self->ctx.version
                                };

                                LDL_MAC_putRekeyInd(&s, &ind);

                                LDL_DEBUG("adding rekey_ind: version=%u", self->ctx.version)
                            }
#endif
                            if(commandIsPending(self, LDL_CMD_RX_PARAM_SETUP)){

                                LDL_MAC_putRXParamSetupAns(&s, &self->ctx.rx_param_setup_ans);

                                LDL_DEBUG("adding rx_param_setup_ans: rx1DROffsetOK=%s rx2DataRate=%s rx2Freq=%s",
                                    self->ctx.rx_param_setup_ans.rx1DROffsetOK ? "true" : "false",
                                    self->ctx.rx_param_setup_ans.rx2DataRateOK ? "true" : "false",
                                    self->ctx.rx_param_setup_ans.channelOK ? "true" : "false"
                                )
                            }

                            if(commandIsPending(self, LDL_CMD_DL_CHANNEL)){

                                LDL_MAC_putDLChannelAns(&s, &self->ctx.dl_channel_ans);

                                LDL_DEBUG("adding dl_channel_ans: uplinkFreqOK=%s channelFreqOK=%s",
                                    self->ctx.dl_channel_ans.uplinkFreqOK ? "true" : "false",
                                    self->ctx.dl_channel_ans.channelFreqOK ? "true" : "false"
                                )
                            }

                            if(commandIsPending(self, LDL_CMD_RX_TIMING_SETUP)){

                                LDL_MAC_putRXTimingSetupAns(&s);
                                LDL_DEBUG("adding rx_timing_setup_ans")
                            }

                            /* single shot commands */

                            if(commandIsPending(self, LDL_CMD_LINK_ADR)){

                                LDL_MAC_putLinkADRAns(&s, &self->ctx.link_adr_ans);
                                clearPendingCommand(self, LDL_CMD_LINK_ADR);

                                LDL_DEBUG("adding link_adr_ans: powerOK=%s dataRateOK=%s channelMaskOK=%s",
                                    self->ctx.link_adr_ans.dataRateOK ? "true" : "false",
                                    self->ctx.link_adr_ans.powerOK ? "true" : "false",
                                    self->ctx.link_adr_ans.channelMaskOK ? "true" : "false"
                                )
                            }

                            if(commandIsPending(self, LDL_CMD_DEV_STATUS)){

                                LDL_MAC_putDevStatusAns(&s, &self->ctx.dev_status_ans);
                                clearPendingCommand(self, LDL_CMD_DEV_STATUS);

                                LDL_DEBUG("adding dev_status_ans: battery=%u margin=%i",
                                    self->ctx.dev_status_ans.battery,
                                    self->ctx.dev_status_ans.margin
                                )
                            }

                            if(commandIsPending(self, LDL_CMD_NEW_CHANNEL)){

                                LDL_MAC_putNewChannelAns(&s, &self->ctx.new_channel_ans);
                                clearPendingCommand(self, LDL_CMD_NEW_CHANNEL);

                                LDL_DEBUG("adding new_channel_ans: dataRateRangeOK=%s channelFreqOK=%s",
                                    self->ctx.new_channel_ans.dataRateRangeOK ? "true" : "false",
                                    self->ctx.new_channel_ans.channelFreqOK ? "true" : "false"
                                )
                            }
#if defined(LDL_ENABLE_L2_1_1)
                            if(commandIsPending(self, LDL_CMD_REJOIN_PARAM_SETUP)){

                                LDL_MAC_putRejoinParamSetupAns(&s, &self->ctx.rejoin_param_setup_ans);
                                clearPendingCommand(self, LDL_CMD_REJOIN_PARAM_SETUP);

                                LDL_DEBUG("adding rejoin_param_setup_ans: timeOK=%s",
                                    self->ctx.rejoin_param_setup_ans.timeOK ? "true" : "false"
                                )
                            }

                            if(commandIsPending(self, LDL_CMD_ADR_PARAM_SETUP)){

                                LDL_MAC_putADRParamSetupAns(&s);
                                clearPendingCommand(self, LDL_CMD_ADR_PARAM_SETUP);

                                LDL_DEBUG("adding adr_param_setup_ans")
                            }
#endif
#ifndef LDL_DISABLE_TX_PARAM_SETUP
                            if(commandIsPending(self, LDL_CMD_TX_PARAM_SETUP)){

                                LDL_MAC_putTXParamSetupAns(&s);
                                clearPendingCommand(self, LDL_CMD_TX_PARAM_SETUP);

                                LDL_DEBUG("adding tx_param_setup_ans")
                            }
#endif
                            if(commandIsPending(self, LDL_CMD_DUTY_CYCLE)){

                                LDL_MAC_putDutyCycleAns(&s);
                                clearPendingCommand(self, LDL_CMD_DUTY_CYCLE);

                                LDL_DEBUG("adding duty_cycle_ans")
                            }
#ifndef LDL_DISABLE_LINK_CHECK
                            if(self->opts.check){

                                LDL_MAC_putLinkCheckReq(&s);
                                LDL_DEBUG("adding link_check_req")
                            }
#endif
#ifndef LDL_DISABLE_DEVICE_TIME
                            if(self->opts.getTime){

                                LDL_MAC_putDeviceTimeReq(&s);
                                LDL_DEBUG("adding device_time_req")
                            }
#endif
                            /* create port 0 message and ignore application */
                            if(LDL_Stream_tell(&s) > 15U){

                                LDL_DEBUG("mac data has been prioritised")

                                f.type = FRAME_TYPE_DATA_UNCONFIRMED_UP;
                                self->op = LDL_OP_DATA_UNCONFIRMED;

                                f.port = 0U;
                                f.data = macs;
                                f.dataLen = LDL_Stream_tell(&s);

                                /* mac was prioritised */
                                retval = LDL_STATUS_MACPRIORITY;
                            }
                            /* ignore application */
                            else if((desired_len + (size_t)LDL_Stream_tell(&s)) > (size_t)maxPayload){

                                LDL_DEBUG("mac data has been prioritised")

                                f.type = FRAME_TYPE_DATA_UNCONFIRMED_UP;
                                self->op = LDL_OP_DATA_UNCONFIRMED;

                                f.opts = macs;
                                f.optsLen = LDL_Stream_tell(&s);

                                /* mac was prioritised */
                                retval = LDL_STATUS_MACPRIORITY;
                            }
                            /* mac and application fit together */
                            else{

                                f.opts = macs;
                                f.optsLen = LDL_Stream_tell(&s);

                                f.data = data;
                                f.dataLen = len;

                                /* indicate success to application */
                                retval = LDL_STATUS_OK;
                            }

                            self->bufferLen = LDL_OPS_prepareData(self, &f, self->buffer, U8(sizeof(self->buffer)));

                            LDL_OPS_micDataFrame(self, self->buffer, self->bufferLen);

                            if(self->state == LDL_STATE_IDLE){

                                self->state = LDL_STATE_WAIT_TX;
                                LDL_MAC_timerSet(self, LDL_TIMER_WAITA, 0U);
                            }

                            LDL_DEBUG("initiate data: ticks=%" PRIu32,
                                self->ticks(self->app)
                            )
                        }
                        else{

                            retval = LDL_STATUS_SIZE;
                        }
                    }
                    else{

                        retval = LDL_STATUS_NOCHANNEL;
                    }
                }
                else{

                    retval = LDL_STATUS_NOCHANNEL;
                }
            }
            else{

                retval = LDL_STATUS_PORT;
            }
        }
        else{

            retval = LDL_STATUS_BUSY;
        }
    }
    else{

        retval = LDL_STATUS_NOTJOINED;
    }

    return retval;
}

static bool adaptRate(struct ldl_mac *self)
{
    bool session_changed = false;

    self->adrAckReq = false;

    if(self->ctx.adr){

        if(self->adrAckCounter < U16(UINT8_MAX)){

            if(self->adrAckCounter >= self->ctx.adr_ack_limit){

                self->adrAckReq = true;

                LDL_DEBUG("adr: adrAckCounter=%u (past ADRAckLimit)", self->adrAckCounter)

                if(self->adrAckCounter >= (self->ctx.adr_ack_limit + self->ctx.adr_ack_delay)){

                    if(((self->adrAckCounter - (self->ctx.adr_ack_limit + self->ctx.adr_ack_delay)) % self->ctx.adr_ack_delay) == 0U){

                        if(self->ctx.power == 0U){

                            if(self->ctx.rate > U8(MIN_RATE)){

                                self->ctx.rate--;
                                LDL_DEBUG("adr: rate reduced to %u", self->ctx.rate)
                            }
                            else{

                                LDL_DEBUG("adr: all channels unmasked")

                                unmaskAllChannels(self->ctx.chMask, sizeof(self->ctx.chMask));

                                self->adrAckCounter = UINT8_MAX;
                            }
                        }
                        else{

                            LDL_DEBUG("adr: full power enabled")
                            self->ctx.power = 0U;
                        }

                        session_changed = true;
                    }
                }
            }

            self->adrAckCounter++;
        }
    }

    return session_changed;
}

static uint32_t symbolPeriod(uint32_t tps, enum ldl_spreading_factor sf, enum ldl_signal_bandwidth bw)
{
    return (((U32(1)) << sf) * tps) / LDL_Radio_bwToNumber(bw);
}

static uint32_t extraSymbols(uint32_t xtal_error, uint32_t symbol_period)
{
    return (xtal_error / symbol_period) + (((xtal_error % symbol_period) > 0U) ? U32(1) : U32(0));
}

static void processCommands(struct ldl_mac *self, const uint8_t *in, uint8_t len)
{
    struct ldl_stream s_in;

    /* rollback cache */
    uint8_t chMask[sizeof(self->ctx.chMask)];
    uint8_t nbTrans;
    uint8_t power;
    uint8_t rate;

    struct ldl_downstream_cmd cmd;
    enum ldl_mac_cmd_type next_cmd;

    enum adr_validation_states {

        _NO_ADR,
        _ADR_OK,
        _ADR_BAD
    };

    enum adr_validation_states adr_state;

    adr_state = _NO_ADR;

    /* these things can be rolled back */
    nbTrans = self->ctx.nbTrans;
    power = self->ctx.power;
    rate = self->ctx.rate;
    (void)memcpy(&chMask, self->ctx.chMask, sizeof(chMask));

    LDL_Stream_initReadOnly(&s_in, in, len);

    self->ctx.link_adr_ans.channelMaskOK = true;

    while(LDL_MAC_getDownCommand(&s_in, &cmd)){

        switch(cmd.type){
        default:
            /* not handling */
            LDL_DEBUG("not handling type %u", cmd.type)
            break;
#ifndef LDL_DISABLE_LINK_CHECK
        case LDL_CMD_LINK_CHECK:
        {
            union ldl_mac_response_arg arg;
            const struct ldl_link_check_ans *ans = &cmd.fields.linkCheck;

            arg.link_status.margin = ans->margin;
            arg.link_status.gwCount = ans->gwCount;

            LDL_DEBUG("link_check_ans: margin=%u gwCount=%u",
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

            LDL_DEBUG("link_adr_req: dataRate=%u txPower=%u chMask=%04x chMaskCntl=%u nbTrans=%u",
                req->dataRate, req->txPower, req->channelMask, req->channelMaskControl, req->nbTrans)

            uint8_t i;

            /* this is against the standard but we simply ignore any additional
             * blocks after the first one */
            if(!commandIsPending(self, LDL_CMD_LINK_ADR)){

                if(LDL_Region_isDynamic(self->ctx.region)){

                    switch(req->channelMaskControl){
                    case 0U:

                        /* mask/unmask channels 0..15 */
                        for(i=0U; i < U8(sizeof(req->channelMask))*8U; i++){

                            if((req->channelMask & (U16(1) << i)) > 0U){

                                (void)unmaskChannel(self->ctx.chMask, sizeof(self->ctx.chMask), self->ctx.region, i);
                            }
                            else{

                                (void)maskChannel(self->ctx.chMask, sizeof(self->ctx.chMask), self->ctx.region, i);
                            }
                        }
                        break;

                    case 6U:

                        unmaskAllChannels(self->ctx.chMask, sizeof(self->ctx.chMask));
                        break;

                    default:
                        self->ctx.link_adr_ans.channelMaskOK = false;
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

                                (void)unmaskChannel(self->ctx.chMask, sizeof(self->ctx.chMask), self->ctx.region, i);
                            }
                            else{

                                (void)maskChannel(self->ctx.chMask, sizeof(self->ctx.chMask), self->ctx.region, i);
                            }
                        }
                        break;

                    default:

                        for(i=0U; i < (sizeof(req->channelMask)*8U); i++){

                            if((req->channelMask & (U16(1) << i)) > 0U){

                                (void)unmaskChannel(self->ctx.chMask, sizeof(self->ctx.chMask), self->ctx.region, (req->channelMaskControl * 16U) + i);
                            }
                            else{

                                (void)maskChannel(self->ctx.chMask, sizeof(self->ctx.chMask), self->ctx.region, (req->channelMaskControl * 16U) + i);
                            }
                        }
                        break;
                    }
                }

                if(!LDL_MAC_peekNextCommand(&s_in, &next_cmd) || (next_cmd != LDL_CMD_LINK_ADR)){

                    self->ctx.link_adr_ans.dataRateOK = true;
                    self->ctx.link_adr_ans.powerOK = true;

                    /* nbTrans setting 0 means keep existing */
                    if(req->nbTrans > 0U){

                        self->ctx.nbTrans = req->nbTrans & 0xfU;

                        if(self->ctx.nbTrans > U8(LDL_REDUNDANCY_MAX)){

                            self->ctx.nbTrans = U8(LDL_REDUNDANCY_MAX);
                        }
                    }

                    /* ignore rate setting 15, because that means keep the current value */
                    if(req->dataRate < 0xfU){

                        // todo: need to pin out of range to maximum
                        if(rateSettingIsValid(self->ctx.region, req->dataRate)){

                            self->ctx.rate = req->dataRate;
                        }
                        else{

                            self->ctx.link_adr_ans.dataRateOK = false;
                        }
                    }

                    /* ignore power setting 15, because that means keep the current value */
                    if(req->txPower < 0xfU){

                        if(LDL_Region_validateTXPower(self->ctx.region, req->txPower)){

                            self->ctx.power = req->txPower;
                        }
                        else{

                            self->ctx.link_adr_ans.powerOK = false;
                        }
                    }

                    /* do not allow server to mask all channels */
                    if(allChannelsAreMasked(self->ctx.chMask, sizeof(self->ctx.chMask))){

                        LDL_INFO("server attempted to mask all channels")
                        self->ctx.link_adr_ans.channelMaskOK = false;
                    }

                    if(self->ctx.link_adr_ans.dataRateOK && self->ctx.link_adr_ans.powerOK && self->ctx.link_adr_ans.channelMaskOK){

                        adr_state = _ADR_OK;
                    }
                    else{

                        adr_state = _ADR_BAD;
                    }

                    setPendingCommand(self, LDL_CMD_LINK_ADR);
                }
            }
            else{

                /* ignoring mulitple link_adr_req contiguous blocks */
                LDL_DEBUG("ignoring multiple link_adr_req contiguous blocks")
            }
        }
            break;

        case LDL_CMD_DUTY_CYCLE:

            LDL_DEBUG("duty_cycle_req: %u",
                cmd.fields.dutyCycle.maxDutyCycle
            )

            self->ctx.maxDutyCycle = cmd.fields.dutyCycle.maxDutyCycle;

            setPendingCommand(self, LDL_CMD_DUTY_CYCLE);
            break;

        case LDL_CMD_RX_PARAM_SETUP:
        {
            const struct ldl_rx_param_setup_req *req = &cmd.fields.rxParamSetup;

            LDL_DEBUG("rx_param_setup_req: rx1DROffset=%u rx2DataRate=%u freq=%" PRIu32,
                req->rx1DROffset,
                req->rx2DataRate,
                req->freq
            )

            // todo: validation

            self->ctx.rx1DROffset = req->rx1DROffset;
            self->ctx.rx2DataRate = req->rx2DataRate;
            self->ctx.rx2Freq = req->freq;

            self->ctx.rx_param_setup_ans.rx1DROffsetOK = true;
            self->ctx.rx_param_setup_ans.rx2DataRateOK = true;
            self->ctx.rx_param_setup_ans.channelOK = true;

            setPendingCommand(self, LDL_CMD_RX_PARAM_SETUP);
        }
            break;

        case LDL_CMD_DEV_STATUS:
        {
            LDL_DEBUG("dev_status_req")

            self->ctx.dev_status_ans.battery = self->get_battery_level(self->app);

            if(self->rx_snr > 31){

                self->ctx.dev_status_ans.margin = 31;
            }
            else if(self->rx_snr < -32){

                self->ctx.dev_status_ans.margin = -32;
            }
            else{

                self->ctx.dev_status_ans.margin = (int8_t)self->rx_snr;
            }

            setPendingCommand(self, LDL_CMD_DEV_STATUS);
        }
            break;

        case LDL_CMD_NEW_CHANNEL:

            LDL_DEBUG("new_channel_req: chIndex=%u freq=%" PRIu32 " maxDR=%u minDR=%u",
                cmd.fields.newChannel.chIndex,
                cmd.fields.newChannel.freq,
                cmd.fields.newChannel.maxDR,
                cmd.fields.newChannel.minDR
            )

            if(LDL_Region_isDynamic(self->ctx.region)){

                self->ctx.new_channel_ans.dataRateRangeOK = LDL_Region_validateRate(self->ctx.region, cmd.fields.newChannel.chIndex, cmd.fields.newChannel.minDR, cmd.fields.newChannel.maxDR);
                self->ctx.new_channel_ans.channelFreqOK = LDL_Region_validateFreq(self->ctx.region, cmd.fields.newChannel.freq);

                // todo: check if modifying default channel

                if(self->ctx.new_channel_ans.dataRateRangeOK && self->ctx.new_channel_ans.channelFreqOK){

                    (void)setChannel(self, cmd.fields.newChannel.chIndex, cmd.fields.newChannel.freq, cmd.fields.newChannel.minDR, cmd.fields.newChannel.maxDR);
                }

                setPendingCommand(self, LDL_CMD_NEW_CHANNEL);
            }
            else{

                /* not processed */
                LDL_DEBUG("new_channel_req not processed in this region")
            }
            break;

        case LDL_CMD_DL_CHANNEL:

            LDL_DEBUG("dl_channel_req: chIndex=%u freq=%" PRIu32,
                cmd.fields.dlChannel.chIndex,
                cmd.fields.dlChannel.freq
            )

            if(LDL_Region_isDynamic(self->ctx.region)){

                self->ctx.dl_channel_ans.uplinkFreqOK = true;
                self->ctx.dl_channel_ans.channelFreqOK = LDL_Region_validateFreq(self->ctx.region, cmd.fields.dlChannel.freq);
                setPendingCommand(self, LDL_CMD_DL_CHANNEL);
            }
            else{

                /* not processed */
                LDL_DEBUG("dl_channel_req not processed in this region")
            }
            break;

        case LDL_CMD_RX_TIMING_SETUP:

            LDL_DEBUG("rx_timing_setup_req: delay=%u",
                cmd.fields.rxTimingSetup.delay
            )

            self->ctx.rx1Delay = cmd.fields.rxTimingSetup.delay;

            setPendingCommand(self, LDL_CMD_RX_TIMING_SETUP);
            break;

#ifndef LDL_DISABLE_TX_PARAM_SETUP
        case LDL_CMD_TX_PARAM_SETUP:

            LDL_DEBUG("tx_param_setup_req: downlinkDwellTime=%s uplinkDwellTime=%s maxEIRP=%u",
                ((cmd.fields.txParamSetup & 0x20U) > 0U) ? "true" : "false",
                ((cmd.fields.txParamSetup & 0x10U) > 0U) ? "true" : "false",
                cmd.fields.txParamSetup & 0xfU
            )

            if(LDL_Region_txParamSetupImplemented(self->ctx.region)){

                self->ctx.tx_param_setup = cmd.fields.txParamSetup;

                setPendingCommand(self, LDL_CMD_TX_PARAM_SETUP);
            }
            else{

                /* not processed */
                LDL_DEBUG("tx_param_setup_req not processed in this region")
            }
            break;
#endif
#ifndef LDL_DISABLE_DEVICE_TIME
        case LDL_CMD_DEVICE_TIME:
        {
            union ldl_mac_response_arg arg;
            uint32_t lag;

            arg.device_time.time = U64(cmd.fields.deviceTime.seconds);
            arg.device_time.time <<= 8;
            arg.device_time.time |= U64(cmd.fields.deviceTime.fractions);

            lag = timerDelta(self->ticks_at_tx, self->ticks(self->app));

            arg.device_time.time += (U64(lag) * U64(timeTPS) / U64(GET_TPS()));

            arg.device_time.seconds = U32(arg.device_time.time >> 8);
            arg.device_time.fractions = U8(arg.device_time.time);

            LDL_DEBUG("device_time_ans: seconds=%" PRIu32 " fractions=%u",
                arg.device_time.seconds,
                arg.device_time.fractions
            )

            self->handler(self->app, LDL_MAC_DEVICE_TIME, &arg);
        }
            break;
#endif
#if defined(LDL_ENABLE_L2_1_1)
        case LDL_CMD_ADR_PARAM_SETUP:

            LDL_DEBUG("adr_param_setup: limit_exp=%u delay_exp=%u",
                cmd.fields.adrParamSetup.limit_exp,
                cmd.fields.adrParamSetup.delay_exp
            )

            self->ctx.adr_ack_limit = U16(1) << cmd.fields.adrParamSetup.limit_exp;
            self->ctx.adr_ack_delay = U16(1) << cmd.fields.adrParamSetup.delay_exp;

            setPendingCommand(self, LDL_CMD_ADR_PARAM_SETUP);
            break;

        case LDL_CMD_REKEY:

            LDL_DEBUG("rekey_conf: version=%u", cmd.fields.rekey.version)

            /* The server version must be greater than 0 (0 is not allowed), and smaller or equal (<=) to the
             * device’s LoRaWAN version. Therefore for a LoRaWAN1.1 device the only valid value is 1. If
             * the server’s version is invalid the device SHALL discard the RekeyConf command and
             * retransmit the RekeyInd in the next uplink frame */
            if(cmd.fields.rekey.version == self->ctx.version){

                clearPendingCommand(self, LDL_CMD_REKEY);
            }
            break;

        case LDL_CMD_FORCE_REJOIN:

            LDL_DEBUG("force_rejoin_req: max_retries=%u rejoin_type=%u period=% dr=%u",
                cmd.fields.forceRejoin.max_retries,
                cmd.fields.forceRejoin.rejoin_type,
                cmd.fields.forceRejoin.period,
                cmd.fields.forceRejoin.dr
            )

            LDL_DEBUG("force rejoin not implemented")

            /* no reply - you are meant to start doing a rejoin */

            break;

        case LDL_CMD_REJOIN_PARAM_SETUP:

            LDL_DEBUG("rejoin_param_setup_req: maxTimeN=%u maxCountN=%u",
                cmd.fields.rejoinParamSetup.maxTimeN,
                cmd.fields.rejoinParamSetup.maxCountN
            )

            self->ctx.rejoin_param_setup_ans.timeOK = false;
            setPendingCommand(self, LDL_CMD_REJOIN_PARAM_SETUP);
            break;
#endif
        }
    }

    /* roll back ADR request if not successful */
    if(adr_state == _ADR_BAD){

        LDL_DEBUG("bad ADR setting; rollback")

        (void)memcpy(self->ctx.chMask, chMask, sizeof(self->ctx.chMask));

        self->ctx.rate = rate;
        self->ctx.power = power;
        self->ctx.nbTrans = nbTrans;
    }
}

static void registerTime(struct ldl_mac *self, const struct ldl_mac_tx *tx)
{
    uint8_t band;
    uint32_t offTime;

    if(LDL_Region_getBand(self->ctx.region, tx->freq, &band)){

        LDL_PEDANTIC( band < LDL_BAND_MAX )

        offTime = tx->airTime * LDL_Region_getOffTimeFactor(self->ctx.region, band);

        if((self->band[band] + offTime) < self->band[band]){

            self->band[band] = UINT32_MAX;
        }
        else{

            self->band[band] += offTime;
        }
    }

    if((self->op == LDL_OP_JOINING) || (self->ctx.maxDutyCycle > 0U)){

        if(self->op == LDL_OP_JOINING){

            offTime = tx->airTime * getOTAAOffTime(self);
        }
        else{

            offTime = tx->airTime * (U32(1) << (self->ctx.maxDutyCycle & 0xfU));
        }

        if((self->band[LDL_BAND_GLOBAL] + offTime) < self->band[LDL_BAND_GLOBAL]){

            self->band[LDL_BAND_GLOBAL] = UINT32_MAX;
        }
        else{

            self->band[LDL_BAND_GLOBAL] += offTime;
        }
    }
}

static uint8_t requiredRate(uint8_t desired, uint8_t min, uint8_t max)
{
    uint8_t retval;

    if(desired < min){

        retval = min;
    }
    else if(desired > max){

        retval = max;
    }
    else{

        retval = desired;
    }

    return retval;
}

#ifndef LDL_DISABLE_TX_PARAM_SETUP
static bool uplinkDwell(uint8_t tx_param_setup)
{
    return ((tx_param_setup & 0x10U) > 0U);
}
#endif

static void selectJoinChannelAndRate(struct ldl_mac *self, struct ldl_mac_tx *tx)
{
    uint8_t minRate;
    uint8_t maxRate;
    bool retval;
    uint8_t desired_rate;

    desired_rate = LDL_Region_getJoinRate(self->ctx.region, self->trials);
#ifndef LDL_DISABLE_TX_PARAM_SETUP
    desired_rate = LDL_Region_applyUplinkDwell(self->ctx.region, uplinkDwell(self->ctx.tx_param_setup), desired_rate);
#endif

    /* dynamic regions join on default channels so select as per usual */
    if(LDL_Region_isDynamic(self->ctx.region)){

        retval = selectChannel(self, desired_rate, timeUntilNextChannel(self), tx);
    }
    else{

        minRate = 0;
        maxRate = 0;

        tx->chIndex = LDL_Region_getJoinIndex(self->ctx.region, self->trials, self->rand(self->app));

        retval = LDL_Region_getChannel(self->ctx.region, tx->chIndex, &tx->freq, &minRate, &maxRate);

        tx->rate = requiredRate(desired_rate, minRate, maxRate);
    }

    (void)retval;

    /* join channels should always be available */
    LDL_ASSERT(retval);
}

static bool selectChannel(const struct ldl_mac *self, uint8_t desired_rate, uint32_t limit, struct ldl_mac_tx *tx)
{
    bool retval = false;
    uint8_t i;
    uint8_t selection;
    uint32_t available = 0;
    uint8_t j = 0;
    uint8_t minRate;
    uint8_t maxRate;
    uint8_t except = UINT8_MAX;

    uint8_t mask[sizeof(self->ctx.chMask)];

    (void)memset(mask, 0, sizeof(mask));

    /* count number of available channels for this rate */
    for(i=0; i < LDL_Region_numChannels(self->ctx.region); i++){

        if(isAvailable(self, i, limit)){

            if(i == self->tx.chIndex){

                except = i;
            }

            (void)maskChannel(mask, sizeof(mask), self->ctx.region, i);
            available++;
        }
    }

    if(available > 0U){

        if(except != U8(UINT8_MAX)){

            if(available == 1U){

                except = UINT8_MAX;
            }
            else{

                available--;
            }
        }

        selection = U8(self->rand(self->app) % available);

        for(i=0; i < LDL_Region_numChannels(self->ctx.region); i++){

            if(channelIsMasked(mask, sizeof(mask), self->ctx.region, i)){

                if(except != i){

                    if(selection == j){

                        if(getChannel(self, i, &tx->freq, &minRate, &maxRate)){

                            tx->chIndex = i;

                            tx->rate = requiredRate(desired_rate, minRate, maxRate);

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

static bool isAvailable(const struct ldl_mac *self, uint8_t chIndex, uint32_t limit)
{
    bool retval = false;
    uint32_t freq;
    uint8_t minRate;
    uint8_t maxRate;
    uint8_t band;

    if(!channelIsMasked(self->ctx.chMask, sizeof(self->ctx.chMask), self->ctx.region, chIndex)){

        if(getChannel(self, chIndex, &freq, &minRate, &maxRate)){

            if(freq > 0U){

                if(LDL_Region_getBand(self->ctx.region, freq, &band)){

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

static uint32_t timeUntilAvailable(const struct ldl_mac *self, uint8_t chIndex)
{
    uint32_t retval = UINT32_MAX;
    uint32_t freq;
    uint8_t minRate;
    uint8_t maxRate;
    uint8_t band;

    if(!channelIsMasked(self->ctx.chMask, sizeof(self->ctx.chMask), self->ctx.region, chIndex)){

        if(getChannel(self, chIndex, &freq, &minRate, &maxRate)){

            if(freq > 0U){

                if(LDL_Region_getBand(self->ctx.region, freq, &band)){

                    LDL_PEDANTIC( band < LDL_BAND_MAX )

                    retval = (self->band[band] > self->band[LDL_BAND_GLOBAL]) ? self->band[band] : self->band[LDL_BAND_GLOBAL];
                }
            }
        }
    }

    return retval;
}

static void initSession(struct ldl_mac *self, enum ldl_region region)
{
    forgetNetwork(self);

    self->ctx.region = region;
    self->ctx.rate = MIN_RATE;
    self->ctx.power = 0U;
    self->ctx.adr = true;
}

static void forgetNetwork(struct ldl_mac *self)
{
    /* the following items must not be cleared */
    enum ldl_region region = self->ctx.region;
    uint8_t rate = self->ctx.rate;
    uint8_t power = self->ctx.power;
    bool adr = self->ctx.adr;

    self->fPending = false;
    self->pendingACK = false;

    (void)memset(&self->ctx, 0, sizeof(self->ctx));

    /* restore the essential fields */
    self->ctx.region = region;
    self->ctx.rate = rate;
    self->ctx.power = power;
    self->ctx.adr = adr;

    /* init non-zero fields */
    self->ctx.magic = sessionMagicNumber;
    self->ctx.rx1DROffset = LDL_Region_getRX1Offset(region);
    self->ctx.rx1Delay = LDL_Region_getRX1Delay(region);
    self->ctx.rx2DataRate = LDL_Region_getRX2Rate(region);
    self->ctx.rx2Freq = LDL_Region_getRX2Freq(region);
    self->ctx.adr_ack_limit = ADRAckLimit;
    self->ctx.adr_ack_delay = ADRAckDelay;
#ifndef LDL_DISABLE_TX_PARAM_SETUP
    self->ctx.tx_param_setup = 0xff;
#endif

    /* locally set fields */
    self->ctx.maxDutyCycle = self->maxDutyCycle;

    /* reset the default channels (even though they shouldn't have changed!) */
    LDL_Region_getDefaultChannels(self->ctx.region, self);
}

static bool getChannel(const struct ldl_mac *self, uint8_t chIndex, uint32_t *freq, uint8_t *minRate, uint8_t *maxRate)
{
    bool retval;
    const struct ldl_mac_channel *chConfig;

    retval = false;

    if(LDL_Region_isDynamic(self->ctx.region)){

        if(chIndex < LDL_Region_numChannels(self->ctx.region)){

            if(chIndex < sizeof(self->ctx.chConfig)/sizeof(*self->ctx.chConfig)){

                chConfig = &self->ctx.chConfig[chIndex];

                *freq = (chConfig->freqAndRate >> 8) * U32(100);
                *minRate = U8((chConfig->freqAndRate >> 4) & 0xfU);
                *maxRate = U8(chConfig->freqAndRate & 0xfU);

                retval = true;
            }
        }
    }
    else{

        retval = LDL_Region_getChannel(self->ctx.region, chIndex, freq, minRate, maxRate);
    }

    return retval;
}

static bool setChannel(struct ldl_mac *self, uint8_t chIndex, uint32_t freq, uint8_t minRate, uint8_t maxRate)
{
    bool retval;

    retval = false;

    if(chIndex < LDL_Region_numChannels(self->ctx.region)){

        if(chIndex < sizeof(self->ctx.chConfig)/sizeof(*self->ctx.chConfig)){

            if(freq == 0U){

                self->ctx.chConfig[chIndex].freqAndRate = 0U;
                retval = true;
            }
            else if(LDL_Region_validateFreq(self->ctx.region, freq)){

                self->ctx.chConfig[chIndex].freqAndRate = ((freq/U32(100)) << 8) | (U32(minRate) << 4) | (U32(maxRate) & 0xfU);
                retval = true;
            }
            else{

                /* not allowed */
                LDL_ERROR("channel %" PRIu32 "Hz not allowed in this region", freq)
            }
        }
    }

    return retval;
}

static bool maskChannel(uint8_t *mask, size_t max, enum ldl_region region, uint8_t chIndex)
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

static bool unmaskChannel(uint8_t *mask, size_t max, enum ldl_region region, uint8_t chIndex)
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

static void unmaskAllChannels(uint8_t *mask, size_t max)
{
    (void)memset(mask, 0, max);
}

static bool channelIsMasked(const uint8_t *mask, size_t max, enum ldl_region region, uint8_t chIndex)
{
    bool retval = false;

    if(chIndex < LDL_Region_numChannels(region)){

        if(chIndex < (max*8U)){

            retval = ((mask[chIndex / 8U] & (1U << (chIndex % 8U))) > 0U);
        }
    }

    return retval;
}

static bool allChannelsAreMasked(const uint8_t *mask, size_t max)
{
    bool retval = true;
    uint8_t i;

    for(i=0; i < max; i++){

        if(mask[i] != 0xffU){

            retval = false;
            break;
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

static uint32_t getOTAAOffTime(const struct ldl_mac *self)
{
    uint32_t retval;

    /* first hour: 36/3600 (0.01) */
    if(self->day > (timeTPS * U32(23) * U32(60) * U32(60))){

        retval = 100;
    }
    /* first 11 hours: 36/36000 (0.001) */
    else if(self->day > (timeTPS * U32(13) * U32(60) * U32(60))){

        retval = 1000;
    }
    /* 8.7/86400 (0.0001) */
    else{

        retval = 10000;
    }

    LDL_DEBUG("offtime=%" PRIu32 "", retval)

    return retval;
}

static uint32_t timerDelta(uint32_t timeout, uint32_t time)
{
    return (timeout <= time) ? (time - timeout) : (UINT32_MAX - timeout + time);
}

static bool updateDownCounter(uint32_t *counter, uint32_t time)
{
    bool expired = false;

    if(*counter > 0U){

        if(*counter <= time){

            *counter = 0U;
            expired = true;
        }
        else{

            *counter -= time;
        }
    }

    return expired;
}

static bool processBands(struct ldl_mac *self)
{
    bool retval = false;
    size_t i;
    uint32_t time = 0U;

    if(!self->time.parked){

        /* time tracking via ticks */
        {
            uint32_t ticks = self->ticks(self->app);

            uint32_t since = timerDelta(self->time.ticks, ticks);

            self->time.ticks = ticks;

            uint32_t fraction = ((since % GET_TPS()) * timeTPS) + self->time.remainder;

            time = ((since / GET_TPS()) * timeTPS) + (fraction / GET_TPS());

            self->time.remainder = fraction % GET_TPS();
        }

        bool ready = false;

        /* handle the day counter if set */
        (void)updateDownCounter(&self->day, time);

        /* decrement down counters with time */
        for(i=0U; i < sizeof(self->band)/sizeof(*self->band); i++){

            if(updateDownCounter(&self->band[i], time)){

                ready = true;
            }
        }

        if(ready && (self->band[LDL_BAND_GLOBAL] == 0U)){

            LDL_DEBUG("channel is ready")
            self->handler(self->app, LDL_MAC_CHANNEL_READY, NULL);
            retval = true;
        }
    }

    return retval;
}

static void setNextBandEvent(struct ldl_mac *self)
{
    uint32_t time = UINT32_MAX;
    uint32_t ticks;
    size_t i;

    for(i=0; i < sizeof(self->band)/sizeof(*self->band); i++){

        if(self->band[i] > 0U){

            if(self->band[i] < time){

                time = self->band[i];
            }
        }
    }

    if(time <= self->band[LDL_BAND_GLOBAL]){

        time = self->band[LDL_BAND_GLOBAL];
    }

    /* consider day counter if there are no
     * band counters */
    if((time == UINT32_MAX) && (self->day > 0U)){

        time = self->day;
    }

    /* convert to ticks */
    if(time < UINT32_MAX){

        self->time.parked = false;

        if(time < (U32(INT32_MAX)/GET_TPS()*timeTPS)){

            ticks = ((time / timeTPS) + (((time % timeTPS) > 0U) ? U32(1) : U32(0))) * GET_TPS();
        }
        else{

            ticks = U32(INT32_MAX)/GET_TPS()*U32(INT32_MAX);
        }

        LDL_MAC_timerSet(self, LDL_TIMER_BAND, ticks);
    }
    else{

        //LDL_DEBUG("time tracking is disabled")
        self->time.parked = true;
        LDL_MAC_timerClear(self, LDL_TIMER_BAND);
    }
}

static void downlinkMissingHandler(struct ldl_mac *self)
{
    uint8_t nbTrans;
    union ldl_mac_response_arg arg;

    if(self->opts.nbTrans > 0U){

        nbTrans = self->opts.nbTrans;
    }
    else{

        nbTrans = (self->ctx.nbTrans > 0U) ? self->ctx.nbTrans : 1U;
    }

    self->trials++;

    switch(self->op){
    default:
    case LDL_OP_NONE:
        /* do nothing */
        break;

    case LDL_OP_DATA_UNCONFIRMED:
    case LDL_OP_DATA_CONFIRMED:
    {
        struct ldl_mac_tx tx;

        bool global_band_ok = (self->band[LDL_BAND_GLOBAL] < LDL_Region_getMaxDCycleOffLimit(self->ctx.region));
        bool channel_ok = selectChannel(self, self->tx.rate, LDL_Region_getMaxDCycleOffLimit(self->ctx.region), &tx);

        if((self->trials < nbTrans) && global_band_ok && channel_ok){

            LDL_OPS_micDataFrame(self, self->buffer, self->bufferLen);

            if(self->op == LDL_OP_DATA_CONFIRMED){

                /* double back-off with each trial */
                LDL_MAC_timerSet(self, LDL_TIMER_WAITA, (GET_TPS() << self->trials));
            }
            else{

                LDL_MAC_timerSet(self, LDL_TIMER_WAITA, 0U);
            }

            self->state = LDL_STATE_WAIT_TX;
        }
        else{

            if(adaptRate(self)){

                pushSessionUpdate(self);
            }

            self->handler(self->app, (self->op == LDL_OP_DATA_CONFIRMED) ? LDL_MAC_DATA_TIMEOUT : LDL_MAC_DATA_COMPLETE, NULL);

            self->state = LDL_STATE_IDLE;
            self->op = LDL_OP_NONE;
        }
    }
        break;

    case LDL_OP_JOINING:

        /* OTAA needs to return an error if we run out of DevNonce */
        if(self->devNonce <= U32(UINT16_MAX)){

            fillJoinBuffer(self, U16(self->devNonce));

            self->devNonce++;

            arg.dev_nonce_updated.nextDevNonce = self->devNonce;
            self->handler(self->app, LDL_MAC_DEV_NONCE_UPDATED, &arg);

            LDL_DEBUG("waiting to retry OTAA")

            self->state = LDL_STATE_WAIT_OTAA;
            LDL_MAC_timerSet(self, LDL_TIMER_WAITA, 0);
        }
        else{

            self->handler(self->app, LDL_MAC_JOIN_EXHAUSTED, NULL);

            self->state = LDL_STATE_IDLE;
            self->op = LDL_OP_NONE;
        }
        break;
    }
}

static uint32_t msToTime(uint32_t ms)
{
    /* round up */
    uint32_t t = ms * timeTPS;

    return (t / U32(1000)) + (((t % U32(1000)) > 0U) ? U32(1) : U32(0));
}

static uint32_t msToTicks(const struct ldl_mac *self, uint32_t ms)
{
    /* round up */
    uint32_t t = ms * GET_TPS();

    return (t / U32(1000)) + (((t % U32(1000)) > 0U) ? U32(1) : U32(0));
}

static uint32_t timeUntilNextChannel(const struct ldl_mac *self)
{
    uint8_t i;
    uint32_t min = UINT32_MAX;

    for(i=0; i < LDL_Region_numChannels(self->ctx.region); i++){

        if(min > timeUntilAvailable(self, i)){

            min = timeUntilAvailable(self, i);
        }
    }

    return min;
}

static void pushSessionUpdate(struct ldl_mac *self)
{
    union ldl_mac_response_arg arg;

    arg.session_updated.session = &self->ctx;

    self->handler(self->app, LDL_MAC_SESSION_UPDATED, &arg);

    debugSession(self);
}

static void debugSession(struct ldl_mac *self)
{
#ifndef LDL_TRACE_DISABLED
    size_t i;

    LDL_TRACE("magic=%u", self->ctx.magic)
    LDL_TRACE("joined=%s", self->ctx.joined ? "true" : "false")
    LDL_TRACE("adr=%s", self->ctx.adr ? "true" : "false")
    LDL_TRACE("version=%u", SESS_VERSION(self->ctx))

    LDL_TRACE("region=%s", LDL_Region_enumToString(self->ctx.region))

    LDL_TRACE("up=%" PRIu32 "", self->ctx.up)
    LDL_TRACE("appDown=%" PRIu16 "", self->ctx.appDown)
    LDL_TRACE("nwkDown=%" PRIu16 "", self->ctx.nwkDown)

    LDL_TRACE("devAddr=%" PRIu32 "", self->ctx.devAddr)
    LDL_TRACE("netID=%" PRIu32 "", self->ctx.netID)

    for(i=0U; i < sizeof(self->ctx.chConfig)/sizeof(*self->ctx.chConfig); i++){

        LDL_TRACE("chConfig: chIndex=%u freq=%" PRIu32 " minRate=%u maxRate=%u dlFreq=%" PRIu32 "",
            U8(i),
            (self->ctx.chConfig[i].freqAndRate >> 8) * U32(100),
            U8((self->ctx.chConfig[i].freqAndRate >> 4) & 0xfU),
            U8(self->ctx.chConfig[i].freqAndRate & 0xfU),
            self->ctx.chConfig[i].dlFreq
        )
    }

    LDL_TRACE_BEGIN()
    LDL_TRACE_PART("chMask=")
    LDL_TRACE_BIT_STRING(self->ctx.chMask, sizeof(self->ctx.chMask))
    LDL_TRACE_FINAL()

    LDL_TRACE("rate=%u", self->ctx.rate)
    LDL_TRACE("power=%u", self->ctx.power)
    LDL_TRACE("maxDutyCycle=%u", self->ctx.maxDutyCycle)
    LDL_TRACE("nbTrans=%u", self->ctx.nbTrans)
    LDL_TRACE("rx1DROffset=%u", self->ctx.rx1DROffset)
    LDL_TRACE("rx1Delay=%u", self->ctx.rx1Delay)
    LDL_TRACE("rx2DataRate=%u", self->ctx.rx2DataRate)

    LDL_TRACE("rx2Freq=%" PRIu32 "", self->ctx.rx2Freq)

    LDL_TRACE("adr_ack_limit=%u", self->ctx.adr_ack_limit)
    LDL_TRACE("adr_ack_delay=%u", self->ctx.adr_ack_delay)

    LDL_TRACE("joinNonce=%" PRIu32 "", self->ctx.joinNonce)
    LDL_TRACE("devNonce=%" PRIu16 "", self->ctx.devNonce)
    LDL_TRACE("pending_cmds=0x%02X", self->ctx.pending_cmds)
#endif
}

static void dummyResponseHandler(void *app, enum ldl_mac_response_type type, const union ldl_mac_response_arg *arg)
{
    (void)app;
    (void)type;
    (void)arg;
}

static bool commandIsPending(const struct ldl_mac *self, enum ldl_mac_cmd_type type)
{
    return ((self->ctx.pending_cmds & (U16(1) << type)) > 0U);
}

static void clearPendingCommand(struct ldl_mac *self, enum ldl_mac_cmd_type type)
{
    self->ctx.pending_cmds &= ~(U16(1) << type);
}

static void setPendingCommand(struct ldl_mac *self, enum ldl_mac_cmd_type type)
{
    self->ctx.pending_cmds |= (U16(1) << type);
}

static uint32_t defaultRand(void *app)
{
    (void)app;

    /* I assure you that this is random */
    return 42U;
}

static uint8_t defaultBatteryLevel(void *app)
{
    (void)app;

    return 255U;
}

static void inputSignal(struct ldl_mac *self, uint32_t ticks)
{
    LDL_SYSTEM_ENTER_CRITICAL(self->app)

    if(!self->inputs.state && self->inputs.armed){

        self->inputs.time = ticks;
        self->inputs.state = true;
    }

    LDL_SYSTEM_LEAVE_CRITICAL(self->app)
}

static void inputArm(struct ldl_mac *self)
{
    LDL_SYSTEM_ENTER_CRITICAL(self->app)

    self->inputs.state = false;
    self->inputs.armed = true;

    LDL_SYSTEM_LEAVE_CRITICAL(self->app)
}

static void inputDisarm(struct ldl_mac *self)
{
    LDL_SYSTEM_ENTER_CRITICAL(self->app)

    self->inputs.armed = false;

    LDL_SYSTEM_LEAVE_CRITICAL(self->app)
}

static bool inputCheck(struct ldl_mac *self, uint32_t *lag)
{
    bool retval = false;

    LDL_SYSTEM_ENTER_CRITICAL(self->app)

    if(self->inputs.state){

        self->inputs.armed = false;
        self->inputs.state = false;
        *lag = timerDelta(self->inputs.time, self->ticks(self->app));

        retval = true;
    }

    LDL_SYSTEM_LEAVE_CRITICAL(self->app)

    return retval;
}

static bool inputPending(const struct ldl_mac *self)
{
    return self->inputs.state;
}


static void fillJoinBuffer(struct ldl_mac *self, uint16_t devNonce)
{
    struct ldl_frame_join_request f;

    f.joinEUI = self->joinEUI;
    f.devEUI = self->devEUI;

#if defined(LDL_ENABLE_L2_1_0_3)
    (void)devNonce;
    self->ctx.devNonce = self->rand(self->app);
#else
    self->ctx.devNonce = devNonce;
#endif
    f.devNonce = self->ctx.devNonce;

    self->bufferLen = LDL_OPS_prepareJoinRequest(self, &f, self->buffer, U8(sizeof(self->buffer)));

    selectJoinChannelAndRate(self, &self->tx);
}

