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

#ifndef LDL_MAC_H
#define LDL_MAC_H

/** @file */

/**
 * @defgroup ldl LDL
 * 
 * # LDL Interface Documentation
 * 
 * - @ref ldl_mac MAC layer interface
 * - @ref ldl_radio radio driver interface
 * - @ref ldl_system portable system interface
 * - @ref ldl_radio_connector portable connector between radio driver and transceiver digital interface
 * - @ref ldl_build_options portable build options
 * - @ref ldl_tsm portable security module interface
 * - @ref ldl_crypto cryptographic implementations used by the default security module
 * 
 * ## Usage
 * 
 * Below is an example of how to use LDL to join and then send data. 
 * 
 * What isn't shown:
 * 
 * - implementation of functions marked as "extern"
 * - sensible values for @ref ldl_radio_connector and @ref ldl_system implementation
 * 
 * This example would need the following @ref ldl_build_options to be defined:
 * 
 * - #LDL_ENABLE_SX1272
 * - #LDL_ENABLE_EU_863_870
 * 
 * @include examples/doxygen/example.c
 * 
 * ## Examples
 * 
 *  - Arduino (AVR)
 *      - [arduino_ldl.cpp](https://github.com/cjhdev/lora_device_lib/tree/master/wrappers/arduino/output/arduino_ldl/arduino_ldl.cpp)
 *      - [arduino_ldl.h](https://github.com/cjhdev/lora_device_lib/tree/master/wrappers/arduino/output/arduino_ldl/arduino_ldl.h)
 *      - [platform.h](https://github.com/cjhdev/lora_device_lib/tree/master/wrappers/arduino/output/arduino_ldl/platform.h)
 * 
 * */

/**
 * @defgroup ldl_mac MAC
 * @ingroup ldl
 * 
 * # MAC Interface
 * 
 * Before accessing any of the interfaces #ldl_mac must be initialised
 * by calling LDL_MAC_init().
 * 
 * Initialisation will take order of milliseconds but LDL_MAC_init() does 
 * not block, instead it schedules actions to run in the future.
 * It is then the responsibility of the application to poll LDL_MAC_process() 
 * from a loop in order to ensure the schedule is processed. LDL_MAC_ready() will return true
 * when initialisation is complete.
 * 
 * On systems that sleep, LDL_MAC_ticksUntilNextEvent() can be used in combination
 * with a wakeup timer to ensure that LDL_MAC_process() is called only when 
 * necessary. Note that the counter behind LDL_System_ticks() must continue
 * to increment during sleep.
 * 
 * Events (#ldl_mac_response_type) that occur within LDL_MAC_process() are pushed back to the 
 * application using the #ldl_mac_response_fn function pointer. 
 * This includes everything from state change notifications
 * to data received from the network.
 * 
 * The application is free to apply settings while waiting for LDL_MAC_ready() to become
 * true. Setting interfaces are:
 * 
 * - LDL_MAC_setRate()
 * - LDL_MAC_setPower()
 * - LDL_MAC_enableADR()
 * - LDL_MAC_disableADR()
 * - LDL_MAC_setMaxDCycle()
 * - LDL_MAC_setNbTrans()
 * 
 * Data services are not available until #ldl_mac is joined to a network.
 * The join procedure is initiated by calling LDL_MAC_otaa(). LDL_MAC_otaa() will return false if the join procedure cannot be initiated. The application
 * can use LDL_MAC_errno() to discover the reason for failure.
 * 
 * The join procedure will run indefinitely until either the join succeeds, or the application
 * calls LDL_MAC_cancel() or LDL_MAC_forget(). The application can check on the join status
 * by calling LDL_MAC_joined().
 * 
 * Once joined the application can use the data services:
 * 
 * - LDL_MAC_unconfirmedData()
 * - LDL_MAC_confirmedData()
 * 
 * If the application wishes to un-join it can call LDL_MAC_forget().
 * 
 * 
 * @{
 * */

#ifdef __cplusplus
extern "C" {
#endif

#include "ldl_platform.h"
#include "ldl_region.h"
#include "ldl_radio.h"

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
        
    /** diagnostic event: radio chip did not respond as expected and will now be reset
     * 
     * */
    LDL_MAC_CHIP_ERROR,
    
    /** diagnostic event: radio chip is in process of being reset
     * 
     * MAC will send #LDL_MAC_STARTUP when it is ready again
     * 
     * */
    LDL_MAC_RESET,
    
    /** MAC has started and is now ready for commands
     * 
     * */
    LDL_MAC_STARTUP,
    
    /** Join request was answered and MAC is now joined 
     * 
     * Receipt of this event also means:
     * 
     * - Fresh sessions keys have been derived. The application
     *   should ensure that these are cached if you are implementing
     *   persistent sessions.
     * 
     * - devNonce has been updated. The next devNonce
     *   is pushed with this event and should be cached so that
     *   it can be restored the next time LDL_MAC_init() is called.
     * 
     * */
    LDL_MAC_JOIN_COMPLETE,
    
    /** join request was not answered (MAC will try again) */
    LDL_MAC_JOIN_TIMEOUT,
    
    /** data request (confirmed or unconfirmed) completed successfully */
    LDL_MAC_DATA_COMPLETE,
    
    /** confirmed data request was not answered */
    LDL_MAC_DATA_TIMEOUT,
    
    /** confirmed data request was answered but the ACK bit wasn't set */
    LDL_MAC_DATA_NAK,
    
    /** data receieved */
    LDL_MAC_RX,
    
    /** LinkCheckAns */
    LDL_MAC_LINK_STATUS,
    
    /** diagnostic event: RX1 window opened */
    LDL_MAC_RX1_SLOT,
    
    /** diagnostic event: RX2 window opened */
    LDL_MAC_RX2_SLOT,
    
    /** diagnostic event: a frame has been receieved in an RX window */
    LDL_MAC_DOWNSTREAM,
        
    /** diagnostic event: transmit complete */
    LDL_MAC_TX_COMPLETE,
    
    /** diagnostic event: transmit begin */
    LDL_MAC_TX_BEGIN,
    
    /** #ldl_mac_session has changed 
     * 
     * The application can choose to save the session at this point
     * 
     * */
    LDL_MAC_SESSION_UPDATED,
    
    /** deviceTimeAns receieved
     * 
     * */
    LDL_MAC_DEVICE_TIME
};

struct ldl_mac_session;

/** Event arguments sent to application
 * 
 * @see ldl_mac_response_type
 * 
 *  */
union ldl_mac_response_arg {

    /** #LDL_MAC_DOWNSTREAM argument */
    struct {
        
        int16_t rssi;   /**< rssi of frame */
        int16_t snr;    /**< snr of frame */
        uint8_t size;   /**< size of frame */
        
    } downstream;
    
    /** #LDL_MAC_RX argument */
    struct {
        
        const uint8_t *data;    /**< message data */
        uint16_t counter;       /**< frame counter */  
        uint8_t port;           /**< lorawan application port */  
        uint8_t size;           /**< size of message */
        
    } rx;     
    
    /** #LDL_MAC_LINK_STATUS argument */
    struct {
        
        int8_t margin;      /**< SNR margin */                     
        uint8_t gwCount;    /**< number of gateways in range */
        
    } link_status;
    
    /** #LDL_MAC_RX1_SLOT and #LDL_MAC_RX2_SLOT argument */
    struct {
    
        uint32_t margin;                /**< allowed error margin */
        uint32_t error;                 /**< ticks passed since scheduled event */
        uint32_t freq;                  /**< frequency */
        enum ldl_signal_bandwidth bw;  /**< bandwidth */
        enum ldl_spreading_factor sf;  /**< spreading factor */
        uint8_t timeout;                /**< symbol timeout */
        
    } rx_slot;    
    
    /** #LDL_MAC_TX_BEGIN argument */
    struct {
        
        uint32_t freq;                      /**< frequency */    
        enum ldl_spreading_factor sf;      /**< spreading factor */
        enum ldl_signal_bandwidth bw;      /**< bandwidth */
        uint8_t power;                      /**< LoRaWAN power setting @warning this is not dBm */
        uint8_t size;                       /**< message size */
        
    } tx_begin;
    
    /** #LDL_MAC_STARTUP argument */
    struct {
        
        unsigned int entropy;               /**< srand seed from radio driver */
        
    } startup;
    
    /** #LDL_MAC_SESSION_UPDATED argument */
    struct { 
        
        const struct ldl_mac_session *session;
        
    } session_updated;
    
    /** #LDL_MAC_DEVICE_TIME argument */
    struct {
        
        uint32_t seconds;
        uint8_t fractions;
        
    } device_time;
    
    /** #LDL_MAC_JOIN_COMPLETE argument */
    struct {
        
        /** the most up to date joinNonce */
        uint32_t joinNonce;
        /** the next devNonce to use in OTAA */
        uint16_t nextDevNonce;
        /** netID */
        uint32_t netID;
        /** devAddr */
        uint32_t devAddr;
        
    } join_complete;
};

/** LDL calls this function pointer to notify application of events
 * 
 * @param[in] app   app from LDL_MAC_init()
 * @param[in] type  #ldl_mac_response_type
 * @param[in] arg   **OPTIONAL** depending on #ldl_mac_response_type
 * 
 * */
typedef void (*ldl_mac_response_fn)(void *app, enum ldl_mac_response_type type, const union ldl_mac_response_arg *arg);

/** MAC state */
enum ldl_mac_state {

    LDL_STATE_INIT,            /**< stack has been memset to zero */

    LDL_STATE_INIT_RESET,      /**< holding reset after _INIT */
    LDL_STATE_INIT_LOCKOUT,    /**< waiting after _RESET */
    
    LDL_STATE_RECOVERY_RESET,   /**< holding reset after chip error */
    LDL_STATE_RECOVERY_LOCKOUT, /**< waiting after chip error reset */

    LDL_STATE_ENTROPY,         /**< sample entropy */

    LDL_STATE_IDLE,        /**< ready for operations */
    
    LDL_STATE_WAIT_TX,     /**< waiting for channel to become available */
    LDL_STATE_TX,          /**< radio is TX */
    LDL_STATE_WAIT_RX1,    /**< waiting for first RX window */
    LDL_STATE_RX1,         /**< first RX window */
    LDL_STATE_WAIT_RX2,    /**< waiting for second RX window */
    LDL_STATE_RX2,         /**< second RX window */    
    
    LDL_STATE_RX2_LOCKOUT, /**< used to ensure an out of range RX2 window is not clobbered */
    
    LDL_STATE_WAIT_RETRY,  /**< wait to retransmit / retry */
    LDL_STATE_WAIT_SEND
    
};

/** MAC operations */
enum ldl_mac_operation {
  
    LDL_OP_NONE,                   /**< no active operation */
    LDL_OP_JOINING,                /**< MAC is performing a join */
    LDL_OP_REJOINING,              /**< MAC is performing a rejoin */
    LDL_OP_DATA_UNCONFIRMED,       /**< MAC is sending unconfirmed data */
    LDL_OP_DATA_CONFIRMED,         /**< MAC is sending confirmed data */    
    LDL_OP_RESET,                  /**< MAC is performing radio reset */
};

/** MAC error modes */
enum ldl_mac_errno {
    
    LDL_ERRNO_NONE,
    LDL_ERRNO_NOCHANNEL,   /**< upstream channel not available */
    LDL_ERRNO_SIZE,        /**< message too large to send */
    LDL_ERRNO_RATE,        /**< data rate setting not valid for region */
    LDL_ERRNO_PORT,        /**< port not valid for upstream message */
    LDL_ERRNO_BUSY,        /**< stack is busy; cannot process request */
    LDL_ERRNO_NOTJOINED,   /**< stack is not joined; cannot process request */
    LDL_ERRNO_POWER,       /**< power setting not valid for region */
    LDL_ERRNO_INTERNAL     /**< implementation fault */
};

/* band array indices */
enum ldl_band_index {
    
    LDL_BAND_1,
    LDL_BAND_2,
    LDL_BAND_3,
    LDL_BAND_4,
    LDL_BAND_5,
    LDL_BAND_GLOBAL,
    LDL_BAND_RETRY,
    LDL_BAND_MAX
};

enum ldl_timer_inst {
    
    LDL_TIMER_WAITA,
    LDL_TIMER_WAITB,
    LDL_TIMER_BAND,
    LDL_TIMER_MAX
};

struct ldl_timer {
    
    uint32_t time;    
    bool armed;
};

enum ldl_input_type {
  
    LDL_INPUT_TX_COMPLETE,
    LDL_INPUT_RX_READY,
    LDL_INPUT_RX_TIMEOUT
};

struct ldl_input {
    
    uint8_t armed;
    uint8_t state;
    uint32_t time;
};

struct ldl_mac_channel {
    
    uint32_t freqAndRate;
#ifndef LDL_DISABLE_CMD_DL_CHANNEL    
    uint32_t dlFreq;
#endif    
};

/** session cache */
struct ldl_mac_session {
    
    /* frame counters */
    uint32_t up;
    uint16_t appDown;
    uint16_t nwkDown;
    
    uint32_t devAddr;    
    uint32_t netID;

#ifdef LDL_DISABLE_FULL_CHANNEL_CONFIG    
    struct ldl_mac_channel chConfig[8U];
#else
    struct ldl_mac_channel chConfig[16U];
#endif    
    
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
    
    uint8_t version;
    
    uint16_t adr_ack_limit;
    uint16_t adr_ack_delay;
};

/** data service invocation options */
struct ldl_mac_data_opts {
    
    uint8_t nbTrans;        /**< redundancy (0..LDL_REDUNDANCY_MAX) */    
    bool check;             /**< piggy-back a LinkCheckReq */
    bool getTime;           /**< piggy-back a DeviceTimeReq */
    uint8_t dither;         /**< seconds of dither to add to the transmit schedule (0..60) */
};

/** MAC layer data */
struct ldl_mac {

    enum ldl_mac_state state;
    enum ldl_mac_operation op;
    enum ldl_mac_errno errno;
    
    uint8_t joinEUI[8U];
    uint8_t devEUI[8U];
    
#ifdef LDL_ENABLE_STATIC_RX_BUFFER
    uint8_t rx_buffer[LDL_MAX_PACKET];
#endif    
    uint8_t buffer[LDL_MAX_PACKET];
    uint8_t bufferLen;
    
    /* off-time in ms per band */    
    uint32_t band[LDL_BAND_MAX];
    
    uint32_t polled_band_ticks;
    
    uint16_t devNonce;
    uint32_t joinNonce;
    
    int16_t margin;

    /* time of the last valid downlink in seconds */
    uint32_t last_valid_downlink;
    
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
    
    struct ldl_mac_session ctx;
    
    struct ldl_sm *sm;
    struct ldl_radio *radio;
    struct ldl_input inputs;
    struct ldl_timer timers[LDL_TIMER_MAX];
    
    enum ldl_region region;
    
    ldl_mac_response_fn handler;
    void *app;
    
    bool linkCheckReq_pending;
    bool rxParamSetupAns_pending;    
    bool dlChannelAns_pending;
    bool rxtimingSetupAns_pending;
    bool rekeyConf_pending;
    
    uint8_t adrAckCounter;
    bool adrAckReq;
    
    uint32_t time;
    uint32_t polled_time_ticks;
    
    uint32_t service_start_time;
    
    /* number of join/data trials */
    uint32_t trials;
  
    /* options and overrides applicable to current data service */
    struct ldl_mac_data_opts opts;
    
    /* added to the power setting given to radio to compensate 
     * for gains/losses */
    int16_t gain;
};

/** passed as an argument to LDL_MAC_init() 
 * 
 * */
struct ldl_mac_init_arg {
    
    /** pointer passed to #ldl_mac_response_fn and @ref ldl_system functions */
    void *app;      
    
    /** pointer to initialised Radio */
    struct ldl_radio *radio;
    
    /** pointer to initialised Security Module */
    struct ldl_sm *sm;
    
    /** application callback #ldl_mac_response_fn */
    ldl_mac_response_fn handler;
    
    /** optional pointer to restored #ldl_mac_session
     * 
     * If this pointer is NULL, LDL will initialise session to default
     * values.
     * 
     * If session keys could not be recovered, the application must
     * set this pointer to NULL.
     * 
     *  */
    const struct ldl_mac_session *session;    
    
    /** pointer to 8 byte identifier */
    const void *joinEUI;
    
    /** pointer to 8 byte identifier */
    const void *devEUI;
    
    /** the next devNonce to use in OTAA 
     * 
     * @see #LDL_MAC_JOIN_COMPLETE
     * 
     * */
    uint16_t devNonce;
    
    /** the most up to date joinNonce
     * 
     * @see #LDL_MAC_JOIN_COMPLETE
     * 
     * */
    uint32_t joinNonce;
    
    /** dBm to add to the power setting given to the 
     * radio to compensate for gains/losses
     * 
     * If in doubt set to 0
     * 
     * */
    int16_t gain;
};


/** Initialise #ldl_mac 
 * 
 * @param[in] self      #ldl_mac
 * @param[in] region    #ldl_region
 * @param[in] arg       #ldl_mac_init_arg
 * 
 * Many important parameters and references are injected
 * into #ldl_mac via arg. These are immutable
 * for the lifetime of #ldl_mac:
 * 
 * - ldl_mac_init_arg.radio     pointer to initialised Radio
 * - ldl_mac_init_arg.handler   application callback/handler
 * - ldl_mac_init_arg.app       application specific pointer
 * - ldl_mac_init_arg.sm        pointer to initialised Security Module
 * - ldl_mac_init_arg.joinEUI   pointer to 16 byte identifier
 * - ldl_mac_init_arg.devEUI    pointer to 16 byte identifier
 * - ldl_mac_init_arg.devNonce  the next devNonce to use in OTAA
 * - ldl_mac_init_arg.joinNonce the next joinNonce to use in OTAA
 * - ldl_mac_init_arg.session   optional pointer to restored session state
 * - ldl_mac_init_arg.gain      gain compensation
 * 
 * More members may be added in future releases and so it is 
 * recommended to clear #ldl_mac_init_arg before using. This will ensure
 * not-null assertions in LDL_MAC_init() fail.
 * 
 * For example:
 * 
 * @code{.c}
 * struct ldl_mac_init_arg arg = {0};
 * 
 * arg.radio = &radio;
 * arg.handler = my_handler;
 * arg.app = NULL;
 * arg.sm = &sm;
 * arg.joinEUI = joinEUI;
 * arg.devEUI = devEUI;
 * 
 * LDL_MAC_init(&mac, LDL_EU_863_870, &arg);
 * @endcode
 * 
 * */
void LDL_MAC_init(struct ldl_mac *self, enum ldl_region region, const struct ldl_mac_init_arg *arg);

/** Send data without confirmation
 * 
 * Once initiated MAC will send at most nbTrans times until a valid downlink is received. NbTrans may be set:
 * 
 * - globally by the network (via LinkADRReq)
 * - globally by the application (via LDL_MAC_setNbTrans())
 * - per invocation by #ldl_mac_data_opts
 *
 * The application can cancel the service while it is in progress by calling LDL_MAC_cancel().
 * 
 * #ldl_mac_response_fn will push #LDL_MAC_DATA_COMPLETE on completion.
 * 
 * @param[in] self  #ldl_mac
 * @param[in] port  lorawan port (must be >0)
 * @param[in] data  pointer to message to send
 * @param[in] len   byte length of data
 * @param[in] opts  #ldl_mac_data_opts (may be NULL)
 * 
 * @retval true     pending
 * @retval false    error
 * 
 * @see LDL_MAC_errno()
 * 
 * */
bool LDL_MAC_unconfirmedData(struct ldl_mac *self, uint8_t port, const void *data, uint8_t len, const struct ldl_mac_data_opts *opts);

/** Send data with confirmation
 * 
 * Once initiated MAC will send at most nbTrans times until a confirmation is received. NbTrans may be set:
 * 
 * - globally by the network (via LinkADRReq)
 * - globally by the application (via LDL_MAC_setNbTrans())
 * - per invocation by #ldl_mac_data_opts
 * 
 * The application can cancel the service while it is in progress by calling LDL_MAC_cancel().
 * 
 * #ldl_mac_response_fn will push #LDL_MAC_DATA_TIMEOUT on every timeout
 * #ldl_mac_response_fn will push #LDL_MAC_DATA_COMPLETE on completion
 * 
 * 
 * @param[in] self  #ldl_mac
 * @param[in] port  lorawan port (must be >0)
 * @param[in] data  pointer to message to send
 * @param[in] len   byte length of data
 * @param[in] opts  #ldl_mac_data_opts (may be NULL)
 * 
 * @retval true     pending
 * @retval false    error
 * 
 * @see LDL_MAC_errno()
 * 
 * */
bool LDL_MAC_confirmedData(struct ldl_mac *self, uint8_t port, const void *data, uint8_t len, const struct ldl_mac_data_opts *opts);

/** Initiate Over The Air Activation
 * 
 * Once initiated MAC will keep trying to join forever.
 * 
 * - Application can cancel by calling LDL_MAC_cancel()
 * - #ldl_mac_response_fn will push #LDL_MAC_JOIN_TIMEOUT on every timeout
 * - #ldl_mac_response_fn will push #LDL_MAC_JOIN_COMPLETE on completion
 * 
 * @param[in] self  #ldl_mac
 * 
 * @retval true     pending
 * @retval false    error
 * 
 * @see LDL_MAC_errno()
 * 
 * */
bool LDL_MAC_otaa(struct ldl_mac *self);

/** Forget network
 * 
 * @param[in] self  #ldl_mac
 * 
 * */
void LDL_MAC_forget(struct ldl_mac *self);

/** Return state to #LDL_STATE_IDLE
 * 
 * @note has no immediate effect if MAC is already in the process of resetting
 * the radio
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
 * Aside from the passage of time, calls to the following functions may 
 * cause the return value of this function to be change:
 * 
 * - LDL_MAC_process()
 * - LDL_MAC_otaa()
 * - LDL_MAC_unconfirmedData()
 * - LDL_MAC_confirmedData()
 * - LDL_Radio_interrupt()
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

/** Set the transmit data rate
 * 
 * @param[in] self  #ldl_mac
 * @param[in] rate
 * 
 * @retval true     applied
 * @retval false    error 
 * 
 * @see LDL_MAC_errno()
 * 
 * */
bool LDL_MAC_setRate(struct ldl_mac *self, uint8_t rate);

/** Get the current transmit data rate
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
 * @retval true     applied
 * @retval false    error
 * 
 * @see LDL_MAC_errno()
 * 
 * */
bool LDL_MAC_setPower(struct ldl_mac *self, uint8_t power);

/** Get the current transmit power
 * 
 * @param[in] self  #ldl_mac
 * 
 * @return transmit power setting
 * 
 * */
uint8_t LDL_MAC_getPower(const struct ldl_mac *self);

/** Enable ADR mode
 * 
 * @param[in] self  #ldl_mac
 * 
 * */
void LDL_MAC_enableADR(struct ldl_mac *self);

/** Disable ADR mode
 * 
 * @param[in] self  #ldl_mac
 * 
 * */
void LDL_MAC_disableADR(struct ldl_mac *self);

/** Is ADR mode enabled?
 * 
 * @param[in] self  #ldl_mac
 * 
 * @retval true     enabled
 * @retval false    not enabled
 * 
 * */
bool LDL_MAC_adr(const struct ldl_mac *self);

/** Read the last error
 * 
 * The following functions will set the errno when they fail:
 * 
 * - LDL_MAC_otaa()
 * - LDL_MAC_unconfirmedData()
 * - LDL_MAC_confirmedData()
 * - LDL_MAC_setRate()
 * - LDL_MAC_setPower()
 * 
 * @param[in] self  #ldl_mac
 * 
 * @return #ldl_mac_errno
 * 
 * */
enum ldl_mac_errno LDL_MAC_errno(const struct ldl_mac *self);

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

/** Calculate transmit time of message sent from node
 * 
 * @param[in] bw    bandwidth
 * @param[in] sf    spreading factor
 * @param[in] size  size of message
 * 
 * @return system ticks
 * 
 * */
uint32_t LDL_MAC_transmitTimeUp(enum ldl_signal_bandwidth bw, enum ldl_spreading_factor sf, uint8_t size);

/** Calculate transmit time of message sent from gateway
 * 
 * @param[in] bw    bandwidth
 * @param[in] sf    spreading factor
 * @param[in] size  size of message
 * 
 * @return system ticks
 * 
 * */
uint32_t LDL_MAC_transmitTimeDown(enum ldl_signal_bandwidth bw, enum ldl_spreading_factor sf, uint8_t size);

/** Convert bandwidth enumeration to Hz
 * 
 * @param[in] bw bandwidth
 * @return Hz
 * 
 * */
uint32_t LDL_MAC_bwToNumber(enum ldl_signal_bandwidth bw);



/** Get the maximum transfer unit in bytes
 * 
 * This number changes depending on:
 * 
 * - region
 * - rate
 * - pending mac commands
 * 
 * @param[in] self  #ldl_mac
 * @retval mtu
 * 
 * */
uint8_t LDL_MAC_mtu(const struct ldl_mac *self);

/** Seconds since last valid downlink message
 * 
 * A valid downlink is one that is:
 * 
 * - expected type for current operation
 * - well formed
 * - able to be decrypted
 * 
 * @param[in] self  #ldl_mac
 * @return seconds since last valid downlink
 * 
 * @retval UINT32_MAX no valid downlink received
 * 
 * */
uint32_t LDL_MAC_timeSinceValidDownlink(struct ldl_mac *self);

/** Set the aggregated duty cycle limit
 * 
 * duty cycle limit = 1 / (2 ^ limit)
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

/** Set transmission redundancy
 *
 * - confirmed and unconfirmed uplink frames are sent nbTrans times (or until acknowledgement is received)
 * - a value of zero will leave the setting unchanged
 * - limited to 15 or LDL_REDUNDANCY_MAX (whichever is lower)
 * 
 * @param[in] self  #ldl_mac
 * @param[in] nbTrans
 * 
 * @see LoRaWAN Specification: LinkADRReq.Redundancy.NbTrans
 * 
 * */
void LDL_MAC_setNbTrans(struct ldl_mac *self, uint8_t nbTrans);

/** Get transmission redundancy
 * 
 * @param[in] self  #ldl_mac
 * 
 * @return nbTrans
 * 
 * @see LoRaWAN Specification: LinkADRReq.Redundancy.NbTrans
 * 
 * */
uint8_t LDL_MAC_getNbTrans(const struct ldl_mac *self);

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


/* for internal use only */
void LDL_MAC_radioEvent(struct ldl_mac *self, enum ldl_radio_event event);
bool LDL_MAC_addChannel(struct ldl_mac *self, uint8_t chIndex, uint32_t freq, uint8_t minRate, uint8_t maxRate);
void LDL_MAC_removeChannel(struct ldl_mac *self, uint8_t chIndex);
bool LDL_MAC_maskChannel(struct ldl_mac *self, uint8_t chIndex);
bool LDL_MAC_unmaskChannel(struct ldl_mac *self, uint8_t chIndex);

void LDL_MAC_timerSet(struct ldl_mac *self, enum ldl_timer_inst timer, uint32_t timeout);
bool LDL_MAC_timerCheck(struct ldl_mac *self, enum ldl_timer_inst timer, uint32_t *error);
void LDL_MAC_timerClear(struct ldl_mac *self, enum ldl_timer_inst timer);
uint32_t LDL_MAC_timerTicksUntilNext(const struct ldl_mac *self);
uint32_t LDL_MAC_timerTicksUntil(const struct ldl_mac *self, enum ldl_timer_inst timer, uint32_t *error);

void LDL_MAC_inputArm(struct ldl_mac *self, enum ldl_input_type type);
bool LDL_MAC_inputCheck(const struct ldl_mac *self, enum ldl_input_type type, uint32_t *error);
void LDL_MAC_inputClear(struct ldl_mac *self);
void LDL_MAC_inputSignal(struct ldl_mac *self, enum ldl_input_type type);
bool LDL_MAC_inputPending(const struct ldl_mac *self);

#ifdef __cplusplus
}
#endif

/** @} */
#endif
