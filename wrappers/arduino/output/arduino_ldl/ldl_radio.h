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

#ifndef LDL_RADIO_H
#define LDL_RADIO_H

/** @file */

/**
 * @defgroup ldl_radio Radio
 * @ingroup ldl
 * 
 * # Radio Driver Interface
 * 
 * The application must use the following interfaces to initialise the radio
 * driver before calling LDL_MAC_init() :
 * 
 * - LDL_Radio_init()
 * - LDL_Radio_setPA()
 * 
 * There are more radio interfaces than those documented in this group
 * but the application does not need to interact with them directly.
 * 
 * @{
 * */

#ifdef __cplusplus
extern "C" {
#endif

#include "ldl_platform.h"
#include "ldl_radio_defs.h"
#include <stdint.h>
#include <stdbool.h>

enum ldl_radio_event {    
    LDL_RADIO_EVENT_TX_COMPLETE,
    LDL_RADIO_EVENT_RX_READY,
    LDL_RADIO_EVENT_RX_TIMEOUT,    
    LDL_RADIO_EVENT_NONE,
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
    uint8_t timeout;
    uint8_t max;
};

struct ldl_radio_packet_metadata {
    
    int16_t rssi;
    int8_t snr;
    enum ldl_signal_bandwidth bw;
    enum ldl_spreading_factor sf;
    uint32_t freq;
};

/** Power amplifier configuration */
enum ldl_radio_pa {
    LDL_RADIO_PA_RFO,      /**< RFO pin */
    LDL_RADIO_PA_BOOST     /**< BOOST pin */
};

struct ldl_mac;

typedef void (*ldl_radio_event_fn)(struct ldl_mac *self, enum ldl_radio_event event);

/** Radio data */
struct ldl_radio {
    
    void *board;
    enum ldl_radio_pa pa;
    uint8_t dio_mapping1;    
    enum ldl_radio_type type;
    struct ldl_mac *mac;
    ldl_radio_event_fn handler;    
};


/** Initialise radio driver
 * 
 * This must be done before calling LDL_MAC_init().
 * 
 * @param[in] self
 * @param[in] type  driver to initialise
 * @param[in] board passed to board interface functions (e.g. LDL_Chip_write())
 * 
 * */
void LDL_Radio_init(struct ldl_radio *self, enum ldl_radio_type type, void *board);

/** Select power amplifier
 * 
 * This setting applies to the following radio drivers:
 * 
 * - LDL_RADIO_SX1272
 * - LDL_RADIO_SX1276
 * 
 * These radios have different hardware connections for different
 * power amplifiers. This setting tells the driver which one is connected.
 * 
 * For example, the Semtech SX1272MB2xAS MBED shield uses the #LDL_RADIO_PA_RFO connection,
 * while the HopeRF RFM96W uses the #LDL_RADIO_PA_BOOST connection.
 * 
 * @param[in] self
 * @param[in] pa power amplifier setting
 * 
 * */
void LDL_Radio_setPA(struct ldl_radio *self, enum ldl_radio_pa pa);

/** Receive an interrupt from the chip
 * 
 * @param[in] self  #ldl_radio
 * @param[in] n     DIO number
 * 
 * @ingroup ldl_radio_connector
 * 
 * @note interrupt safe if LDL_SYSTEM_ENTER_CRITICAL() and LDL_SYSTEM_ENTER_CRITICAL() have been defined
 * 
 * */
void LDL_Radio_interrupt(struct ldl_radio *self, uint8_t n);

void LDL_Radio_setHandler(struct ldl_radio *self, struct ldl_mac *mac, ldl_radio_event_fn handler);

void LDL_Radio_entropyBegin(struct ldl_radio *self);
unsigned int LDL_Radio_entropyEnd(struct ldl_radio *self);
enum ldl_radio_event LDL_Radio_signal(struct ldl_radio *self, uint8_t n);
void LDL_Radio_reset(struct ldl_radio *self, bool state);
uint8_t LDL_Radio_collect(struct ldl_radio *self, struct ldl_radio_packet_metadata *meta, void *data, uint8_t max);
void LDL_Radio_sleep(struct ldl_radio *self);
void LDL_Radio_transmit(struct ldl_radio *self, const struct ldl_radio_tx_setting *settings, const void *data, uint8_t len);
void LDL_Radio_receive(struct ldl_radio *self, const struct ldl_radio_rx_setting *settings);
void LDL_Radio_clearInterrupt(struct ldl_radio *self);

#ifdef LDL_ENABLE_RADIO_TEST
void LDL_Radio_setFreq(struct ldl_radio *self, uint32_t freq);
void LDL_Radio_setModemConfig(struct ldl_radio *self, enum ldl_signal_bandwidth bw, enum ldl_spreading_factor sf);
void LDL_Radio_setPower(struct ldl_radio *self, int16_t dbm);
void LDL_Radio_enableLora(struct ldl_radio *self);
#endif


#ifdef __cplusplus
}
#endif

/** @} */
#endif
