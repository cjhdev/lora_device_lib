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

#ifndef LORA_MAC_H
#define LORA_MAC_H

/** @file */

/**
 * @defgroup ldl LDL
 * 
 * Below is a basic example of how to use LDL to join and then send 
 * data:
 * 
 * @include examples/doxygen/example.c
 * 
 * */

/**
 * @defgroup ldl_mac MAC
 * @ingroup ldl
 * 
 * @{
 * */

#ifdef __cplusplus
extern "C" {
#endif

#include "lora_platform.h"
#include "lora_region.h"
#include "lora_radio.h"
#include "lora_event.h"
#include "lora_stream.h"

#include <stdint.h>
#include <stdbool.h>

#ifndef LORA_MAX_PACKET
    /** @ingroup ldl_optional
     * 
     * Redefine this to reduce the stack and data memory footprint.
     * 
     * **Example (defined in makefile):**
     * 
     * @code
     * -DLORA_MAX_PACKET=64U
     * @endcode
     * 
     * */
    #define LORA_MAX_PACKET UINT8_MAX
    
#endif
 
#ifndef LORA_DEFAULT_RATE
    /** @ingroup ldl_optional
     * 
     * - transmit rate is set to this value upon reset
     * - if ADR is enabled, the stack will never drop back below this value
     * 
     * example:
     * 
     * @code
     * 
     * -DLORA_DEFAULT_RATE=0U
     * 
     * @endcode
     * 
     * This would ensure the stack defaults to using the largest spreading
     * factor.
     * 
     * */
    #define LORA_DEFAULT_RATE 2U

#endif
 
struct lora_mac;

/** events pushed to application (some with data arguments)
 * 
 * @see lora_mac_response_arg
 * 
 *  */
enum lora_mac_response_type {
        
    LORA_MAC_CHIP_ERROR,    /**< Chip didn't respond as expected */
    LORA_MAC_RESET,         /**< Chip is being reset; wait for #LORA_MAC_STARTUP */
    LORA_MAC_STARTUP,       /**< MAC has started and is ready for commands */
    
    LORA_MAC_JOIN_COMPLETE, /**< join request was successful */
    LORA_MAC_JOIN_TIMEOUT,  /**< join request timed out */        
    LORA_MAC_DATA_COMPLETE, /**< confirmed/unconfirmed data request completed successfully */
    LORA_MAC_DATA_TIMEOUT,  /**< confirmed data request has timed out */
    LORA_MAC_DATA_NAK,      /**< confirmed data request was not acknowledged (i.e. a downlink was received but without ACK) */
    
    LORA_MAC_RX,            /**< data received */    
    
    LORA_MAC_LINK_STATUS,   /**< link status answer */
    
    LORA_MAC_RX1_SLOT,      /**< RX1 slot information */    
    LORA_MAC_RX2_SLOT,      /**< RX2 slot information */ 
    
    LORA_MAC_DOWNSTREAM,    /**< downstream message received */
        
    LORA_MAC_TX_COMPLETE,   /**< transmit complete */             
    LORA_MAC_TX_BEGIN,      /**< transmit begin */
};

/** event data arguments pushed to application
 * 
 * @see lora_mac_response_type
 * 
 *  */
union lora_mac_response_arg {

    /** #LORA_MAC_JOIN_TIMEOUT argument */
    struct {
        
        uint32_t retry_ms;  /**< join will retry in this many ms */
        
    } join_timeout;
    
    /** #LORA_MAC_DOWNSTREAM argument */
    struct {
        
        int16_t rssi;
        int16_t snr;
        uint8_t size;   /**< size of downstream message */
        
    } downstream;
    
    /** #LORA_MAC_RX argument */
    struct {
        
        const uint8_t *data;
        uint16_t counter;        
        uint8_t port;           
        uint8_t size;   /**< size of downstream payload */
        
    } rx;     
    
    /** #LORA_MAC_LINK_STATUS argument */
    struct {
        
        bool inFOpt;        /**< carried in Fopt field */
        int8_t margin;                         
        uint8_t gwCount;    /**< received by this many gateways */
        
    } link_status;
    
    /** #LORA_MAC_RX1_SLOT and #LORA_MAC_RX2_SLOT argument */
    struct {
    
        uint32_t margin;                /**< allowed error margin */
        uint32_t error;                 /**< ticks passed since scheduled event */
        uint32_t freq;                  /**< frequency */
        enum lora_signal_bandwidth bw;  /**< bandwidth */
        enum lora_spreading_factor sf;  /**< spreading factor */
        uint8_t timeout;                /**< symbol timeout */
        
    } rx_slot;    
    
    /** #LORA_MAC_TX_BEGIN argument */
    struct {
        
        uint32_t freq;                      /**< frequency */    
        enum lora_spreading_factor sf;      /**< spreading factor */
        enum lora_signal_bandwidth bw;      /**< bandwidth */
        uint8_t power;
        uint8_t size;                       /**< message size */
        
    } tx_begin;
    
    /** #LORA_MAC_STARTUP argument */
    struct {
        
        unsigned int entropy;       /**< srand seed */
        
    } startup;
};

/** MAC calls this function to notify application of events
 * 
 * @param[in] app   the pointer passed in LDL_MAC_init()
 * @param[in] type  
 * @param[in] arg   **OPTIONAL** depending on #lora_mac_response_type
 * 
 * */
typedef void (*lora_mac_response_fn)(void *app, enum lora_mac_response_type type, const union lora_mac_response_arg *arg);

/** MAC state */
enum lora_mac_state {

    LORA_STATE_INIT,            /**< stack has been memset to zero */

    LORA_STATE_INIT_RESET,      /**< holding reset after _INIT */
    LORA_STATE_INIT_LOCKOUT,    /**< waiting after _RESET */
    
    LORA_STATE_RECOVERY_RESET,   /**< holding reset after chip error */
    LORA_STATE_RECOVERY_LOCKOUT, /**< waiting after chip error reset */

    LORA_STATE_ENTROPY,         /**< sample entropy */

    LORA_STATE_IDLE,        /**< ready for operations */
    
    LORA_STATE_WAIT_TX,     /**< waiting for channel to become available */
    LORA_STATE_TX,          /**< radio is TX */
    LORA_STATE_WAIT_RX1,    /**< waiting for first RX window */
    LORA_STATE_RX1,         /**< first RX window */
    LORA_STATE_WAIT_RX2,    /**< waiting for second RX window */
    LORA_STATE_RX2,         /**< second RX window */    
    
    LORA_STATE_WAIT_RETRY,  /**< wait to retransmit / retry */
    
};

/** MAC operations */
enum lora_mac_operation {
  
    LORA_OP_NONE,                   /**< no active operation */
    LORA_OP_JOINING,                /**< MAC is performing a join */
    LORA_OP_DATA_UNCONFIRMED,       /**< MAC is sending unconfirmed data */
    LORA_OP_DATA_CONFIRMED,         /**< MAC is sending confirmed data */    
    LORA_OP_RESET,                  /**< MAC is performing radio reset */
};

/** MAC error modes */
enum lora_mac_errno {
    
    LORA_ERRNO_NONE,
    LORA_ERRNO_NOCHANNEL,   /**< upstream channel not available */
    LORA_ERRNO_SIZE,        /**< message too large to send */
    LORA_ERRNO_RATE,        /**< data rate setting not valid for region */
    LORA_ERRNO_PORT,        /**< port not valid for upstream message */
    LORA_ERRNO_BUSY,        /**< stack is busy; cannot process request */
    LORA_ERRNO_NOTJOINED,   /**< stack is not joined; cannot process request */
    LORA_ERRNO_POWER,       /**< power setting not valid for region */
    LORA_ERRNO_INTERNAL     /**< implementation fault */
};

struct lora_mac_channel {
    
    uint32_t freqAndRate;
    uint32_t dlFreq;
};

/** session parameter cache */
struct lora_mac_session {
    
    uint16_t up;
    uint16_t down;
    
    uint8_t appSKey[16U];
    uint8_t nwkSKey[16U];    

    uint32_t devAddr;
    
    struct lora_mac_channel chConfig[16U];
    
    uint8_t chMask[72U / 8U];
    
    uint8_t rate;
    uint8_t power;
    
    uint8_t maxDutyCycle;    
    uint8_t nbTrans;
    
    uint8_t rx1DROffset;    
    uint8_t rx1Delay;        
    uint8_t rx2DataRate;    
    uint8_t rx2Rate;   
    
    uint32_t rx2Freq;
    
    bool joined;
    bool adr;
};

/** MAC layer data */
struct lora_mac {

    enum lora_mac_state state;
    enum lora_mac_operation op;
    enum lora_mac_errno errno;
    
#ifdef LORA_ENABLE_STATIC_RX_BUFFER
    uint8_t rx_buffer[LORA_MAX_PACKET];
#endif    
    uint8_t buffer[LORA_MAX_PACKET];
    uint8_t bufferLen;

    uint8_t band_ready;

    uint16_t devNonce;
    
    int16_t margin;

    uint32_t last_downlink;
    
    /* the settings currently being used to TX */
    struct {
        
        uint8_t chIndex;
        uint32_t freq;
        uint8_t rate;
        uint8_t power;
        
    } tx;
    
    uint32_t rx1_margin;
    uint32_t rx2_margin;
    
    uint8_t rx1_symbols;    
    uint8_t rx2_symbols;
    
    struct lora_mac_session ctx;
    
    struct lora_radio *radio;
    struct lora_event events;
    enum lora_region region;
    
    lora_mac_response_fn handler;
    void *app;
    
    bool linkCheckReq_pending;
    bool rxParamSetupAns_pending;    
    bool dlChannelAns_pending;
    bool rxtimingSetupAns_pending;
    
    uint8_t adrAckCounter;
    bool adrAckReq;
    
    uint32_t time;
    uint32_t firstJoinAttempt;
    uint32_t msUntilRetry;
    uint16_t joinTrial;
    
    uint8_t tx_dither;
};

/** Initialise MAC 
 * 
 * @param[in] self
 * @param[in] app       passed back in #lora_mac_response_fn and applicable @ref ldl_system functions
 * @param[in] region    lorawan region id
 * @param[in] radio     initialised radio object see LDL_Radio_init()
 * @param[in] handler   application callback
 * 
 * */
void LDL_MAC_init(struct lora_mac *self, void *app, enum lora_region region, struct lora_radio *radio, lora_mac_response_fn handler);

/** Send data upstream without confirmation
 * 
 * @param[in] self
 * @param[in] port 
 * @param[in] data pointer to message to send (will be cached by MAC)
 * @param[in] len byte length of data
 * 
 * @return request result
 * 
 * @retval true upstream data pending pending
 * @retval false error, LDL_MAC_errno() will give reason
 * 
 * */
bool LDL_MAC_unconfirmedData(struct lora_mac *self, uint8_t port, const void *data, uint8_t len);

/** Send data upstream with confirmation
 * 
 * @param[in] self
 * @param[in] port 
 * @param[in] data pointer to message to send (will be cached by MAC)
 * @param[in] len byte length of data
 * 
 * @return request result
 * 
 * @retval true upstream data pending pending
 * @retval false error, LDL_MAC_errno() will give reason
 * 
 * */
bool LDL_MAC_confirmedData(struct lora_mac *self, uint8_t port, const void *data, uint8_t len);

/** Initiate over the air join procedure (or re-join if already joined)
 * 
 * @note MAC will try to join forever if no respose is received or 
 *       until application calls #LDL_MAC_cancel
 * 
 * @param[in] self
 * 
 * @return request result
 * 
 * @retval true join request pending
 * @retval false error, LDL_MAC_errno() will give reason
 * 
 * */
bool LDL_MAC_otaa(struct lora_mac *self);

/** Forget network and all settings
 * 
 * @param[in] self
 * 
 * */
void LDL_MAC_forget(struct lora_mac *self);

/** Perform a link check either now (as own message) or in future (as piggy-back)
 * 
 * @param[in] self 
 * @param[in] now `true` to send as message now (`false` to piggy back at next opportunity)
 * 
 * @return request result
 * 
 * @retval true check link request pending
 * @retval false error, LDL_MAC_errno() will give reason
 * 
 * */
bool LDL_MAC_check(struct lora_mac *self, bool now);

/** Return state to IDLE
 * 
 * @note has no effect if state is RESET 
 *       (i.e. stack is performing chip reset cycle which will complete in so many millseconds)
 * 
 * @param[in] self
 * 
 * */
void LDL_MAC_cancel(struct lora_mac *self);

/** Drive the MAC to process next events
 * 
 * @param[in] self
 * 
 * */
void LDL_MAC_process(struct lora_mac *self);

/** Get number of ticks until the next event
 * 
 * @note this function is safe to call from mainloop and interrupt
 * 
 * @param[in] self
 * 
 * @return ticks until next event
 * 
 * @retval UINT32_MAX there are no future events at this time
 * 
 * */
uint32_t LDL_MAC_ticksUntilNextEvent(const struct lora_mac *self);

/** Get number of ticks until another channel is available
 * 
 * @param[in] self
 * 
 * @return ticks until next channel
 * 
 * */
uint32_t LDL_MAC_ticksUntilNextChannel(const struct lora_mac *self);

/** Set the transmit data rate
 * 
 * @param[in] self
 * @param[in] rate
 * 
 * @retval true data rate applied
 * @retval false error, LDL_MAC_errno() will give reason
 * 
 * */
bool LDL_MAC_setRate(struct lora_mac *self, uint8_t rate);

/** Get the current transmit data rate
 * 
 * @param[in] self
 * 
 * @return transmit data rate setting
 * 
 * */
uint8_t LDL_MAC_getRate(const struct lora_mac *self);

/** Set the transmit power
 * 
 * @param[in] self
 * @param[in] power
 * 
 * @return request result
 * 
 * @retval true power applied
 * @retval false error, LDL_MAC_errno() will give reason
 * 
 * */
bool LDL_MAC_setPower(struct lora_mac *self, uint8_t power);

/** Get the current transmit power
 * 
 * @param[in] self
 * 
 * @return transmit power setting
 * 
 * */
uint8_t LDL_MAC_getPower(const struct lora_mac *self);

/** Enable ADR mode
 * 
 * @param[in] self
 * 
 * @retval true enabled
 * @retval false error, LDL_MAC_errno() will give reason
 * 
 * */
bool LDL_MAC_enableADR(struct lora_mac *self);

/** Disable ADR mode
 * 
 * @param[in] self
 * 
 * @retval true enabled
 * @retval false error, LDL_MAC_errno() will give reason
 * 
 * */
bool LDL_MAC_disableADR(struct lora_mac *self);

/** Is ADR mode enabled?
 * 
 * @param[in] self
 * 
 * @return answer
 * 
 * @retval true ADR is enabled
 * @retval false ADR is not enabled
 * 
 * */
bool LDL_MAC_adr(const struct lora_mac *self);

/** Read the last error
 * 
 * @param[in] self
 * 
 * @return #lora_mac_errno
 * 
 * */
enum lora_mac_errno LDL_MAC_errno(const struct lora_mac *self);

/** Read the current operation
 * 
 * @param[in] self
 * 
 * @return #lora_mac_operation
 * 
 * */
enum lora_mac_operation LDL_MAC_op(const struct lora_mac *self);

/** Read the current state
 * 
 * @param[in] self
 * 
 * @return #lora_mac_state
 * 
 * */
enum lora_mac_state LDL_MAC_state(const struct lora_mac *self);

/** Is MAC joined?
 * 
 * @param[in] self
 * 
 * @return answer
 * 
 * @retval true joined
 * @retval false not joined
 * 
 * */
bool LDL_MAC_joined(const struct lora_mac *self);

/** Is MAC ready to send?
 * 
 * @param[in] self
 * 
 * @retval true ready
 * @retval false not ready
 *
 * */
bool LDL_MAC_ready(const struct lora_mac *self);

/** Calculate transmit time of message sent from node
 * 
 * @param[in] bw    bandwidth
 * @param[in] sf    spreading factor
 * @param[in] size  size of message
 * 
 * @return time in system ticks
 * 
 * */
uint32_t LDL_MAC_transmitTimeUp(enum lora_signal_bandwidth bw, enum lora_spreading_factor sf, uint8_t size);

/** Calculate transmit time of message sent from gateway
 * 
 * @param[in] bw    bandwidth
 * @param[in] sf    spreading factor
 * @param[in] size  size of message
 * 
 * @return time in system ticks
 * 
 * */
uint32_t LDL_MAC_transmitTimeDown(enum lora_signal_bandwidth bw, enum lora_spreading_factor sf, uint8_t size);

/** Convert bandwidth enumeration to Hz
 * 
 * @param[in] bw bandwidth
 * @return Hz
 * 
 * */
uint32_t LDL_MAC_bwToNumber(enum lora_signal_bandwidth bw);

/** Signal a DIO interrupt
 * 
 * @param[in] bw bandwidth
 * @param[in] n DIO number (e.g. DIO0 is 0)
 * @param[in] time system time
 * 
 * */
void LDL_MAC_interrupt(struct lora_mac *self, uint8_t n, uint32_t time);

/** Get application maximum transfer unit in bytes
 * 
 * This number changes depending on:
 * 
 * - region
 * - rate
 * - pending mac commands
 * 
 * @param[in] self
 * @retval payload size in bytes
 * 
 * */
uint8_t LDL_MAC_mtu(const struct lora_mac *self);

/** Seconds since last downlink message
 * 
 * @param[in] self
 * @return seconds since last downlink
 * 
 * */
uint32_t LDL_MAC_timeSinceDownlink(struct lora_mac *self);

/** Add (0..dither) seconds of randomisation to the time next message
 * is sent
 * 
 * @note useful for ensuring devices that transmit periodically do
 * not always transmit at the same time
 * 
 * @warning applies only to the next message sent
 * 
 * @param[in] self
 * @param[in] dither (0..dither) seconds
 * 
 * */
void LDL_MAC_setSendDither(struct lora_mac *self, uint8_t dither);

/** Set the aggregated duty cycle limit
 * 
 * duty cycle limit = 1 / (2 ^ limit)
 * 
 * @note useful for meeting network imposed fair access policy
 * 
 * @param[in] self
 * @param[limit] limit aggregated duty cycle limit
 * 
 * */
void LDL_MAC_setAggregatedDutyCycleLimit(struct lora_mac *self, uint8_t limit);

#ifdef __cplusplus
}
#endif

/** @} */
#endif
