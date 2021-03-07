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

#ifndef LDL_MAC_H
#define LDL_MAC_H

/** @file */

/**
 * @defgroup ldl_mac MAC
 *
 * Before accessing any of the interfaces @ref ldl_mac must be initialised
 * by calling LDL_MAC_init().
 *
 * Any operation that cannot be completed immediately runs from a state
 * machine in LDL_MAC_process(). Applications that sleep can use LDL_MAC_ticksUntilNextEvent()
 * to work out when LDL_MAC_process() needs to be called.
 *
 * Events (#ldl_mac_response_type) that occur within LDL_MAC_process() are pushed back to the
 * application using the #ldl_mac_response_fn function pointer.
 * This includes everything from state change notifications
 * to data received from the network.
 *
 * Data services are not available until #ldl_mac is joined to a network.
 * The join procedure is initiated by calling LDL_MAC_otaa(). Join will
 * run indefintely until either join succeeds, or the application
 * calls LDL_MAC_cancel() or LDL_MAC_forget(). The application can check
 * on the join status by calling LDL_MAC_joined().
 *
 * The application can un-join by call LDL_MAC_forget().
 *
 * The application can cancel an operation by calling LDL_MAC_cancel().
 *
 * @{
 * */

#ifdef __cplusplus
extern "C" {
#endif

#include "ldl_platform.h"
#include "ldl_region.h"
#include "ldl_radio.h"
#include "ldl_mac_commands.h"
#include "ldl_mac_internal.h"
#include "ldl_system.h"

#include <stdint.h>
#include <stdbool.h>

struct ldl_mac;
struct ldl_sm;

/** Event types pushed to application
 *
 * @see ldl_mac_response_arg
 *
 *  */
enum ldl_mac_response_type {

    /** Entropy has received from the radio driver
     *
     * */
    LDL_MAC_ENTROPY,

    /** A channel has become ready.
     *
     * This event can be spurious in some situations and so make sure
     * to check that LDL can actually send on next attempt.
     *
     * */
    LDL_MAC_CHANNEL_READY,

    /** The last requested OP failed due to a radio error
     *
     *
     *
     * */
    LDL_MAC_OP_ERROR,

    /** The last requested OP was cancelled using LDL_MAC_cancel()
     *
     *
     * */
    LDL_MAC_OP_CANCELLED,

    /** Join request was answered and MAC is now joined
     *
     * */
    LDL_MAC_JOIN_COMPLETE,

    /** DevNonce has been incremented
     *
     *
     * */
    LDL_MAC_DEV_NONCE_UPDATED,

    /** OTAA failed because DevNonce has incremented more than
     * 65535 times.
     *
     * */
    LDL_MAC_JOIN_EXHAUSTED,

    /** data request (confirmed or unconfirmed) completed successfully */
    LDL_MAC_DATA_COMPLETE,

    /** confirmed data request was not answered */
    LDL_MAC_DATA_TIMEOUT,

    /** data received */
    LDL_MAC_RX,

    /** LinkCheckAns received */
    LDL_MAC_LINK_STATUS,

    /** #ldl_mac_session has changed
     *
     * The application can choose to save the session at this point
     *
     * */
    LDL_MAC_SESSION_UPDATED,

    /** deviceTimeAns received
     *
     * */
    LDL_MAC_DEVICE_TIME
};

enum ldl_mac_sme {

    LDL_SME_NONE,
    LDL_SME_TIMER_A,
    LDL_SME_TIMER_B,
    LDL_SME_INTERRUPT,
    LDL_SME_BAND
};

/** MAC state */
enum ldl_mac_state {

    LDL_STATE_INIT,         /**< stack has been memset to zero */

    LDL_STATE_RADIO_RESET,     /**< holding reset */
    LDL_STATE_RADIO_BOOT,      /**< waiting for radio to start after reset */

    LDL_STATE_IDLE,         /**< ready for operations */

    LDL_STATE_WAIT_ENTROPY,
    LDL_STATE_START_RADIO_FOR_ENTROPY,  /**< waiting for radio to start before entropy */
    LDL_STATE_ENTROPY,                  /**< waiting to receive entropy */

    LDL_STATE_WAIT_OTAA,
    LDL_STATE_WAIT_TX,               /**< waiting for channel to become available */
    LDL_STATE_START_RADIO_FOR_TX,    /**< waiting for radio to start before TX */
    LDL_STATE_TX,           /**< radio is TX */
    LDL_STATE_WAIT_RX1,     /**< waiting for first RX window */
    LDL_STATE_START_RADIO_FOR_RX1,
    LDL_STATE_RX1,          /**< first RX window */
    LDL_STATE_WAIT_RX2,     /**< waiting for second RX window */
    LDL_STATE_START_RADIO_FOR_RX2,     /**< waiting for second RX window */
    LDL_STATE_RX2,          /**< second RX window */

    LDL_STATE_RX2_LOCKOUT  /**< used to ensure an out of range RX2 window is not clobbered */

};

/** MAC operations */
enum ldl_mac_operation {

    LDL_OP_NONE,                /**< no active operation */
    LDL_OP_ENTROPY,             /**< MAC is gathering entropy */
    LDL_OP_JOINING,             /**< MAC is performing a join */
    LDL_OP_REJOINING,           /**< MAC is performing a rejoin */
    LDL_OP_DATA_UNCONFIRMED,    /**< MAC is sending unconfirmed data */
    LDL_OP_DATA_CONFIRMED,      /**< MAC is sending confirmed data */
};

/** MAC operation return status codes */
enum ldl_mac_status {

    LDL_STATUS_OK,          /**< success/pending */
    LDL_STATUS_NOCHANNEL,   /**< upstream channel not available */
    LDL_STATUS_SIZE,        /**< message too large to send */
    LDL_STATUS_RATE,        /**< invalid rate setting for region */
    LDL_STATUS_PORT,        /**< invalid port number */
    LDL_STATUS_BUSY,        /**< cannot perform operation because busy */
    LDL_STATUS_NOTJOINED,   /**< cannot perform operation because not joined */
    LDL_STATUS_POWER,       /**< invalid power setting for region */
    LDL_STATUS_MACPRIORITY, /**< data request failed due to MAC command(s) being prioritised */
    LDL_STATUS_JOINED,      /**< cannot join because already joined */
    LDL_STATUS_DEVNONCE,    /**< cannot join because DevNonce is exhausted */

    /* the following status codes are available for
     * use by wrappers with blocking interfaces */

    LDL_STATUS_NOACK,       /**< confirmed service did not receive acknowledgement */
    LDL_STATUS_CANCELLED,   /**< service was cancelled */
    LDL_STATUS_TIMEOUT,     /**< user timeout waiting for service */
    LDL_STATUS_ERROR,       /**< hardware error */
};

/** Event arguments sent to application
 *
 * @see ldl_mac_response_type
 *
 *  */
union ldl_mac_response_arg {

    /** #LDL_MAC_RX argument */
    struct {

        const uint8_t *data;    /**< message data */
        uint8_t port;           /**< lorawan application port */
        uint8_t size;           /**< size of message */

    } rx;

    /** #LDL_MAC_LINK_STATUS argument */
    struct {

        uint8_t margin;      /**< SNR margin */
        uint8_t gwCount;    /**< number of gateways in range */

    } link_status;

    /** #LDL_MAC_ENTROPY argument */
    struct {

        uint32_t value;     /**< srand seed from radio driver */

    } entropy;

    /** #LDL_MAC_SESSION_UPDATED argument
     *
     * */
    struct {

        const struct ldl_mac_session *session;

    } session_updated;

    /** #LDL_MAC_DEVICE_TIME argument */
    struct {

        uint64_t time;      /**< seconds|fractions */
        uint32_t seconds;   /**< seconds since jan 5 1980 */
        uint8_t fractions;  /**< 1/255 of a second */

    } device_time;

    /** #LDL_MAC_JOIN_COMPLETE argument */
    struct {

        /** the most up to date joinNonce */
        uint32_t joinNonce;
        /** netID */
        uint32_t netID;
        /** devAddr */
        uint32_t devAddr;

    } join_complete;

    /** The next devNonce
     *
     * This can be saved and restored (at LDL_MAC_init()).
     *
     *  */
    struct {

        uint32_t nextDevNonce;

    } dev_nonce_updated;
};

/** LDL calls this function pointer to notify application of events
 *
 * @param[in] app   from #ldl_mac_init_arg.app
 * @param[in] type  #ldl_mac_response_type
 * @param[in] arg   **OPTIONAL** depending on #ldl_mac_response_type
 *
 * */
typedef void (*ldl_mac_response_fn)(void *app, enum ldl_mac_response_type type, const union ldl_mac_response_arg *arg);

/** Data service invocation options */
struct ldl_mac_data_opts {

    uint8_t nbTrans;        /**< redundancy (0..LDL_REDUNDANCY_MAX) */
    bool check;             /**< piggy-back a LinkCheckReq */
    bool getTime;           /**< piggy-back a DeviceTimeReq */
};


enum ldl_band_index {

    LDL_BAND_1,         /* off-time per sub-band (depends on the region) */
    LDL_BAND_2,
    LDL_BAND_3,
    LDL_BAND_4,
    LDL_BAND_5,
    LDL_BAND_GLOBAL,    /* global off-time counter */
    LDL_BAND_MAX
};

struct ldl_timer {

    uint32_t time;
    bool armed;
};

struct ldl_input {

    bool armed;
    bool state;
    uint32_t time;
};

struct ldl_mac_channel {

    uint32_t freqAndRate;
    uint32_t dlFreq;
};

struct ldl_mac_time {

    uint32_t ticks;
    uint32_t remainder;
    bool parked;
};

/** Session cache */
struct ldl_mac_session {

    /* sanity check against accepting uninitialised memory as session */
    uint8_t magic;
    bool joined;
    bool adr;

    /* 0: 1.0 backend, 1: 1.1 backend */
#if defined(LDL_ENABLE_L2_1_1)
    uint8_t version;
#endif

    enum ldl_region region;

    /* frame counters */
    uint32_t up;
    uint16_t appDown;
    uint16_t nwkDown;

    uint32_t devAddr;
    uint32_t netID;

    struct ldl_mac_channel chConfig[16U];

    uint8_t chMask[72U / 8U];
    uint8_t rate;
    uint8_t power;
    uint8_t maxDutyCycle;
    uint8_t nbTrans;
    uint8_t rx1DROffset;
    uint8_t rx1Delay;
    uint8_t rx2DataRate;

    uint32_t rx2Freq;

    uint16_t adr_ack_limit;
    uint16_t adr_ack_delay;

    struct ldl_rx_param_setup_ans rx_param_setup_ans;
    struct ldl_dl_channel_ans dl_channel_ans;
    struct ldl_link_adr_ans link_adr_ans;
    struct ldl_dev_status_ans dev_status_ans;
    struct ldl_new_channel_ans new_channel_ans;
    struct ldl_rejoin_param_setup_ans rejoin_param_setup_ans;

    uint32_t joinNonce;
    uint16_t devNonce;
    uint16_t pending_cmds;

#ifndef LDL_DISABLE_TX_PARAM_SETUP
    uint8_t tx_param_setup;
#endif
};

struct ldl_mac_tx {

    uint32_t freq;
    uint32_t counter;
    uint32_t airTime;
    uint8_t chIndex;
    uint8_t rate;
    uint8_t power;

};

/** MAC layer data */
struct ldl_mac {

    enum ldl_mac_state state;
    enum ldl_mac_operation op;

    bool pendingACK;

    uint8_t joinEUI[8U];
    uint8_t devEUI[8U];

#ifdef LDL_ENABLE_STATIC_RX_BUFFER
    uint8_t rx_buffer[LDL_MAX_PACKET];
#endif
    uint8_t buffer[LDL_MAX_PACKET];
    uint8_t bufferLen;

    /* down-counters that use the 'time' timebase
     *
     * used for duty cycle timing per band among other things */
    uint32_t band[LDL_BAND_MAX];

    /* day down-counter used by OTAA to apply duty-cycle reduction
     *
     * 'time' timebase like the band down-counters
     * */
    uint32_t day;

    /* 32bit to detect 16bit overflow */
    uint32_t devNonce;
    uint32_t joinNonce;

    int16_t rx_snr;

    /* the settings currently being used to TX */
    struct ldl_mac_tx tx;

    uint16_t rx1_symbols;
    uint16_t rx2_symbols;

    struct ldl_mac_session ctx;

    struct ldl_sm *sm;
    struct ldl_radio *radio;
    struct ldl_input inputs;
    struct ldl_timer timers[LDL_TIMER_MAX];

    const struct ldl_sm_interface *sm_interface;
    const struct ldl_radio_interface *radio_interface;

    ldl_mac_response_fn handler;
    void *app;

    uint8_t adrAckCounter;
    bool adrAckReq;

    struct ldl_mac_time time;

#ifndef LDL_DISABLE_DEVICE_TIME
    /* used to provide precise time sync */
    uint32_t ticks_at_tx;
#endif

    /* number of join/data trials */
    uint32_t trials;

    /* options and overrides applicable to current data service */
    struct ldl_mac_data_opts opts;

    /* system interfaces */
    ldl_system_rand_fn rand;
    ldl_system_ticks_fn ticks;
    ldl_system_get_battery_level_fn get_battery_level;

    bool fPending;

#ifndef LDL_PARAM_TPS
    uint32_t tps;
#endif
#ifndef LDL_PARAM_A
    uint32_t a;
#endif
#ifndef LDL_PARAM_B
    uint32_t b;
#endif
#ifndef LDL_PARAM_ADVANCE
    uint32_t advance;
#endif

    uint8_t maxDutyCycle;

#ifdef LDL_ENABLE_OTAA_DITHER
    uint32_t otaaDither;
#endif

#ifdef LDL_ENABLE_TEST_MODE
    bool unlimitedDutyCycle;
#endif
};

/** Passed as an argument to LDL_MAC_init()
 *
 * */
struct ldl_mac_init_arg {

    /** pointer passed to #ldl_mac_response_fn and @ref ldl_system functions */
    void *app;

    /** pointer to initialised Radio */
    struct ldl_radio *radio;

    /** pointer to Radio interfaces */
    const struct ldl_radio_interface *radio_interface;

    /** pointer to initialised Security Module */
    struct ldl_sm *sm;

    /** pointer to SM interfaces */
    const struct ldl_sm_interface *sm_interface;

    /** application callback #ldl_mac_response_fn */
    ldl_mac_response_fn handler;

    /** optional pointer to session data to restore
     *
     *  */
    const struct ldl_mac_session *session;

    /** pointer to 8 byte identifier */
    const void *joinEUI;

    /** pointer to 8 byte identifier */
    const void *devEUI;

    /** the next devNonce to use in OTAA
     *
     * @see #LDL_MAC_DEV_NONCE_UDPATED
     *
     * */
    uint32_t devNonce;

    /** the most up to date joinNonce
     *
     * @see #LDL_MAC_JOIN_COMPLETE
     *
     * */
    uint32_t joinNonce;

    /** System interface for getting random numbers
     *
     * */
    ldl_system_rand_fn rand;

    /** system interface for getting ticks
     *
     * */
    ldl_system_ticks_fn ticks;

    /** System interface for getting battery level
     *
     * Leave this field NULL to use the default implementation.
     *
     * */
    ldl_system_get_battery_level_fn get_battery_level;

#ifndef LDL_PARAM_TPS
    /** The rate at which #ldl_mac_init_arg.ticks() increments i.e. "ticks per second"
     *
     * This is not required if LDL_PARAM_TPS has been defined.
     *
     * Refer to porting.md for details.
     *
     * */
    uint32_t tps;
#endif

#ifndef LDL_PARAM_A
    /** #ldl_mac_init_arg.ticks() compensation parameter A
     *
     * This is not required if LDL_PARAM_A has been defined.
     *
     * Refer to porting.md for details.
     *
     * */
    uint32_t a;
#endif

#ifndef LDL_PARAM_B
    /** #ldl_mac_init_arg.ticks() compensation parameter B
     *
     * This is not required if LDL_PARAM_B has been defined.
     *
     * Refer to porting.md for details.
     *
     * */
    uint32_t b;
#endif

#ifndef LDL_PARAM_ADVANCE
    /** Advance window open events by this many ticks
     *
     * This is not required if LDL_PARAM_ADVANCE has been defined.
     *
     * Useful if there is a constant and significant latency
     * between when a window is scheduled to open, and when
     * it actually opens.
     *
     * */
    uint32_t advance;
#endif

#ifdef LDL_ENABLE_OTAA_DITHER
    /** (0..otaaDither) ticks to add to JoinReq
     *
     * */
    uint32_t otaaDither;
#endif
};


/** Initialise #ldl_mac
 *
 * Parameters are injected into #ldl_mac via arg.
 *
 * The following paramters are **MANDATORY**:
 *
 * - ldl_mac_init_arg.radio
 * - ldl_mac_init_arg.ticks
 * - ldl_mac_init_arg.joinEUI
 * - ldl_mac_init_arg.devEUI
 * - ldl_mac_init_arg.tps (if #LDL_PARAM_TPS is not defined)
 * - ldl_mac_init_arg.a (if #LDL_PARAM_A is not defined)
 * - ldl_mac_init_arg.b (if #LDL_PARAM_B is not defined)
 * - ldl_mac_init_arg.advance (if #LDL_PARAM_ADVANCE is not defined)
 * - ldl_mac_init_arg.sm
 * - ldl_mac_init_arg.sm_interface
 * - ldl_mac_init_arg.radio
 * - ldl_mac_init_arg.radio_interface
 *
 * The following parameters are **OPTIONAL** (leave NULL or 0):
 *
 * - ldl_mac_init_arg.app
 * - ldl_mac_init_arg.handler
 * - ldl_mac_init_arg.session
 * - ldl_mac_init_arg.devNonce
 * - ldl_mac_init_arg.joinNonce
 * - ldl_mac_init_arg.rand
 * - ldl_mac_init_arg.get_battery_level
 *
 * @param[in] self      #ldl_mac
 * @param[in] region    #ldl_region
 * @param[in] arg       #ldl_mac_init_arg
 *
 *
 * @code
 * struct ldl_mac mac;
 * struct ldl_mac_init_arg arg = {0};
 *
 * // ...
 *
 * LDL_MAC_init(&mac, &arg);
 * @endcode
 *
 * */
void LDL_MAC_init(struct ldl_mac *self, enum ldl_region region, const struct ldl_mac_init_arg *arg);

/** Read entropy from the radio
 *
 * The application shall be notified of completion via #ldl_mac_response_fn.
 * One of the following events can be expected:
 *
 * - #LDL_MAC_ENTROPY
 * - #LDL_MAC_OP_ERROR
 * - #LDL_MAC_OP_CANCELLED
 *
 * @param[in] self #ldl_mac
 *
 * @return #ldl_mac_status
 *
 * @retval #LDL_STATUS_OK
 * @retval #LDL_STATUS_BUSY
 *
 * */
enum ldl_mac_status LDL_MAC_entropy(struct ldl_mac *self);

/** Initiate Over The Air Activation
 *
 * Once initiated MAC will keep trying to join forever.
 *
 * The application can cancel the service while it is in progress by
 * calling LDL_MAC_cancel().
 *
 * OTAA will fail permanently if more than 65535 join requests are
 * made using the same join EUI. This will be indicated by the #LDL_MAC_JOIN_EXHAUSTED
 * event and any future calls to LDL_MAC_otaa() will return #LDL_STATUS_DEVNONCE.
 * Join EUI is changed at LDL_MAC_init().
 *
 * The application shall be notified of completion via #ldl_mac_response_fn.
 * One of the following events can be expected:
 *
 * - #LDL_MAC_JOIN_COMPLETE
 * - #LDL_MAC_OP_CANCELLED
 * - #LDL_MAC_JOIN_EXHAUSTED
 *
 * Unlike data services, OTAA will continue even if radio errors
 * are detected.
 *
 * @param[in] self  #ldl_mac
 *
 * @return #ldl_mac_status
 *
 * @retval #LDL_STATUS_OK
 * @retval #LDL_STATUS_BUSY
 * @retval #LDL_STATUS_JOINED
 * @retval #LDL_STATUS_DEVNONCE
 *
 * */
enum ldl_mac_status LDL_MAC_otaa(struct ldl_mac *self);

enum ldl_mac_status LDL_MAC_abp(struct ldl_mac *self, uint32_t devAddr);

/** Send data without network confirmation
 *
 * Once initiated MAC will send at most nbTrans times or until a valid downlink is received. NbTrans may be set:
 *
 * - globally by the network (via LinkADRReq)
 * - per invocation by #ldl_mac_data_opts
 *
 * The application can cancel the service while it is in progress by calling LDL_MAC_cancel().
 *
 * The application shall be notified of completion via #ldl_mac_response_fn.
 * One of the following events can be expected:
 *
 * - #LDL_MAC_DATA_COMPLETE
 * - #LDL_MAC_OP_ERROR
 * - #LDL_MAC_OP_CANCELLED
 *
 * Be aware that pending MAC commands are piggy-backed onto upstream data frames.
 * If the pending MAC commands will not fit into the same frame as the
 * application data, the MAC commands will be prioritised.
 *
 * In the event this happens, please be aware that:
 *
 * 1. LDL_MAC_unconfirmedData() function will return #LDL_STATUS_MACPRIORITY to indicate non-success for sending
 *    user data
 * 2. LDL will send the pending MAC commands as an unconfirmed data frame with
 *    the usual event notifications coming back to the application.
 *
 * An application encountering #LDL_STATUS_MACPRIORITY should try again
 * after the MAC commands have been sent.
 *
 * @param[in] self  #ldl_mac
 * @param[in] port  lorawan port (must be >0)
 * @param[in] data  pointer to message to send
 * @param[in] len   byte length of data
 * @param[in] opts  #ldl_mac_data_opts (may be NULL)
 *
 * @return #ldl_mac_status
 *
 * @retval #LDL_STATUS_OK
 * @retval #LDL_STATUS_NOTJOINED
 * @retval #LDL_STATUS_BUSY
 * @retval #LDL_STATUS_PORT
 * @retval #LDL_STATUS_NOCHANNEL
 * @retval #LDL_STATUS_SIZE
 * @retval #LDL_STATUS_MACPRIORITY
 *
 * */
enum ldl_mac_status LDL_MAC_unconfirmedData(struct ldl_mac *self, uint8_t port, const void *data, uint8_t len, const struct ldl_mac_data_opts *opts);

/** Send data with network confirmation
 *
 * Once initiated MAC will send at most nbTrans times until a confirmation is received.
 * NbTrans may be set per invocation by #ldl_mac_data_opts.nbTrans and has precedence over
 * the value possibly received from network.
 * As of LoRaWAN standard 1.0.4, the network ignores frames if it receives more than nbTrans
 * frames, whereby the nbTrans value is determined on the server. Overriding this value with
 * client options can therefore be helpful if the uplink transmission is lost, but no longer
 * has a positive effect if the acknowledgment downlink frame is lost.
 *
 * The application can cancel the operation while it is in progress by calling LDL_MAC_cancel().
 *
 * The application shall be notified of completion via #ldl_mac_response_fn. One of the following events
 * can be expected:
 *
 * - #LDL_MAC_DATA_COMPLETE
 * - #LDL_MAC_DATA_TIMEOUT
 * - #LDL_MAC_OP_ERROR
 * - #LDL_MAC_OP_CANCELLED
 *
 * MAC commands are piggy-backed and prioritised the same as they are for
 * LDL_MAC_unconfirmedData().
 *
 * @param[in] self  #ldl_mac
 * @param[in] port  lorawan port (must be >0)
 * @param[in] data  pointer to message to send
 * @param[in] len   byte length of data
 * @param[in] opts  #ldl_mac_data_opts (may be NULL)
 *
 * @return #ldl_mac_status
 *
 * @retval #LDL_STATUS_OK
 * @retval #LDL_STATUS_NOTJOINED
 * @retval #LDL_STATUS_BUSY
 * @retval #LDL_STATUS_PORT
 * @retval #LDL_STATUS_NOCHANNEL
 * @retval #LDL_STATUS_SIZE
 * @retval #LDL_STATUS_MACPRIORITY
 *
 * */
enum ldl_mac_status LDL_MAC_confirmedData(struct ldl_mac *self, uint8_t port, const void *data, uint8_t len, const struct ldl_mac_data_opts *opts);

/** Forget network and cancel any operation
 *
 * @param[in] self  #ldl_mac
 *
 * */
void LDL_MAC_forget(struct ldl_mac *self);

/** Cancel any operation
 *
 * Calling this function will always cause the radio hardware to be
 * reset. This may be useful for long running applications that wish
 * to periodically cycle the radio.
 *
 * @param[in] self  #ldl_mac
 *
 * */
void LDL_MAC_cancel(struct ldl_mac *self);

/** Call to process next events
 *
 * @param[in] self  #ldl_mac
 *
 * */
void LDL_MAC_process(struct ldl_mac *self);

/** Get number of ticks until the next event
 *
 * @param[in] self  #ldl_mac
 *
 * @return system ticks
 *
 * @retval UINT32_MAX   there are no future events at this time
 *
 * @note interrupt safe if LDL_SYSTEM_ENTER_CRITICAL() and LDL_SYSTEM_ENTER_CRITICAL() have been defined
 *
 * */
uint32_t LDL_MAC_ticksUntilNextEvent(const struct ldl_mac *self);

/** Set the desired transmit data rate
 *
 * @param[in] self  #ldl_mac
 * @param[in] rate
 *
 * @return #ldl_mac_status
 *
 * @retval #LDL_STATUS_OK
 * @retval #LDL_STATUS_RATE
 *
 * */
enum ldl_mac_status LDL_MAC_setRate(struct ldl_mac *self, uint8_t rate);

/** Get the current desired transmit data rate
 *
 * @param[in] self  #ldl_mac
 *
 * @return transmit data rate setting
 *
 * */
uint8_t LDL_MAC_getRate(const struct ldl_mac *self);

/** Set the transmit power
 *
 * @param[in] self  #ldl_mac
 * @param[in] power
 *
 * @return #ldl_mac_status
 *
 * @retval #LDL_STATUS_OK
 * @retval #LDL_STATUS_POWER
 *
 * */
enum ldl_mac_status LDL_MAC_setPower(struct ldl_mac *self, uint8_t power);

/** Get the current transmit power
 *
 * @param[in] self  #ldl_mac
 *
 * @return transmit power setting
 *
 * */
uint8_t LDL_MAC_getPower(const struct ldl_mac *self);

/** Enable or Disable ADR
 *
 * Note that ADR is enabled by default after OTAA is successful.
 *
 * @param[in] self  #ldl_mac
 * @param[in] value
 *
 * */
void LDL_MAC_setADR(struct ldl_mac *self, bool value);

/** Is ADR mode enabled?
 *
 * @param[in] self  #ldl_mac
 *
 * @retval true     enabled
 * @retval false    not enabled
 *
 * */
bool LDL_MAC_getADR(const struct ldl_mac *self);

/** Read the current operation
 *
 * @param[in] self  #ldl_mac
 *
 * @return #ldl_mac_operation
 *
 * */
enum ldl_mac_operation LDL_MAC_op(const struct ldl_mac *self);

/** Read the current state
 *
 * @param[in] self  #ldl_mac
 *
 * @return #ldl_mac_state
 *
 * */
enum ldl_mac_state LDL_MAC_state(const struct ldl_mac *self);

/** Is MAC joined?
 *
 * @param[in] self  #ldl_mac
 *
 * @retval true     joined
 * @retval false    not joined
 *
 * */
bool LDL_MAC_joined(const struct ldl_mac *self);

/** Is MAC ready to send?
 *
 * @param[in] self  #ldl_mac
 *
 * @retval true     ready
 * @retval false    not ready
 *
 * */
bool LDL_MAC_ready(const struct ldl_mac *self);

/** Get the maximum transfer unit in bytes
 *
 * The MTU depends on:
 *
 * - region
 * - channel mask settings
 * - pending mac commands
 *
 * @param[in] self  #ldl_mac
 * @retval mtu
 *
 * */
uint8_t LDL_MAC_mtu(const struct ldl_mac *self);

/** Set the aggregated duty cycle limit
 *
 * duty cycle limit = 1 / (2 ^ limit)
 *
 * This is useful for slowing the rate at which a device is able
 * to send messages, especially if the device is operating in a
 * region that doesn't limit duty cycle.
 *
 * e.g.
 *
 * @code
 * maxDCycle=0 (off-time=1)
 * maxDCycle=6 (off-time=0.015)
 * maxDCycle=12 (off-time=0.00024)
 * etc.
 * @endcode
 *
 * @note the network may reset this via a MAC command
 *
 * @param[in] self  #ldl_mac
 * @param[in] maxDCycle
 *
 * @see LoRaWAN Specification: DutyCycleReq.DutyCyclePL.MaxDCycle
 *
 * */
void LDL_MAC_setMaxDCycle(struct ldl_mac *self, uint8_t maxDCycle);

/** Get aggregated duty cycle limit
 *
 * @param[in] self  #ldl_mac
 *
 * @return maxDCycle
 *
 * @see LoRaWAN Specification: DutyCycleReq.DutyCyclePL.MaxDCycle
 *
 * */
uint8_t LDL_MAC_getMaxDCycle(const struct ldl_mac *self);

/** Return true to indicate that LDL is expecting to handle
 * a time sensitive event in the next interval.
 *
 * This can be used by an application to ensure long-running tasks
 * do not cause LDL to miss important events.
 *
 * @param[in] self      #ldl_mac
 * @param[in] interval  seconds
 *
 *
 * @retval true
 * @retval false
 *
 * */
bool LDL_MAC_priority(const struct ldl_mac *self, uint8_t interval);

/** Radio calls this function to notify MAC that an event has occurred.
 *
 * Normally this will happen via the function pointer set as an
 * argument in LDL_Radio_setHandler().
 *
 * @param[in] self      #ldl_mac
 *
 * @note interrupt safe if LDL_SYSTEM_ENTER_CRITICAL() and LDL_SYSTEM_ENTER_CRITICAL() have been defined
 *
 * */
void LDL_MAC_radioEvent(struct ldl_mac *self);

/** LDL_MAC_radioEvent() but with ticks passed as argument
 *
 * Normally this will happen via the function pointer set as an
 * argument in LDL_Radio_setHandler().
 *
 * @param[in] self      #ldl_mac
 * @param[in] ticks             timestamp from LDL_MAC_getTicks()
 *
 * @note interrupt safe if LDL_SYSTEM_ENTER_CRITICAL() and LDL_SYSTEM_ENTER_CRITICAL() have been defined
 *
 * */
void LDL_MAC_radioEventWithTicks(struct ldl_mac *self, uint32_t ticks);

/** Get current tick value
 *
 * @param[in] self  #ldl_mac
 * @return ticks
 *
 * */
uint32_t LDL_MAC_getTicks(struct ldl_mac *self);

/** Use this setting to remove duty cycle limits to speed up testing
 *
 * This feature is only available when LDL_ENABLE_TEST_MODE is enabled.
 * It must not be used in production.
 *
 * @param[in] self #ldl_mac
 * @param[in] value true for enabled, false for disabled
 *
 * */
void LDL_MAC_setUnlimitedDutyCycle(struct ldl_mac *self, bool value);

/** Returns fpending status set by the last data downlink frame.
 *
 * Sometimes data will be queued on the network waiting to be sent
 * in the next downlink slot that follows an uplink. An application may
 * choose to react to this status immediately or wait until the next
 * scheduled uplink.
 *
 * @param[in] self #ldl_mac
 *
 * @retval true     network has more data to send to device
 * @retval false
 *
 *
 * */
bool LDL_MAC_getFPending(const struct ldl_mac *self);

/** Returns ack pending status set by the last data downlink frame.
 *
 * This status is set by confirmed downlinks. The next uplink will
 * acknowledge receipt of the confirmed downlink. An application may
 * choose to react to this status immediately or wait until the next
 * scheduled uplink.
 *
 * @param[in] self #ldl_mac
 *
 * @retval true     network is expecting acknowledgement
 * @retval false
 *
 *
 * */
bool LDL_MAC_getAckPending(const struct ldl_mac *self);

#ifdef __cplusplus
}
#endif

/** @} */
#endif
