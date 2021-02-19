/* Copyright (c) 2021 Cameron Harper
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

#include <string.h>

#include "ldl_debug.h"
#include "ldl_sx126x.h"
#include "ldl_system.h"
#include "ldl_internal.h"

#if defined(LDL_ENABLE_SX1261) || defined(LDL_ENABLE_SX1262) || defined(LDL_ENABLE_WL55)

#define OPCODE_SET_SLEEP                    0x84
#define OPCODE_SET_STANDBY                  0x80
#define OPCODE_SET_FS                       0xc1
#define OPCODE_SET_TX                       0x83
#define OPCODE_SET_RX                       0x82
#define OPCODE_STOP_TIMER_ON_PREAMBLE       0x9f
#define OPCODE_SET_RX_DUTY_CYCLE            0x94
#define OPCODE_SET_CAD                      0xc5
#define OPCODE_SET_TX_CONTINUOUS_WAVE       0xd1
#define OPCODE_SET_TX_INFINITE_PREAMBLE     0xd2
#define OPCODE_SET_REGULATOR_MODE           0x96
#define OPCODE_CALIBRATE                    0x89
#define OPCODE_CALIBRATE_IMAGE              0x98
#define OPCODE_SET_PA_CONFIG                0x95
#define OPCODE_SET_RX_TX_FALLBACK_MODE      0x93
#define OPCODE_WRITE_REGISTER               0x0d
#define OPCODE_READ_REGISTER                0x1d
#define OPCODE_WRITE_BUFFER                 0x0e
#define OPCODE_READ_BUFFER                  0x1e
#define OPCODE_SET_DIO_IRQ_PARAMS           0x08
#define OPCODE_GET_IRQ_STATUS               0x12
#define OPCODE_CLEAR_IRQ_STATUS             0x02
#define OPCODE_SET_DIO2_AS_RF_SWITCH_CTRL   0x9d
#define OPCODE_SET_DIO3_AS_TCXO_CTRL        0x97
#define OPCODE_SET_RF_FREQUENCY             0x86
#define OPCODE_SET_PACKET_TYPE              0x8a
#define OPCODE_GET_PACKET_TYPE              0x11
#define OPCODE_SET_TX_PARAMS                0x8e
#define OPCODE_SET_MODULATION_PARAMS        0x8b
#define OPCODE_SET_PACKET_PARAMS            0x8c
#define OPCODE_SET_CAD_PARAMS               0x88
#define OPCODE_SET_BUFFER_BASE_ADDRESS      0x8f
#define OPCODE_SET_LORA_SYMB_NUM_TIMEOUT    0xa0
#define OPCODE_GET_STATUS                   0xc0
#define OPCODE_GET_RSSI_INST                0x15
#define OPCODE_GET_RX_BUFFER_STATUS         0x13
#define OPCODE_GET_PACKET_STATUS            0x14
#define OPCODE_GET_DEVICE_ERRORS            0x17
#define OPCODE_CLEAR_DEVICE_ERRORS          0x07
#define OPCODE_GET_STATS                    0x10
#define OPCODE_RESET_STATS                  0x00

#define REG_WHITENING_MSB       0x06b8
#define REG_WHITENING_LSB       0x06b9
#define REG_CRC_MSB             0x06bc
#define REG_CRC_LSB             0x06bd
#define REG_CRC_POLY_MSB        0x06be
#define REG_CRC_POLY_LSB        0x06bf
#define REG_SYNC_WORD_0         0x06c0
#define REG_SYNC_WORD_1         0x06c1
#define REG_SYNC_WORD_2         0x06c2
#define REG_SYNC_WORD_3         0x06c3
#define REG_SYNC_WORD_4         0x06c4
#define REG_SYNC_WORD_5         0x06c5
#define REG_SYNC_WORD_6         0x06c6
#define REG_SYNC_WORD_7         0x06c7
#define REG_NODE_ADDRESS        0x06cd
#define REG_BROADCAST_ADDRESS   0x06ce
#define REG_LORA_SYNC_WORD_MSB  0x0740
#define REG_LORA_SYNC_WORD_LSB  0x0741
#define REG_RANDOM_NUMBER_GEN_0 0x0819
#define REG_RANDOM_NUMBER_GEN_1 0x081a
#define REG_RANDOM_NUMBER_GEN_2 0x081b
#define REG_RANDOM_NUMBER_GEN_3 0x081c
#define REG_RX_GAIN             0x08ac
#define REG_OCP_CONFIG          0x08e7
#define REG_XTA_TRIM            0x0911
#define REG_XTB_TRIM            0x0912

enum ldl_radio_sx126x_packet_type {

    PACKET_TYPE_GFSK,
    PACKET_TYPE_LORA
};

struct _modulation_params {

    enum ldl_spreading_factor sf;
    enum ldl_signal_bandwidth bw;
    enum ldl_coding_rate cr;
    bool LowDataRateOptimize;
};

struct _packet_params {

    uint16_t preamble_length;
    bool fixed_length_header;
    uint8_t payload_length;
    bool crc_on;
    bool invert_iq;
};

struct _status {

    enum _status_mode {

        STATUS_MODE_UNUSED = 0,
        STATUS_MODE_STBY_RC = 2,
        STATUS_MODE_STBY_XOSC = 3,
        STATUS_MODE_FS = 4,
        STATUS_MODE_RX = 5,
        STATUS_MODE_TX = 6,
        STATUS_MODE_RFU

    } chip_mode;

    enum _status_command {

        STATUS_COMMAND_RESERVED = 0,
        STATUS_COMMAND_DATA_AVAILABLE = 2,
        STATUS_COMMAND_TIMEOUT = 3,
        STATUS_COMMAND_PROCESSING_ERROR = 4,
        STATUS_COMMAND_EXECUTION_FAILURE = 5,
        STATUS_COMMAND_TX_DONE = 6,
        STATUS_COMMAND_RFU

    } command_status;
};

struct _RxStatus {
};

struct _device_errors {

    bool RC64K_CALIB_ERR;
    bool RC13M_CALIB_ERR;
    bool PLL_CALIB_ERR;
    bool ADC_CALIB_ERR;
    bool IMG_CALIB_ERR;
    bool XOSC_START_ERR;
    bool PLL_LOCK_ERR;
    bool PA_RAMP_ERR;
};

struct _sleep_config {

    bool warm_start;
    bool wakeup_on_rtc_timeout;
};

enum _standby_config {

    STDBY_RC,
    STDBY_XOSC
};

enum _rx_tx_fallback_mode {

    FALLBACK_FS,
    FALLBACK_STDBY_XOSC,
    FALLBACK_STDBY_RC
};

enum _ramp_time {

    SET_RAMP_10U,
    SET_RAMP_20U,
    SET_RAMP_40U,
    SET_RAMP_80U,
    SET_RAMP_200U,
    SET_RAMP_800U,
    SET_RAMP_1700U,
    SET_RAMP_3400U
};

union _packet_status {

    struct {

        bool preamble_err;
        bool sync_err;
        bool adrs_err;
        bool crc_err;
        bool length_err;
        bool abort_err;
        bool pkt_received;
        bool pkt_sent;

        int8_t rssi_sync;
        int8_t rssi_avg;

    } fsk;

    struct {

        int8_t rssi_pkt;
        int8_t snr_pkt;
        int8_t signal_rssi_pkt;

    } lora;

};

enum _sleep_mode {

    SLEEP_MODE_COLD,
    SLEEP_MODE_WARM
};

/* static function prototypes *****************************************/

//static bool SetFs(struct ldl_radio *self);
//static bool StopTimerOnPreamble(struct ldl_radio *self, bool value);
//static bool SetCAD(struct ldl_radio *self);
//static bool GetRssiInst(struct ldl_radio *self, uint32_t *rssi);
//static bool SetTxInfinitePreamble(struct ldl_radio *self);
//static bool SetTxContinuousWave(struct ldl_radio *self);
//static bool GetPacketType(struct ldl_radio *self, enum ldl_radio_sx126x_packet_type *type);
//static bool SetRxTxFallbackMode(struct ldl_radio *self, enum _rx_tx_fallback_mode value);
//static bool ReadReg(struct ldl_radio *self, uint16_t reg, uint8_t *data);
//static bool WriteReg(struct ldl_radio *self, uint16_t reg, uint8_t data);
//static void parseStatus(uint8_t in, struct _status *value);

static void init_state(struct ldl_radio *self, enum ldl_radio_type type, const struct ldl_sx126x_init_arg *arg);
static bool SetSleep(struct ldl_radio *self, enum _sleep_mode mode);
static bool SetStandby(struct ldl_radio *self, enum _standby_config value);
static bool SetTx(struct ldl_radio *self, uint32_t timeout);
static bool SetRx(struct ldl_radio *self, uint32_t timeout);
static bool SetRegulatorMode(struct ldl_radio *self, enum ldl_sx126x_regulator value);
static bool Calibrate(struct ldl_radio *self, uint8_t param);
static bool CalibrateImage(struct ldl_radio *self, uint32_t freq);

static bool SetDioIrqParams(struct ldl_radio *self, uint16_t irq, uint16_t dio1, uint16_t dio2, uint16_t dio3);
static bool GetIrqStatus(struct ldl_radio *self, uint16_t *irq);
static bool ClearIrqStatus(struct ldl_radio *self, uint16_t irq);
#if defined(LDL_ENABLE_SX1261) || defined(LDL_ENABLE_SX1262)
static bool SetDIO2AsRfSwitchCtrl(struct ldl_radio *self, enum ldl_sx126x_txen setting);
#endif
static bool SetDIO3AsTcxoCtrl(struct ldl_radio *self, enum ldl_sx126x_voltage setting, uint8_t ms);
static bool SetRfFrequency(struct ldl_radio *self, uint32_t freq);
static bool SetPacketType(struct ldl_radio *self, enum ldl_radio_sx126x_packet_type type);
static bool SetTxParams(struct ldl_radio *self, int8_t power, enum _ramp_time ramp_time);
static bool SetModulationParams(struct ldl_radio *self, const struct _modulation_params *value);
static bool SetPacketParams(struct ldl_radio *self, const struct _packet_params *value);
static bool SetBufferBaseAddress(struct ldl_radio *self, uint8_t tx_base_addr, uint8_t rx_base_addr);
static bool SetLoRaSymbNumTimeout(struct ldl_radio *self, uint8_t SymbNum);
static bool GetRxBufferStatus(struct ldl_radio *self, uint8_t *PayloadLengthRx, uint8_t *RxStartBufferPointer);
static bool GetPacketStatus(struct ldl_radio *self, union _packet_status *value);

#ifdef LDL_ENABLE_RADIO_DEBUG
static bool GetStatus(struct ldl_radio *self, uint8_t *value);
static bool GetDeviceErrors(struct ldl_radio *self, struct _device_errors *value);
//static bool ClearDeviceErrors(struct ldl_radio *self);
static void printStatus(struct ldl_radio *self, const char *label);
static void printDeviceErrors(struct ldl_radio *self);
//static void printPacketType(struct ldl_radio *self);
#endif

static bool ReadBuffer(struct ldl_radio *self, uint8_t offset, uint8_t *data, uint8_t size);
static bool WriteBuffer(struct ldl_radio *self, uint8_t offset, const uint8_t *data, uint8_t size);

static bool SetPower(struct ldl_radio *self, int16_t dbm);
static bool SetPaConfig(struct ldl_radio *self, uint8_t paDutyCycle, uint8_t hpMax, uint8_t pa);

static bool SetSyncWord(struct ldl_radio *self, uint16_t value);

static const struct ldl_radio_interface interface = {

    .set_mode = LDL_SX126X_setMode,
    .read_entropy = LDL_SX126X_readEntropy,
    .read_buffer = LDL_SX126X_readBuffer,
    .transmit = LDL_SX126X_transmit,
    .receive = LDL_SX126X_receive,
    .receive_entropy = LDL_SX126X_receiveEntropy,
    .get_status = LDL_SX126X_getStatus
};

/* functions **********************************************************/

#ifdef LDL_ENABLE_SX1261
void LDL_SX1261_init(struct ldl_radio *self, const struct ldl_sx126x_init_arg *arg)
{
    init_state(self, LDL_RADIO_SX1261, arg);
}

const struct ldl_radio_interface *LDL_SX1261_getInterface(void)
{
    return &interface;
}
#endif

#ifdef LDL_ENABLE_SX1262
void LDL_SX1262_init(struct ldl_radio *self, const struct ldl_sx126x_init_arg *arg)
{
    init_state(self, LDL_RADIO_SX1262, arg);
}

const struct ldl_radio_interface *LDL_SX1262_getInterface(void)
{
    return &interface;
}
#endif

#ifdef LDL_ENABLE_WL55
void LDL_WL55_init(struct ldl_radio *self, const struct ldl_sx126x_init_arg *arg)
{
    init_state(self, LDL_RADIO_WL55, arg);
}

const struct ldl_radio_interface *LDL_WL55_getInterface(void)
{
    return &interface;
}
#endif

void LDL_SX126X_setMode(struct ldl_radio *self, enum ldl_radio_mode mode)
{
    LDL_PEDANTIC(self != NULL)
    LDL_PEDANTIC((self->type == LDL_RADIO_SX1261) || (self->type == LDL_RADIO_SX1262) || (self->type == LDL_RADIO_WL55))

    LDL_ASSERT(self->chip_read != NULL)
    LDL_ASSERT(self->chip_write != NULL)

    switch(mode){
    default:
    {
        LDL_ABORT()
    }
        break;

    case LDL_RADIO_MODE_RESET:
    {
#ifdef LDL_ENABLE_RADIO_DEBUG
        if(self->mode != LDL_RADIO_MODE_RESET){

            printDeviceErrors(self);
        }
#endif
        self->chip_set_mode(self->chip, LDL_CHIP_MODE_RESET);
    }
        break;

    case LDL_RADIO_MODE_BOOT:
    {
        switch(self->mode){
        case LDL_RADIO_MODE_RESET:

            self->chip_set_mode(self->chip, LDL_CHIP_MODE_SLEEP);
            /* cannot read/write until chip has booted */
            break;

        default:
            LDL_ABORT()
            break;
        }
    }
        break;

    case LDL_RADIO_MODE_SLEEP:
    {
        switch(self->mode){
        case LDL_RADIO_MODE_BOOT:

            (void)SetSleep(self, SLEEP_MODE_COLD);
            break;

        case LDL_RADIO_MODE_SLEEP:
            break;

        case LDL_RADIO_MODE_RX:
        case LDL_RADIO_MODE_TX:
        case LDL_RADIO_MODE_HOLD:

            (void)ClearIrqStatus(self, UINT16_MAX);
            (void)SetSleep(self, SLEEP_MODE_COLD);
            self->chip_set_mode(self->chip, LDL_CHIP_MODE_SLEEP);
            break;

        default:
            LDL_ABORT()
            break;
        }
    }
        break;

    case LDL_RADIO_MODE_RX:
    case LDL_RADIO_MODE_TX:
    {
        switch(self->mode){
        case LDL_RADIO_MODE_BOOT:
        case LDL_RADIO_MODE_SLEEP:

            /* this is from a cold start, therefore already in
             * standby rc mode */

            self->chip_set_mode(self->chip, LDL_CHIP_MODE_STANDBY);

            (void)SetRegulatorMode(self, self->state.sx126x.regulator);

#if defined(LDL_ENABLE_SX1261) || defined(LDL_ENABLE_SX1262)
            switch(self->type){
            case LDL_RADIO_SX1261:
            case LDL_RADIO_SX1262:
                (void)SetDIO2AsRfSwitchCtrl(self, self->state.sx126x.txen);
                break;
            default:
                break;
            }
#endif
            if(self->xtal == LDL_RADIO_XTAL_TCXO){

                /* subtract 3.5ms calibration time */
                (void)SetDIO3AsTcxoCtrl(self, self->state.sx126x.voltage, U8(LDL_PARAM_XTAL_DELAY) - U8(4));

                /* Start the XTAL and re-run calibration.
                 *
                 * assumption: I can turn off image calibration
                 * here because I have to do it again for a known
                 * frequency. The assumption is that the "image calibration"
                 * block is the same one controlled by CalibrateImage().
                 *
                 */
                (void)Calibrate(self, 0x3f);
            }
            else{

                /* start the XTAL */
                (void)SetStandby(self, STDBY_XOSC);
            }

            self->state.sx126x.image_calibration_pending = true;
            break;

        case LDL_RADIO_MODE_HOLD:

            self->chip_set_mode(self->chip, LDL_CHIP_MODE_STANDBY);

            /* start the XTAL */
            (void)SetStandby(self, STDBY_XOSC);
            break;

        default:
            LDL_ABORT()
            break;
        }
    }
        break;

    case LDL_RADIO_MODE_HOLD:
    {
        switch(self->mode){
        case LDL_RADIO_MODE_RX:
        case LDL_RADIO_MODE_TX:

            (void)ClearIrqStatus(self, UINT16_MAX);
            (void)SetSleep(self, SLEEP_MODE_WARM);
            break;

        default:
            LDL_ABORT()
            break;
        }
    }
        break;
    }

    /* printing here may cause RX windows to be missed */
    LDL_TRACE("new_mode=%i cur_mode=%i", mode, self->mode)

    self->mode = mode;
}

void LDL_SX126X_transmit(struct ldl_radio *self, const struct ldl_radio_tx_setting *settings, const void *data, uint8_t len)
{
    LDL_PEDANTIC(self != NULL)
    LDL_PEDANTIC((self->type == LDL_RADIO_SX1261) || (self->type == LDL_RADIO_SX1262) || (self->type == LDL_RADIO_WL55))

    bool ok;

    /* note this is dbm x 100 */
    int16_t dbm = settings->eirp - self->tx_gain;

    do{

        ok = SetPacketType(self, PACKET_TYPE_LORA);
        if(!ok){ break; }

        if(self->state.sx126x.image_calibration_pending){

            ok = CalibrateImage(self, settings->freq);
            if(!ok){ break; }

            self->state.sx126x.image_calibration_pending = false;
        }

        /* set power up here so that IO has time to settle */
        ok = SetPower(self, dbm);
        if(!ok){ break; }

        ok = SetBufferBaseAddress(self, 0, 0);
        if(!ok){ break; }

        ok = SetRfFrequency(self, settings->freq);
        if(!ok){ break; }

        ok = WriteBuffer(self, 0U, data, len);
        if(!ok){ break; }

        {
            struct _modulation_params arg;

            arg.sf = settings->sf;
            arg.bw = settings->bw;
            arg.cr = LDL_CR_5;
            arg.LowDataRateOptimize = ((arg.bw == LDL_BW_125) && ((arg.sf == LDL_SF_11) || (arg.sf == LDL_SF_12))) ? true : false;

            ok = SetModulationParams(self, &arg);
            if(!ok){ break; }
        }

        {
            struct _packet_params arg;

            arg.preamble_length = 8U;
            arg.fixed_length_header = false;
            arg.payload_length = len;
            arg.crc_on = true;
            arg.invert_iq = false;

            ok = SetPacketParams(self, &arg);
            if(!ok){ break; }
        }

        /* TxDone on DIO1 */
        ok = SetDioIrqParams(self, 1, 1, 0, 0);
        if(!ok){ break; }

        ok = SetSyncWord(self, 0x3444);
        if(!ok){ break; }

        ok = SetTx(self, 0);
    }
    while(false);

    if(!ok){

        LDL_DEBUG("chip was busy")
    }
}

void LDL_SX126X_receive(struct ldl_radio *self, const struct ldl_radio_rx_setting *settings)
{
    LDL_PEDANTIC(self != NULL)
    LDL_PEDANTIC((self->type == LDL_RADIO_SX1261) || (self->type == LDL_RADIO_SX1262) || (self->type == LDL_RADIO_WL55))

    bool ok;
    uint8_t timeout = (settings->timeout > U16(UINT8_MAX)) ? U8(UINT8_MAX) : U8(settings->timeout);

    do{

        self->chip_set_mode(self->chip, LDL_CHIP_MODE_RX);

        ok = SetPacketType(self, PACKET_TYPE_LORA);
        if(!ok){ break; }

        if(self->state.sx126x.image_calibration_pending){

            ok = CalibrateImage(self, settings->freq);
            if(!ok){ break; }

            self->state.sx126x.image_calibration_pending = false;
        }

        ok = SetBufferBaseAddress(self, 0, 0);
        if(!ok){ break; }

        ok = SetRfFrequency(self, settings->freq);
        if(!ok){ break; }

        {
            struct _modulation_params arg;

            arg.sf = settings->sf;
            arg.bw = settings->bw;
            arg.cr = LDL_CR_5;
            arg.LowDataRateOptimize = ((arg.bw == LDL_BW_125) && ((arg.sf == LDL_SF_11) || (arg.sf == LDL_SF_12)));

            ok = SetModulationParams(self, &arg);
            if(!ok){ break; }
        }

        {
            struct _packet_params arg;

            arg.preamble_length = 8;
            arg.fixed_length_header = false;
            arg.payload_length = settings->max;
            arg.crc_on = false;
            arg.invert_iq = true;

            ok = SetPacketParams(self, &arg);
            if(!ok){ break; }
        }

        /* RxDone | Timeout on DIO1
         *
         * */
        ok = SetDioIrqParams(self, 0x202, 0x202, 0, 0);
        if(!ok){ break; }

        ok = SetSyncWord(self, 0x3444);
        if(!ok){ break; }

        ok = SetLoRaSymbNumTimeout(self, timeout);
        if(!ok){ break; }

        ok = SetRx(self, 0);
    }
    while(false);

    if(!ok){

        LDL_ERROR("chip was busy")
    }
}

uint8_t LDL_SX126X_readBuffer(struct ldl_radio *self, struct ldl_radio_packet_metadata *meta, void *data, uint8_t max)
{
    bool ok;
    uint8_t size = 0;
    uint8_t offset;
    union _packet_status status;

    do{

        ok = GetRxBufferStatus(self, &size, &offset);
        if(!ok){ break; }

        ok = GetPacketStatus(self, &status);
        if(!ok){ break; }

        meta->rssi = (int16_t)status.lora.signal_rssi_pkt;
        meta->snr = (int16_t)status.lora.snr_pkt;

        size = (size > max) ? max : size;

        ok = ReadBuffer(self, 0, data, size);
        if(!ok){ size = 0; }

    }
    while(false);

    if(!ok){

        LDL_ERROR("chip was busy")
    }

    return size;
}

void LDL_SX126X_receiveEntropy(struct ldl_radio *self)
{
    bool ok;

    self->chip_set_mode(self->chip, LDL_CHIP_MODE_RX);

    do{

        ok = SetPacketType(self, PACKET_TYPE_LORA);
        if(!ok){ break; }

        ok = SetDioIrqParams(self, 0, 0, 0, 0);
        if(!ok){ break; }

        ok = SetRx(self, 0);
    }
    while(false);

    if(!ok){

        LDL_ERROR("chip was busy")
    }
}

uint32_t LDL_SX126X_readEntropy(struct ldl_radio *self)
{
    uint32_t retval = 0;

    uint16_t reg = REG_RANDOM_NUMBER_GEN_0;

    uint8_t opcode[] = {
        OPCODE_READ_REGISTER,
        U8(reg >> 8),
        U8(reg),
        0
    };

    (void)self->chip_read(self->chip, opcode, sizeof(opcode), &retval, sizeof(retval));

    return retval;
}

void LDL_SX126X_getStatus(struct ldl_radio *self, struct ldl_radio_status *status)
{
    uint16_t irq;

    if(GetIrqStatus(self, &irq)){

        status->tx = ((irq & 0x1U) > 0U);
        status->rx = ((irq & 0x2U) > 0U);
        status->timeout = ((irq & 0x200U) > 0U);
    }
    else{

        LDL_ERROR("GetIRQStatus")

        (void)memset(status, 0, sizeof(*status));
    }
}


/* static functions ***************************************************/

static void init_state(struct ldl_radio *self, enum ldl_radio_type type, const struct ldl_sx126x_init_arg *arg)
{
    LDL_PEDANTIC(self != NULL)
    LDL_PEDANTIC(arg != NULL)

    LDL_PEDANTIC(arg->chip_read != NULL)
    LDL_PEDANTIC(arg->chip_write != NULL)
    LDL_PEDANTIC(arg->chip_set_mode != NULL)

    (void)memset(self, 0, sizeof(*self));

    self->type = type;

    self->chip = arg->chip;
    self->chip_read = arg->chip_read;
    self->chip_write = arg->chip_write;
    self->chip_set_mode = arg->chip_set_mode;

    self->tx_gain = arg->tx_gain;
    self->xtal = arg->xtal;

    self->state.sx126x.pa = arg->pa;
    self->state.sx126x.regulator = arg->regulator;
    self->state.sx126x.voltage = arg->voltage;
    self->state.sx126x.txen = arg->txen;

    self->state.sx126x.trim_xtal = arg->trim_xtal;
    self->state.sx126x.xta = arg->xta;
    self->state.sx126x.xtb = arg->xtb;
}

static bool SetPower(struct ldl_radio *self, int16_t dbm)
{
    bool retval = false;
    uint8_t paDutyCycle;
    uint8_t hpMax;
    enum ldl_sx126x_pa pa = LDL_SX126X_PA_LP;

    dbm /= S16(100);

    switch(self->type){
    default:
        break;
#ifdef LDL_ENABLE_SX1261
    case LDL_RADIO_SX1261:
        pa = LDL_SX126X_PA_LP;
        self->chip_set_mode(self->chip, LDL_CHIP_MODE_TX_RFO);
        break;
#endif
#ifdef LDL_ENABLE_SX1262
    case LDL_RADIO_SX1262:
        pa = LDL_SX126X_PA_HP;
        self->chip_set_mode(self->chip, LDL_CHIP_MODE_TX_BOOST);
        break;
#endif
#ifdef LDL_ENABLE_WL55
    case LDL_RADIO_WL55:

        switch(self->state.sx126x.pa){
        default:
        case LDL_SX126X_PA_AUTO:
            pa = (dbm > 15) ? LDL_SX126X_PA_HP : LDL_SX126X_PA_HP;
            self->chip_set_mode(self->chip, (pa == LDL_SX126X_PA_HP) ? LDL_CHIP_MODE_TX_BOOST : LDL_CHIP_MODE_TX_RFO);
            break;
        case LDL_SX126X_PA_LP:
            pa = self->state.sx126x.pa;
            self->chip_set_mode(self->chip, LDL_CHIP_MODE_TX_RFO);
            break;
        case LDL_SX126X_PA_HP:
            pa = self->state.sx126x.pa;
            self->chip_set_mode(self->chip, LDL_CHIP_MODE_TX_BOOST);
            break;
        }
        break;
#endif
    }

    switch(pa){
    default:
        break;
#if defined(LDL_ENABLE_SX1261) || defined(LDL_ENABLE_WL55)
    case LDL_SX126X_PA_LP:

        hpMax = 0;

        if(dbm > 14){

            dbm = 14;
        }
        else if(dbm < -17){

            dbm = -17;
        }
        else{

            /* nothing */
        }

        if(dbm > 10){

            paDutyCycle = 4;
        }
        else{

            paDutyCycle = 1;
        }

        if(SetPaConfig(self, paDutyCycle, hpMax, 1)){

            retval = SetTxParams(self, (int8_t)dbm, SET_RAMP_200U);
        }
        break;
#endif
#if defined(LDL_ENABLE_SX1262) || defined(LDL_ENABLE_WL55)
    case LDL_SX126X_PA_HP:

        if(dbm > 22){

            dbm = 22;
        }
        else if(dbm < -9){

            dbm = -9;
        }
        else{

            /* nothing */
        }

        /* you may want to adjust these values if fine tuning hardware */
        if(dbm > 20){

            paDutyCycle = 4;    /* datasheet says never exceed this value */
            hpMax = 7;          /* datasheet says never exceed this value */
        }
        else if(dbm > 17){

            paDutyCycle = 3;
            hpMax = 5;
        }
        else if(dbm > 14){

            paDutyCycle = 2;
            hpMax = 3;
        }
        else{

            paDutyCycle = 2;
            hpMax = 2;
        }

        if(SetPaConfig(self, paDutyCycle, hpMax, 0)){

            retval = SetTxParams(self, (int8_t)dbm, SET_RAMP_200U);
        }
        break;
#endif
    }

    return retval;
}

static bool SetSleep(struct ldl_radio *self, enum _sleep_mode mode)
{
    uint8_t opcode[] = {
        OPCODE_SET_SLEEP,
        (mode == SLEEP_MODE_WARM) ? 4U : 0U
    };

    return self->chip_write(self->chip, opcode, sizeof(opcode), NULL, 0U);
}

static bool SetStandby(struct ldl_radio *self, enum _standby_config value)
{
    uint8_t opcode[] = {
        OPCODE_SET_STANDBY,
        U8(value)
    };

    return self->chip_write(self->chip, opcode, sizeof(opcode), NULL, 0U);
}

static bool SetTx(struct ldl_radio *self, uint32_t timeout)
{
    uint8_t opcode[] = {
        OPCODE_SET_TX,
        U8(timeout >> 16),
        U8(timeout >> 8),
        U8(timeout)
    };

    return self->chip_write(self->chip, opcode, sizeof(opcode), NULL, 0U);
}

static bool SetRx(struct ldl_radio *self, uint32_t timeout)
{
    uint8_t opcode[] = {
        OPCODE_SET_RX,
        U8(timeout >> 16),
        U8(timeout >> 8),
        U8(timeout)
    };

    return self->chip_write(self->chip, opcode, sizeof(opcode), NULL, 0U);
}

#if 0
static bool SetFs(struct ldl_radio *self)
{
    static const uint8_t opcode[] = {
        OPCODE_SET_FS
    };

    return self->chip_write(self->chip, opcode, sizeof(opcode), NULL, 0U);
}

static bool StopTimerOnPreamble(struct ldl_radio *self, bool enable)
{
    uint8_t opcode[] = {
        OPCODE_STOP_TIMER_ON_PREAMBLE,
        enable ? 1U : 0U
    };

    return self->chip_write(self->chip, opcode, sizeof(opcode), NULL, 0U);
}

static bool SetCAD(struct ldl_radio *self)
{
    static const uint8_t opcode[] = {
        OPCODE_SET_CAD
    };

    return self->chip_write(self->chip, opcode, sizeof(opcode), NULL, 0U);
}

static bool SetTxInfinitePreamble(struct ldl_radio *self)
{
    static const uint8_t opcode[] = {
        OPCODE_SET_TX_INFINITE_PREAMBLE
    };

    return self->chip_write(self->chip, opcode, sizeof(opcode), NULL, 0U);
}

static bool GetRssiInst(struct ldl_radio *self, uint32_t *rssi)
{
    bool retval = false;

    static const uint8_t opcode[] = {
        OPCODE_GET_RSSI_INST,
        0
    };

    uint8_t buffer[2];

    if(self->chip_read(self->chip, opcode, sizeof(opcode), buffer, sizeof(buffer))){

        *rssi = buffer[0];
        *rssi <<= 8;
        *rssi |= buffer[1];

        retval = true;
    }

    return retval;
}

static bool SetTxContinuousWave(struct ldl_radio *self)
{
    static const uint8_t opcode[] = {
        OPCODE_SET_TX_CONTINUOUS_WAVE
    };

    return self->chip_write(self->chip, opcode, sizeof(opcode), NULL, 0U);
}

static bool SetRxTxFallbackMode(struct ldl_radio *self, enum _rx_tx_fallback_mode value)
{
    static const uint8_t mode[] = {
        0x40,
        0x30,
        0x20
    };

    uint8_t opcode[] = {
        OPCODE_SET_RX_TX_FALLBACK_MODE,
        mode[value]
    };

    return self->chip_write(self->chip, opcode, sizeof(opcode), NULL, 0U);
}

static bool GetPacketType(struct ldl_radio *self, enum ldl_radio_sx126x_packet_type *type)
{
    bool retval = false;

    uint8_t buffer[1];

    static const uint8_t opcode[] = {
        OPCODE_GET_PACKET_TYPE,
        0
    };

    if(self->chip_read(self->chip, opcode, sizeof(opcode), buffer, sizeof(buffer))){

        retval = true;

        switch(*buffer){
        case U8(PACKET_TYPE_LORA):
            *type = PACKET_TYPE_LORA;
            break;
        case U8(PACKET_TYPE_GFSK):
            *type = PACKET_TYPE_GFSK;
            break;
        default:
            retval = false;
        }
    }

    return retval;
}
#endif

static bool SetRegulatorMode(struct ldl_radio *self, enum ldl_sx126x_regulator value)
{
    uint8_t opcode[] = {
        OPCODE_SET_REGULATOR_MODE,
        U8(value)
    };

    return self->chip_write(self->chip, opcode, sizeof(opcode), NULL, 0U);
}

static bool Calibrate(struct ldl_radio *self, uint8_t param)
{
    uint8_t opcode[] = {
        OPCODE_CALIBRATE,
        param & 0x7fU
    };

    return self->chip_write(self->chip, opcode, sizeof(opcode), NULL, 0U);
}

static bool CalibrateImage(struct ldl_radio *self, uint32_t freq)
{
    uint8_t f1;
    uint8_t f2;

    if(freq < U32(440000000)){

        f1 = 0x6b;
        f2 = 0x6f;
    }
    else if(freq < U32(510000000)){

        f1 = 0x75;
        f2 = 0x81;
    }
    else if(freq < U32(787000000)){

        f1 = 0xc1;
        f2 = 0xc5;

    }
    else if(freq < U32(870000000)){

        f1 = 0xd7;
        f2 = 0xdb;
    }
    else{

        f1 = 0xe1;
        f2 = 0xe9;
    }

    uint8_t opcode[] = {
        OPCODE_CALIBRATE_IMAGE,
        f1,
        f2
    };

    return self->chip_write(self->chip, opcode, sizeof(opcode), NULL, 0U);
}

static bool SetPaConfig(struct ldl_radio *self, uint8_t paDutyCycle, uint8_t hpMax, uint8_t pa)
{
    uint8_t opcode[] = {
        OPCODE_SET_PA_CONFIG,
        paDutyCycle,
        hpMax,
        pa,
        1
    };

    return self->chip_write(self->chip, opcode, sizeof(opcode), NULL, 0U);
}

static bool SetDioIrqParams(struct ldl_radio *self, uint16_t irq, uint16_t dio1, uint16_t dio2, uint16_t dio3)
{
    uint8_t opcode[] = {
        OPCODE_SET_DIO_IRQ_PARAMS,
        U8(irq >> 8),
        U8(irq),
        U8(dio1 >> 8),
        U8(dio1),
        U8(dio2 >> 8),
        U8(dio2),
        U8(dio3 >> 8),
        U8(dio3)
    };

    return self->chip_write(self->chip, opcode, sizeof(opcode), NULL, 0U);
}

static bool GetIrqStatus(struct ldl_radio *self, uint16_t *irq)
{
    bool retval = false;

    uint8_t opcode[] = {
        OPCODE_GET_IRQ_STATUS,
        0
    };

    uint8_t buffer[2];

    if(self->chip_read(self->chip, opcode, sizeof(opcode), buffer, sizeof(buffer))){

        *irq = buffer[0];
        *irq <<= 8;
        *irq |= buffer[1];

        retval = true;
    }

    return retval;
}

static bool ClearIrqStatus(struct ldl_radio *self, uint16_t irq)
{
    uint8_t opcode[] = {
        OPCODE_CLEAR_IRQ_STATUS,
        U8(irq >> 8),
        U8(irq)
    };

    return self->chip_write(self->chip, opcode, sizeof(opcode), NULL, 0U);
}

#if defined(LDL_ENABLE_SX1261) || defined(LDL_ENABLE_SX1262)
static bool SetDIO2AsRfSwitchCtrl(struct ldl_radio *self, enum ldl_sx126x_txen setting)
{
    uint8_t opcode[] = {
        OPCODE_SET_DIO2_AS_RF_SWITCH_CTRL,
        U8(setting)
    };

    return self->chip_write(self->chip, opcode, sizeof(opcode), NULL, 0U);
}
#endif

static bool SetDIO3AsTcxoCtrl(struct ldl_radio *self, enum ldl_sx126x_voltage setting, uint8_t ms)
{
    /* convert ms to 64KHz clock ticks */
    uint32_t ticks = U32(ms) * U32(64);

    uint8_t opcode[] = {
        OPCODE_SET_DIO3_AS_TCXO_CTRL,
        U8(setting),
        U8(ticks >> 16),
        U8(ticks >> 8),
        U8(ticks)
    };

    return self->chip_write(self->chip, opcode, sizeof(opcode), NULL, 0U);
}

static bool SetRfFrequency(struct ldl_radio *self, uint32_t freq)
{
    uint32_t f = U32((U64(freq) << 25) / U64(32000000));

    uint8_t opcode[] = {
        OPCODE_SET_RF_FREQUENCY,
        U8(f >> 24),
        U8(f >> 16),
        U8(f >> 8),
        U8(f)
    };

    return self->chip_write(self->chip, opcode, sizeof(opcode), NULL, 0U);
}

static bool SetPacketType(struct ldl_radio *self, enum ldl_radio_sx126x_packet_type type)
{
    uint8_t opcode[] = {
        OPCODE_SET_PACKET_TYPE,
        U8(type)
    };

    return self->chip_write(self->chip, opcode, sizeof(opcode), NULL, 0U);
}

static bool SetTxParams(struct ldl_radio *self, int8_t power, enum _ramp_time ramp_time)
{
    uint8_t opcode[] = {
        OPCODE_SET_TX_PARAMS,
        U8(power),
        U8(ramp_time)
    };

    return self->chip_write(self->chip, opcode, sizeof(opcode), NULL, 0U);
}

static bool SetModulationParams(struct ldl_radio *self, const struct _modulation_params *value)
{
    static const uint8_t bw[] = {
        4,
        5,
        6
    };

    uint8_t opcode[] = {
        OPCODE_SET_MODULATION_PARAMS,
        U8(value->sf),
        bw[value->bw],
        U8(value->cr),
        value->LowDataRateOptimize ? 1U : 0U
    };

    return self->chip_write(self->chip, opcode, sizeof(opcode), NULL, 0U);
}

static bool SetPacketParams(struct ldl_radio *self, const struct _packet_params *value)
{
    uint8_t opcode[] = {
        OPCODE_SET_PACKET_PARAMS,
        U8(value->preamble_length >> 8),
        U8(value->preamble_length),
        value->fixed_length_header ? 1U : 0U,
        value->payload_length,
        value->crc_on ? 1U : 0U,
        value->invert_iq ? 1U : 0U
    };

    return self->chip_write(self->chip, opcode, sizeof(opcode), NULL, 0U);
}

static bool SetBufferBaseAddress(struct ldl_radio *self, uint8_t tx_base_addr, uint8_t rx_base_addr)
{
    bool retval;

    uint8_t opcode[] = {
        OPCODE_SET_BUFFER_BASE_ADDRESS,
        tx_base_addr,
        rx_base_addr
    };

    retval = self->chip_write(self->chip, opcode, sizeof(opcode), NULL, 0U);

    return retval;
}

static bool SetLoRaSymbNumTimeout(struct ldl_radio *self, uint8_t SymbNum)
{
    uint8_t opcode[] = {
        OPCODE_SET_LORA_SYMB_NUM_TIMEOUT,
        SymbNum
    };

    return self->chip_write(self->chip, opcode, sizeof(opcode), NULL, 0U);
}

static bool GetRxBufferStatus(struct ldl_radio *self, uint8_t *PayloadLengthRx, uint8_t *RxStartBufferPointer)
{
    bool retval = false;

    uint8_t opcode[] = {
        OPCODE_GET_RX_BUFFER_STATUS,
        0
    };

    uint8_t buffer[2];

    if(self->chip_read(self->chip, opcode, sizeof(opcode), buffer, sizeof(buffer))){

        *PayloadLengthRx = buffer[0];
        *RxStartBufferPointer = buffer[1];

        retval = true;
    }

    return retval;
}

static bool GetPacketStatus(struct ldl_radio *self, union _packet_status *value)
{
    bool retval = false;

    uint8_t opcode[] = {
        OPCODE_GET_PACKET_STATUS,
        0
    };

    uint8_t buffer[3];

    if(self->chip_read(self->chip, opcode, sizeof(opcode), buffer, sizeof(buffer))){

        value->lora.rssi_pkt = -((int8_t)buffer[0])/2;
        value->lora.snr_pkt = ((int8_t)buffer[1])/4;
        value->lora.signal_rssi_pkt = ((int8_t)buffer[2])/2;

        retval = true;
    }

    return retval;
}

#ifdef LDL_ENABLE_RADIO_DEBUG
static bool GetDeviceErrors(struct ldl_radio *self, struct _device_errors *value)
{
    bool retval = false;

    uint16_t errors;

    uint8_t opcode[] = {
        OPCODE_GET_DEVICE_ERRORS,
        0
    };

    uint8_t buffer[2];

    if(self->chip_read(self->chip, opcode, sizeof(opcode), buffer, sizeof(buffer))){

        errors = buffer[0];
        errors <<= 8;
        errors |= buffer[1];

        value->RC64K_CALIB_ERR = ((errors & 0x1) > 0) ? true : false;
        value->RC13M_CALIB_ERR = ((errors & 0x2) > 0) ? true : false;
        value->PLL_CALIB_ERR = ((errors & 0x4) > 0) ? true : false;
        value->ADC_CALIB_ERR = ((errors & 0x8) > 0) ? true : false;
        value->IMG_CALIB_ERR = ((errors & 0x10) > 0) ? true : false;
        value->XOSC_START_ERR = ((errors & 0x20) > 0) ? true : false;
        value->PLL_LOCK_ERR = ((errors & 0x40) > 0) ? true : false;
        value->PA_RAMP_ERR = ((errors & 0x100) > 0) ? true : false;

        retval = true;
    }

    return retval;
}

static bool ClearDeviceErrors(struct ldl_radio *self)
{
    uint8_t opcode[] = {
        OPCODE_CLEAR_DEVICE_ERRORS,
        0
    };

    return self->chip_write(self->chip, opcode, sizeof(opcode), NULL, 0U);
}

static void printDeviceErrors(struct ldl_radio *self)
{
    struct _device_errors errors;

     if(GetDeviceErrors(self, &errors)){

        LDL_TRACE("OpError: "
            "RC64K_CALIB_ERR=%u "
            "RC13M_CALIB_ERR=%u "
            "PLL_CALIB_ERR=%u "
            "ADC_CALIB_ERR=%u "
            "IMG_CALIB_ERR=%u "
            "PLL_LOCK_ERR=%u "
            "PA_RAMP_ERR=%u ",
            errors.RC64K_CALIB_ERR ? 1U : 0U,
            errors.RC13M_CALIB_ERR ? 1U : 0U,
            errors.PLL_CALIB_ERR ? 1U : 0U,
            errors.ADC_CALIB_ERR ? 1U : 0U,
            errors.IMG_CALIB_ERR ? 1U : 0U,
            errors.PLL_LOCK_ERR ? 1U : 0U,
            errors.PA_RAMP_ERR ? 1U : 0U
        )
    }
    else{

        LDL_ERROR("GetDeviceErrors()")
    }
}

static void printStatus(struct ldl_radio *self, const char *label)
{
    uint8_t status;

    if(GetStatus(self, &status)){

        LDL_TRACE("status@%s: chip_mode=%u comand_status=%u", label, (status >> 4) & 7U, (status >> 1) & 7U)
    }
    else{

        LDL_ERROR("GetStatus()")
    }
}

static bool GetStatus(struct ldl_radio *self, uint8_t *value)
{
    uint8_t opcode[] = {
        OPCODE_GET_STATUS
    };

    return self->chip_read(self->chip, opcode, sizeof(opcode), value, sizeof(*value));
}
#endif


#if 0
static bool ReadReg(struct ldl_radio *self, uint16_t reg, uint8_t *data)
{
    uint8_t opcode[] = {
        OPCODE_READ_REGISTER,
        U8(reg >> 8),
        U8(reg),
        0
    };

    return self->chip_read(self->chip, opcode, sizeof(opcode), data, sizeof(*data));
}

static bool WriteReg(struct ldl_radio *self, uint16_t reg, uint8_t data)
{
    uint8_t opcode[] = {
        OPCODE_WRITE_REGISTER,
        U8(reg >> 8),
        U8(reg)
    };

    return self->chip_write(self->chip, opcode, sizeof(opcode), &data, sizeof(data));
}
#endif

static bool ReadBuffer(struct ldl_radio *self, uint8_t offset, uint8_t *data, uint8_t size)
{
    uint8_t opcode[] = {
        OPCODE_READ_BUFFER,
        U8(offset),
        0
    };

    return self->chip_read(self->chip, opcode, sizeof(opcode), data, size);
}

static bool WriteBuffer(struct ldl_radio *self, uint8_t offset, const uint8_t *data, uint8_t size)
{
    uint8_t opcode[] = {
        OPCODE_WRITE_BUFFER,
        U8(offset)
    };

    return self->chip_write(self->chip, opcode, sizeof(opcode), data, size);
}

static bool SetSyncWord(struct ldl_radio *self, uint16_t value)
{
    uint8_t opcode[] = {
        OPCODE_WRITE_REGISTER,
        U8(U16(REG_LORA_SYNC_WORD_MSB) >> 8),
        U8(REG_LORA_SYNC_WORD_MSB),
        U8(value >> 8),
        U8(value)
    };

    return self->chip_write(self->chip, opcode, sizeof(opcode), NULL, 0U);
}

#endif
