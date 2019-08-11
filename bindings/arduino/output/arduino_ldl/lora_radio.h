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

#ifndef LORA_RADIO_H
#define LORA_RADIO_H

/** @file */

/**
 * @defgroup ldl_radio Radio
 * @ingroup ldl
 * 
 * 
 * 
 * @{
 * */

#ifdef __cplusplus
extern "C" {
#endif

#include "lora_platform.h"
#include "lora_radio_defs.h"
#include <stdint.h>
#include <stdbool.h>

struct lora_board;

enum lora_radio_event {    
    LORA_RADIO_EVENT_TX_COMPLETE,
    LORA_RADIO_EVENT_RX_READY,
    LORA_RADIO_EVENT_RX_TIMEOUT,    
    LORA_RADIO_EVENT_NONE,
};

/** radio type */
enum lora_radio_type {
#ifdef LORA_ENABLE_SX1272    
    LORA_RADIO_SX1272,      /**< SX1272 */
#endif    
#ifdef LORA_ENABLE_SX1276   
    LORA_RADIO_SX1276,      /**< SX1272 (RFM95W) */
#endif    
    LORA_RADIO_NONE         /**< no radio */
};

struct lora_radio_tx_setting {
    
    uint32_t freq;
    enum lora_signal_bandwidth bw;
    enum lora_spreading_factor sf;
    int16_t dbm;        
};

struct lora_radio_rx_setting {
    
    bool continuous;
    uint32_t freq;                  
    enum lora_signal_bandwidth bw;
    enum lora_spreading_factor sf;
    uint8_t timeout;
    uint8_t max;
};

struct lora_radio_packet_metadata {
    
    int16_t rssi;
    int8_t snr;
    enum lora_signal_bandwidth bw;
    enum lora_spreading_factor sf;
    uint32_t freq;
};

/** specify power amplifier */
enum lora_radio_pa {
    LORA_RADIO_PA_RFO,      /**< RFO pin */
    LORA_RADIO_PA_BOOST     /**< BOOST pin */
};

/** Radio data */
struct lora_radio {
    
    const struct lora_board *board;
    enum lora_radio_pa pa;
    uint8_t dio_mapping1;    
    enum lora_radio_type type;
};

/** Initialise Radio
 * 
 * @param[in] self
 * @param[in] board board specific connections see @ref ldl_board
 * 
 * */
void LDL_Radio_init(struct lora_radio *self, enum lora_radio_type type, const struct lora_board *board);

/** Which PA is connected?
 * 
 * @param[in] self
 * @param[in] pa power amplifier setting
 * 
 * */
void LDL_Radio_setPA(struct lora_radio *self, enum lora_radio_pa pa);

void LDL_Radio_entropyBegin(struct lora_radio *self);
unsigned int LDL_Radio_entropyEnd(struct lora_radio *self);
enum lora_radio_event LDL_Radio_signal(struct lora_radio *self, uint8_t n);
void LDL_Radio_reset(struct lora_radio *self, bool state);
uint8_t LDL_Radio_collect(struct lora_radio *self, struct lora_radio_packet_metadata *meta, void *data, uint8_t max);
void LDL_Radio_sleep(struct lora_radio *self);
void LDL_Radio_transmit(struct lora_radio *self, const struct lora_radio_tx_setting *settings, const void *data, uint8_t len);
void LDL_Radio_receive(struct lora_radio *self, const struct lora_radio_rx_setting *settings);
void LDL_Radio_clearInterrupt(struct lora_radio *self);

#ifdef LORA_ENABLE_RADIO_TEST
void LDL_Radio_setFreq(struct lora_radio *self, uint32_t freq);
void LDL_Radio_setModemConfig(struct lora_radio *self, enum lora_signal_bandwidth bw, enum lora_spreading_factor sf);
void LDL_Radio_setPower(struct lora_radio *self, int16_t dbm);
void LDL_Radio_enableLora(struct lora_radio *self);
#endif


#ifdef __cplusplus
}
#endif

/** @} */
#endif
