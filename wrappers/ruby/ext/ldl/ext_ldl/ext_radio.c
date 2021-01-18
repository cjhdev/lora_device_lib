#include <ruby.h>
#include <stddef.h>
#include <assert.h>

#include "ext_ldl.h"
#include "ext_radio.h"
#include "ldl_radio.h"

static VALUE cRadio;

/* static prototypes **************************************************/

static void receive_entropy(struct ldl_radio *self);
static unsigned int read_entropy(struct ldl_radio *self);
static void transmit(struct ldl_radio *self, const struct ldl_radio_tx_setting *settings, const void *data, uint8_t len);
static void receive(struct ldl_radio *self, const struct ldl_radio_rx_setting *settings);
static uint8_t read_buffer(struct ldl_radio *self, struct ldl_radio_packet_metadata *meta, void *data, uint8_t max);
static void set_mode(struct ldl_radio *self, enum ldl_radio_mode mode);
static void get_status(struct ldl_radio *self, struct ldl_radio_status *status);

static VALUE bw_to_number(enum ldl_signal_bandwidth bw);
static VALUE sf_to_number(enum ldl_spreading_factor sf);
static enum ldl_spreading_factor number_to_sf(VALUE sf);
static enum ldl_signal_bandwidth number_to_bw(VALUE bw);

static VALUE transmit_time_up(VALUE self, VALUE bandwidth, VALUE spreading_factor, VALUE size);
static VALUE transmit_time_down(VALUE self, VALUE bandwidth, VALUE spreading_factor, VALUE size);

const struct ldl_radio_interface ext_radio_interface = {
    .set_mode = set_mode,
    .read_entropy = read_entropy,
    .read_buffer = read_buffer,
    .transmit = transmit,
    .receive = receive,
    .receive_entropy = receive_entropy,
    .get_status = get_status
};

/* functions **********************************************************/

void ext_radio_init(void)
{
    cRadio = rb_define_class_under(cLDL, "Radio", rb_cObject);

    rb_define_singleton_method(cRadio, "transmit_time_up", transmit_time_up, 3);
    rb_define_singleton_method(cRadio, "transmit_time_down", transmit_time_down, 3);
}



/* static functions **********************************************************/

static unsigned int read_entropy(struct ldl_radio *self)
{
    return rb_genrand_int32();
}

static void transmit(struct ldl_radio *self, const struct ldl_radio_tx_setting *settings, const void *data, uint8_t len)
{
    VALUE params = rb_hash_new();

    rb_hash_aset(params, ID2SYM(rb_intern("freq")), UINT2NUM(settings->freq));
    rb_hash_aset(params, ID2SYM(rb_intern("dbm")), rb_float_new(settings->dbm / 100));
    rb_hash_aset(params, ID2SYM(rb_intern("bw")), bw_to_number(settings->bw));
    rb_hash_aset(params, ID2SYM(rb_intern("sf")), sf_to_number(settings->sf));

    rb_funcall((VALUE)self, rb_intern("transmit"), 2, rb_str_new(data, len), params);
}

static void receive(struct ldl_radio *self, const struct ldl_radio_rx_setting *settings)
{
    VALUE params = rb_hash_new();

    rb_hash_aset(params, ID2SYM(rb_intern("continuous")), settings->continuous ? Qtrue : Qfalse);
    rb_hash_aset(params, ID2SYM(rb_intern("freq")), UINT2NUM(settings->freq));
    rb_hash_aset(params, ID2SYM(rb_intern("timeout")), UINT2NUM(settings->timeout));
    rb_hash_aset(params, ID2SYM(rb_intern("bw")), bw_to_number(settings->bw));
    rb_hash_aset(params, ID2SYM(rb_intern("sf")), sf_to_number(settings->sf));
    rb_hash_aset(params, ID2SYM(rb_intern("max")), UINT2NUM(settings->max));

    rb_funcall((VALUE)self, rb_intern("receive"), 1, params);
}

static uint8_t read_buffer(struct ldl_radio *self, struct ldl_radio_packet_metadata *meta, void *data, uint8_t max)
{
    VALUE buffer;
    VALUE result;
    uint8_t read;

    result = rb_funcall((VALUE)self, rb_intern("read_buffer"), 0);

    meta->rssi = NUM2UINT(rb_hash_aref(result, ID2SYM(rb_intern("rssi"))));
    meta->snr = NUM2UINT(rb_hash_aref(result, ID2SYM(rb_intern("lsnr"))));

    buffer = rb_hash_aref(result, ID2SYM(rb_intern("data")));

    read = (RSTRING_LEN(buffer) > max) ? max : (uint8_t)RSTRING_LEN(buffer);

    (void)memcpy(data, RSTRING_PTR(buffer), read);

    return read;
}

static void set_mode(struct ldl_radio *self, enum ldl_radio_mode mode)
{
    VALUE _mode;

    switch(mode){
    default:
    case LDL_RADIO_MODE_RESET:
        _mode = ID2SYM(rb_intern("reset"));
        break;
    case LDL_RADIO_MODE_BOOT:
        _mode = ID2SYM(rb_intern("boot"));
        break;
    case LDL_RADIO_MODE_SLEEP:
        _mode = ID2SYM(rb_intern("sleep"));
        break;
    case LDL_RADIO_MODE_RX:
        _mode = ID2SYM(rb_intern("rx"));
        break;
    case LDL_RADIO_MODE_TX:
        _mode = ID2SYM(rb_intern("tx"));
        break;
    case LDL_RADIO_MODE_HOLD:
        _mode = ID2SYM(rb_intern("hold"));
        break;
    }

    rb_funcall((VALUE)self, rb_intern("set_mode"), 1, _mode);
}

static void receive_entropy(struct ldl_radio *self)
{
    (void)self;
}

static void get_status(struct ldl_radio *self, struct ldl_radio_status *status)
{
    VALUE result;

    result = rb_funcall((VALUE)self, rb_intern("get_status"), 0);

    status->rx = (rb_hash_aref(result, ID2SYM(rb_intern("rx"))) == Qtrue);
    status->tx = (rb_hash_aref(result, ID2SYM(rb_intern("tx"))) == Qtrue);
    status->timeout = (rb_hash_aref(result, ID2SYM(rb_intern("timeout"))) == Qtrue);
}

static VALUE bw_to_number(enum ldl_signal_bandwidth bw)
{
    return UINT2NUM(LDL_Radio_bwToNumber(bw));
}

static VALUE sf_to_number(enum ldl_spreading_factor sf)
{
    return UINT2NUM(sf);
}

static enum ldl_spreading_factor number_to_sf(VALUE sf)
{
    enum ldl_spreading_factor retval;
    size_t i;

    static const enum ldl_spreading_factor map[] = {
        LDL_SF_7,
        LDL_SF_8,
        LDL_SF_9,
        LDL_SF_10,
        LDL_SF_11,
        LDL_SF_12
    };

    for(i=0U; i < sizeof(map)/sizeof(*map); i++){

        if(map[i] == NUM2UINT(sf)){

            retval = map[i];
            break;
        }
    }

    if(i == sizeof(map)/sizeof(*map)){

        rb_raise(rb_eRangeError, "not a valid spreading factor");
    }

    return retval;
}

static enum ldl_signal_bandwidth number_to_bw(VALUE bw)
{
    enum ldl_signal_bandwidth retval;
    size_t i;

    static const enum ldl_signal_bandwidth map[] = {
        LDL_BW_125,
        LDL_BW_250,
        LDL_BW_500
    };

    for(i=0U; i < sizeof(map)/sizeof(*map); i++){

        if(LDL_Radio_bwToNumber(map[i]) == NUM2UINT(bw)){

            retval = map[i];
            break;
        }
    }

    if(i == sizeof(map)/sizeof(*map)){

        rb_raise(rb_eRangeError, "not a valid bandwidth");
    }

    return retval;
}

static VALUE transmit_time_up(VALUE self, VALUE bandwidth, VALUE spreading_factor, VALUE size)
{
    return UINT2NUM(LDL_Radio_getAirTime(number_to_bw(bandwidth), number_to_sf(spreading_factor), (uint8_t)NUM2UINT(size), true) * 1000UL);
}

static VALUE transmit_time_down(VALUE self, VALUE bandwidth, VALUE spreading_factor, VALUE size)
{
    return UINT2NUM(LDL_Radio_getAirTime(number_to_bw(bandwidth), number_to_sf(spreading_factor), (uint8_t)NUM2UINT(size), false) * 1000UL);
}
