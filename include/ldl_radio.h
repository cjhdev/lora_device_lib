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
 *
 * The application should initialise the radio with @ref ldl_mac in the following
 * order:
 *
 * -# LDL_SX1272_init()/LDL_SX1276_init()/LDL_SX1261_init()/LDL_SX1262_init()/LDL_WL55_init()
 * -# LDL_MAC_init()
 * -# LDL_Radio_setEventCallback()
 *
 * @ref ldl_mac calls all non-static (i.e. functions that pass self as first
 * argument) via #ldl_radio_interface. This means it is possible
 * to wrap @ref ldl_radio in a C++ if necessary.
 *
 * Be careful using these interfaces for other projects. The driver expects
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
#include "ldl_chip.h"
#include <stdint.h>
#include <stdbool.h>

/** Radio driver type
 *
 * Drivers are included in the build by defining:
 *
 * - #LDL_ENABLE_SX1272
 * - #LDL_ENABLE_SX1276
 * - #LDL_ENABLE_SX1261
 * - #LDL_ENABLE_SX1262
 * - #LDL_ENABLE_WL55
 *
 * */
enum ldl_radio_type {
    LDL_RADIO_NONE,        /**< no radio */
    LDL_RADIO_SX1272,      /**< SX1272 */
    LDL_RADIO_SX1276,      /**< SX1276 */
    LDL_RADIO_SX1261,      /**< SX126X with only LP PA */
    LDL_RADIO_SX1262,      /**< SX126X with only HP PA */
    LDL_RADIO_WL55         /**< integrated SX126X with HP and LP PA */
};

/** This controls the radio
 *
 * */
enum ldl_radio_mode {

    LDL_RADIO_MODE_RESET,       /**< assert reset line */
    LDL_RADIO_MODE_BOOT,        /**< de-assert reset line */
    LDL_RADIO_MODE_SLEEP,       /**< cold sleep */
    LDL_RADIO_MODE_RX,          /**< ready for rx */
    LDL_RADIO_MODE_TX,          /**< ready for tx */
    LDL_RADIO_MODE_HOLD         /**< between tx and rx */
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
    int16_t eirp;
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

struct ldl_radio_status {

    bool tx;
    bool rx;
    bool timeout;
};

/** SX127X power amplifier setting */
enum ldl_sx127x_pa {
    LDL_SX127X_PA_AUTO,  /**< driver decides which PA to use */
    LDL_SX127X_PA_RFO,   /**< use RFO */
    LDL_SX127X_PA_BOOST  /**< use BOOST */
};

/** SX126X power amplifier setting */
enum ldl_sx126x_pa {
    LDL_SX126X_PA_AUTO, /**< driver decides which PA to use */
    LDL_SX126X_PA_LP,   /**< use low power */
    LDL_SX126X_PA_HP    /**< use high power */
};

/** SX126X regulator setting
 *
 * */
enum ldl_sx126x_regulator {

    LDL_SX126X_REGULATOR_LDO,
    LDL_SX126X_REGULATOR_DCDC
};

/* SX126X dio3 voltage output setting */
enum ldl_sx126x_voltage {

    LDL_SX126X_VOLTAGE_1V6,
    LDL_SX126X_VOLTAGE_1V7,
    LDL_SX126X_VOLTAGE_1V8,
    LDL_SX126X_VOLTAGE_2V2,
    LDL_SX126X_VOLTAGE_2V4,
    LDL_SX126X_VOLTAGE_2V7,
    LDL_SX126X_VOLTAGE_3V0,
    LDL_SX126X_VOLTAGE_3V3
};

/* SX126X dio2 txen mode setting */
enum ldl_sx126x_txen {

    LDL_SX126X_TXEN_DISABLED,
    LDL_SX126X_TXEN_ENABLED
};

struct ldl_mac;

typedef void (*ldl_radio_event_fn)(struct ldl_mac *self);

struct ldl_sx127x_debug_log_entry {

    uint8_t opcode;
    uint8_t value;
    uint8_t size;
};

#ifndef LDL_RADIO_DEBUG_LOG_SIZE
#define LDL_RADIO_DEBUG_LOG_SIZE 20
#endif

struct ldl_sx127x_debug_log {

    struct ldl_sx127x_debug_log_entry log[LDL_RADIO_DEBUG_LOG_SIZE];
    uint8_t pos;
    bool overflow;
};

/** Radio state */
struct ldl_radio {

    enum ldl_radio_type type;
    enum ldl_radio_mode mode;
    enum ldl_radio_xtal xtal;

    void *chip;
    ldl_chip_write_fn chip_write;
    ldl_chip_read_fn chip_read;
    ldl_chip_set_mode_fn chip_set_mode;

    struct ldl_mac *cb_ctx;
    ldl_radio_event_fn cb;

    union {

#if defined(LDL_ENABLE_SX1272) || defined(LDL_ENABLE_SX1276)
        struct {
            enum ldl_sx127x_pa pa;
#ifdef LDL_ENABLE_RADIO_DEBUG
            struct ldl_sx127x_debug_log debug;
#endif
        } sx127x;
#endif

#if defined(LDL_ENABLE_SX1261) || defined(LDL_ENABLE_SX1262) || defined(LDL_ENABLE_WL55)
        struct {

            enum ldl_sx126x_pa pa;
            enum ldl_sx126x_regulator regulator;
            enum ldl_sx126x_voltage voltage;
            enum ldl_sx126x_txen txen;
            bool image_calibration_pending;
            bool trim_xtal;
            uint8_t xta;
            uint8_t xtb;

        } sx126x;
#endif
        /* make sure this still compiles if radios are not enabled*/
        int none;

    } state;

    int16_t tx_gain;
};

/** @ref ldl_mac calls non-static radio functions through these function pointers
 *
 * The self pointer passed to each function will be #ldl_mac_init_arg.radio
 *
 *  */
struct ldl_radio_interface {

    /** Change the mode
     *
     * @param[in] self
     * @param[in] mode
     *
     * */
    void (*set_mode)(struct ldl_radio *self, enum ldl_radio_mode mode);

    /** Read entropy data from radio
     *
     * @warning ldl_radio.mode must be LDL_RADIO_MODE_STANDBY
     *
     * @param[in] self #ldl_radio
     *
     * @return entropy
     *
     * */
    uint32_t (*read_entropy)(struct ldl_radio *self);

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
    uint8_t (*read_buffer) (struct ldl_radio *self, struct ldl_radio_packet_metadata *meta, void *data, uint8_t max);

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
    void (*transmit)(struct ldl_radio *self, const struct ldl_radio_tx_setting *settings, const void *data, uint8_t len);

    /** Configure radio to receive
     *
     * @warning ldl_radio.mode must be LDL_RADIO_MODE_STANDBY
     *
     * @param[in] self
     * @param[in] settings
     *
     * */
    void (*receive)(struct ldl_radio *self, const struct ldl_radio_rx_setting *settings);

    /** Configure radio to receive entropy
     *
     * @warning ldl_radio.mode must be LDL_RADIO_MODE_STANDBY
     *
     * @param[in] self
     *
     * */
    void (*receive_entropy)(struct ldl_radio *self);


    /** Read status from radio
     *
     * @param[in] self
     * @param[out] status
     *
     * */
    void (*get_status)(struct ldl_radio *self, struct ldl_radio_status *status);

};

/** Get interface for initialised radio driver
 *
 * */
const struct ldl_radio_interface *LDL_Radio_getInterface(const struct ldl_radio *self);

/** Receive an interrupt from the chip
 *
 * @param[in] self  #ldl_radio
 * @param[in] n     DIO number
 *
 * @ingroup ldl_chip_interface
 *
 * This function will typically be called from an ISR like this:
 *
 * @code{.c}
 * extern ldl_radio radio;
 *
 * void dio0_rising_edge_isr(void)
 * {
 *   LDL_Radio_handleInterrupt(&radio, 0);
 * }
 * void dio1_rising_edge_isr(void)
 * {
 *   LDL_Radio_handleInterrupt(&radio, 1);
 * }
 * @endcode
 *
 * */
void LDL_Radio_handleInterrupt(struct ldl_radio *self, uint8_t n);

/** Set event handler callback
 *
 * This is how the Radio tells the MAC about rising interrupt lines.
 *
 * @param[in] self      #ldl_radio
 * @param[in] ctx       passed back as first argument of handler
 * @param[in] cb        LDL_MAC_radioEvent() or an intermediate
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

/** Convert bandwidth enumeration to Hz
 *
 * @param[in] bw bandwidth
 * @return Hz
 *
 * */
uint32_t LDL_Radio_bwToNumber(enum ldl_signal_bandwidth bw);

#include "ldl_sx126x.h"
#include "ldl_sx127x.h"

#ifdef __cplusplus
}
#endif

/** @} */
#endif
