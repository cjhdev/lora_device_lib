/* Copyright (c) 2019-2020 Cameron Harper
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

#ifndef LDL_RADIO_H
#define LDL_RADIO_H

/** @file */

/**
 * @defgroup ldl_radio Radio
 * @ingroup ldl
 *
 * # Radio Driver Interface
 *
 * The application must use the following interfaces to setup LDL:
 *
 * - LDL_Radio_init()
 * - LDL_Radio_setEventCallback() (how @ref ldl_radio passes events to @ref ldl_mac)
 *
 * Sequence of initialisation is as follows:
 *
 * -# LDL_Radio_init()
 * -# LDL_MAC_init()
 * -# LDL_Radio_setEventCallback()
 *
 * All other interfaces do not need to be called by the application.
 *
 * @ref ldl_mac calls all non-static (i.e. functions that pass self as first
 * argument) via #ldl_radio_interface. This means it is possible
 * to wrap @ref ldl_radio in a C++ if necessary.
 *
 * Be careful using these interfaces for other projects (i.e. having
 * something other than @ref ldl_mac call them). The driver expects
 * certain functions to be called before others. For best results you
 * should study how @ref ldl_mac calls the interfaces to figure out
 * what will work.
 *
 * @{
 * */

#ifdef __cplusplus
extern "C" {
#endif

#include "ldl_platform.h"
#include "ldl_radio_defs.h"
#include "ldl_radio_debug.h"
#include "ldl_chip.h"
#include <stdint.h>
#include <stdbool.h>

/** radio events sent to MAC */
enum ldl_radio_event {
    LDL_RADIO_EVENT_TX_COMPLETE,
    LDL_RADIO_EVENT_RX_READY,
    LDL_RADIO_EVENT_RX_TIMEOUT,
    LDL_RADIO_EVENT_NONE
};

/** Radio driver type
 *
 * The driver selected at LDL_Radio_init() must have first been included
 * in the build.
 *
 * Drivers are included in the build by defining:
 *
 * - #LDL_ENABLE_SX1272
 * - #LDL_ENABLE_SX1276
 *
 * */
enum ldl_radio_type {
#ifdef LDL_ENABLE_SX1272
    LDL_RADIO_SX1272,      /**< SX1272 */
#endif
#ifdef LDL_ENABLE_SX1276
    LDL_RADIO_SX1276,      /**< SX1276 */
#endif
    LDL_RADIO_NONE         /**< no radio */
};

/** the radio is considered to be in one of these states
 * at all times */
enum ldl_radio_mode {

    LDL_RADIO_MODE_RESET,       /** reset line is asserted */
    LDL_RADIO_MODE_SLEEP,       /** xtal is off */
    LDL_RADIO_MODE_STANDBY      /** xtal is on */
};

/** oscillator type */
enum ldl_radio_xtal {

    LDL_RADIO_XTAL_CRYSTAL,     /**< built-in crystal oscillator */
    LDL_RADIO_XTAL_TCXO         /**< external temperature compensated */
};

struct ldl_radio_tx_setting {

    uint32_t freq;
    enum ldl_signal_bandwidth bw;
    enum ldl_spreading_factor sf;
    int16_t dbm;
};

struct ldl_radio_rx_setting {

    bool continuous;
    uint32_t freq;
    enum ldl_signal_bandwidth bw;
    enum ldl_spreading_factor sf;
    uint16_t timeout;
    uint8_t max;
};

struct ldl_radio_packet_metadata {

    int16_t rssi;
    int16_t snr;
};

/** Power amplifier configuration */
enum ldl_radio_pa {
    LDL_RADIO_PA_AUTO,  /**< driver decides which PA to use */
    LDL_RADIO_PA_RFO,   /**< use RFO */
    LDL_RADIO_PA_BOOST  /**< use BOOST */
};

struct ldl_mac;

typedef void (*ldl_radio_event_fn)(struct ldl_mac *self, enum ldl_radio_event event);

/** Radio state */
struct ldl_radio {

    void *chip;
    enum ldl_radio_pa pa;
    enum ldl_radio_type type;
    struct ldl_mac *cb_ctx;
    ldl_radio_event_fn cb;
    ldl_chip_write_fn chip_write;
    ldl_chip_read_fn chip_read;
    ldl_chip_set_mode_fn chip_set_mode;
    enum ldl_radio_xtal xtal;
    enum ldl_radio_mode mode;
    int16_t tx_gain;
    uint8_t xtal_delay;
    volatile uint8_t dio_mapping1;

#ifdef LDL_ENABLE_RADIO_DEBUG
    struct ldl_radio_debug_log debug;
#endif
};

/** @ref ldl_mac calls non-static radio functions through these function pointers
 *
 * The self pointer passed to each function will be #ldl_mac_init_arg.radio
 *
 *  */
struct ldl_radio_interface {

    void (*set_mode)(struct ldl_radio *self, enum ldl_radio_mode mode); /**< LDL_Radio_setMode() */
    uint32_t (*read_entropy)(struct ldl_radio *self);               /**< LDL_Radio_readEntropy() */
    uint8_t (*read_buffer) (struct ldl_radio *self, struct ldl_radio_packet_metadata *meta, void *data, uint8_t max);       /**< LDL_Radio_readBuffer() */
    void (*transmit)(struct ldl_radio *self, const struct ldl_radio_tx_setting *settings, const void *data, uint8_t len);   /**< LDL_Radio_transmit() */
    void (*receive)(struct ldl_radio *self, const struct ldl_radio_rx_setting *settings);                                   /**< LDL_Radio_receive() */
    uint8_t (*get_xtal_delay)(struct ldl_radio *self);     /**< LDL_Radio_getXTALDelay() */
    void (*receive_entropy)(struct ldl_radio *self);        /**< LDL_Radio_receiveEntropy() */
};

/** passed to LDL_Radio_init */
struct ldl_radio_init_arg {

    /** transceiver type */
    enum ldl_radio_type type;

    /** type of XTAL */
    enum ldl_radio_xtal xtal;

    /** different XTAL types may have a significant startup delay
     *
     * This delay is measured in milliseconds
     *
     * */
    uint8_t xtal_delay;

    /** SX1272/6 transceivers have PAs on different physical pins
     *
     * The driver needs to know if one or both are connected.
     *
     * */
    enum ldl_radio_pa pa;

    /** Apply TX gain compensation in units of (dB x 10-2)
     *
     * (e.g. -2.4dB == -240)
     *
     * */
    int16_t tx_gain;

    /** a pointer that will be passed as self when calling ldl_chip_*
     * functions */
    void *chip;

    ldl_chip_write_fn chip_write;        /**< #ldl_chip_write_fn */
    ldl_chip_read_fn chip_read;          /**< #ldl_chip_read_fn */
    ldl_chip_set_mode_fn chip_set_mode;  /**< #ldl_chip_set_mode_fn */
};

/** default Radio interface */
extern const struct ldl_radio_interface LDL_Radio_interface;

/** Initialise radio driver
 *
 * @param[in] self
 * @param[in] arg   #ldl_radio_init_arg
 *
 * */
void LDL_Radio_init(struct ldl_radio *self, const struct ldl_radio_init_arg *arg);

/** Receive an interrupt from the chip
 *
 * @param[in] self  #ldl_radio
 * @param[in] n     DIO number
 *
 * @ingroup ldl_chip_interface
 *
 * */
void LDL_Radio_handleInterrupt(struct ldl_radio *self, uint8_t n);

/** Set event handler callback
 *
 * This is how the Radio tells the MAC about rising interrupt lines.
 *
 * @param[in] self      #ldl_radio
 * @param[in] ctx       passed back as first argument of handler
 * @param[in] handler   LDL_MAC_radioEvent() or an intermediate
 *
 * */
void LDL_Radio_setEventCallback(struct ldl_radio *self, struct ldl_mac *ctx, ldl_radio_event_fn cb);

/** Get minimum SNR for a given spreading factor
 *
 * @param[in] sf spreading factor
 * @return dbm
 *
 * */
int16_t LDL_Radio_getMinSNR(enum ldl_spreading_factor sf);

/** Change the mode
 *
 * @param[in] self
 * @param[in] mode
 *
 * */
void LDL_Radio_setMode(struct ldl_radio *self, enum ldl_radio_mode mode);

/** Configure radio to receive
 *
 * @warning ldl_radio.mode must be LDL_RADIO_MODE_STANDBY
 *
 * @param[in] self
 * @param[in] settings
 *
 * */
void LDL_Radio_receive(struct ldl_radio *self, const struct ldl_radio_rx_setting *settings);

/** Configure radio to transmit a message
 *
 * @warning ldl_radio.mode must be LDL_RADIO_MODE_STANDBY
 *
 * @param[in] self
 * @param[in] settings
 * @param[in] data
 * @param[in] len
 *
 * */
void LDL_Radio_transmit(struct ldl_radio *self, const struct ldl_radio_tx_setting *settings, const void *data, uint8_t len);

/** Read the receive buffer and meta data
 *
 * @warning ldl_radio.mode must be LDL_RADIO_MODE_STANDBY
 *
 * @param[in] self
 * @param[out] meta
 * @param[out] data     buffer
 * @param[in] max       maximum size of buffer
 *
 * @retval bytes read
 *
 * */
uint8_t LDL_Radio_readBuffer(struct ldl_radio *self, struct ldl_radio_packet_metadata *meta, void *data, uint8_t max);

/** Read entropy data from radio
 *
 * @warning ldl_radio.mode must be LDL_RADIO_MODE_STANDBY
 *
 * @param[in] self #ldl_radio
 *
 * @return entropy
 *
 * */
uint32_t LDL_Radio_readEntropy(struct ldl_radio *self);

/** Configure radio to receive entropy
 *
 * @warning ldl_radio.mode must be LDL_RADIO_MODE_STANDBY
 *
 * @param[in] self
 *
 * */
void LDL_Radio_receiveEntropy(struct ldl_radio *self);

/** Time taken to transmit a message of certain size
 *
 * @param[in] bw
 * @param[in] sf
 * @param[in] size
 * @param[in] crc
 *
 * @retval milliseconds
 *
 * */
uint32_t LDL_Radio_getAirTime(enum ldl_signal_bandwidth bw, enum ldl_spreading_factor sf, uint8_t size, bool crc);

/** Get the time it takes to start and stabilise the XTAL
 *
 * @param[in] self
 *
 * @return milliseconds
 *
 * */
uint8_t LDL_Radio_getXTALDelay(struct ldl_radio *self);

/** Convert bandwidth enumeration to Hz
 *
 * @param[in] bw bandwidth
 * @return Hz
 *
 * */
uint32_t LDL_Radio_bwToNumber(enum ldl_signal_bandwidth bw);

/** Convert type enum to a string
 *
 * @param[in] type
 * @return string
 *
 * */
const char *LDL_Radio_typeToString(enum ldl_radio_type type);

/** Convert mode enum to a string
 *
 * @param[in] mode
 * @return string
 *
 * */
const char *LDL_Radio_modeToString(enum ldl_radio_mode mode);

/** Convert xtal enum to a string
 *
 * @param[in] xtal
 * @return string
 *
 * */
const char *LDL_Radio_xtalToString(enum ldl_radio_xtal xtal);

/** Convert pa enum to a string
 *
 * @param[in] pa
 * @return string
 *
 * */
const char *LDL_Radio_paToString(enum ldl_radio_pa pa);

#ifdef __cplusplus
}
#endif

/** @} */
#endif
