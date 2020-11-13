#include <ruby.h>
#include <stddef.h>
#include <assert.h>

#include "ext_ldl.h"
#include "ext_mac.h"
#include "ext_radio.h"
#include "ext_sm.h"

#include "ldl_mac.h"

static VALUE cExtMAC;
static VALUE cEUI;
static VALUE cError;

static VALUE cErrNoChannel;
static VALUE cErrTooLarge;
static VALUE cErrRate;
static VALUE cErrPort;
static VALUE cErrBusy;
static VALUE cErrNotJoined;
static VALUE cErrPower;
static VALUE cErrMACPriority;

static const uint32_t TPS = LDL_PARAM_TPS;

static VALUE initialize(int argc, VALUE *argv, VALUE self);
static VALUE alloc_state(VALUE klass);

static void statusToException(enum ldl_mac_status status);
static enum ldl_region symbol_to_region(VALUE symbol);

static VALUE process(VALUE self);
static VALUE otaa(VALUE self);
static VALUE forget(VALUE self);
static VALUE unconfirmed(int argc, VALUE *argv, VALUE self);
static VALUE confirmed(int argc, VALUE *argv, VALUE self);
static VALUE radioEvent(VALUE self, VALUE ev);
static VALUE ticksUntilNextEvent(VALUE self);
static VALUE setRate(VALUE self, VALUE rate);
static VALUE getRate(VALUE self);
static VALUE setPower(VALUE self, VALUE power);
static VALUE getPower(VALUE self);
static VALUE setADR(VALUE self, VALUE value);
static VALUE getADR(VALUE self);
static VALUE getState(VALUE self);
static VALUE getOp(VALUE self);
static VALUE cancel(VALUE self);
static VALUE ready(VALUE self);
static VALUE joined(VALUE self);
static VALUE getMaxDCycle(VALUE self);
static VALUE setMaxDCycle(VALUE self, VALUE value);
static VALUE ticks(VALUE self);

static VALUE get_dev_eui(VALUE self);
static VALUE get_join_eui(VALUE self);
static VALUE get_name(VALUE self);

static VALUE seconds_since_valid_downlink(VALUE self);

static uint32_t system_ticks(void *app);
static uint32_t system_rand(void *app);
static void response(void *receiver, enum ldl_mac_response_type type, const union ldl_mac_response_arg *arg);

/* functions **********************************************************/

void ext_mac_init(void)
{
    rb_require("ldl/eui.rb");
    rb_require("ldl/error.rb");

    cExtMAC = rb_define_class_under(cLDL, "ExtMAC", rb_cObject);

    rb_define_alloc_func(cExtMAC, alloc_state);

    rb_include_module(cExtMAC, rb_const_get(cLDL, rb_intern("LoggerMethods")));

    rb_define_const(cExtMAC, "TICKS_PER_SECOND", UINT2NUM(TPS));

    rb_define_method(cExtMAC, "initialize", initialize, -1);

    rb_define_method(cExtMAC, "process", process, 0);

    rb_define_method(cExtMAC, "rate=", setRate, 1);
    rb_define_method(cExtMAC, "rate", getRate, 0);

    rb_define_method(cExtMAC, "power=", setPower, 1);
    rb_define_method(cExtMAC, "power", getPower, 0);

    rb_define_method(cExtMAC, "state", getState, 0);
    rb_define_method(cExtMAC, "op", getOp, 0);

    rb_define_method(cExtMAC, "adr=", setADR, 1);
    rb_define_method(cExtMAC, "adr", getADR, 0);

    rb_define_method(cExtMAC, "max_dcycle=", setMaxDCycle, 1);
    rb_define_method(cExtMAC, "max_dcycle", getMaxDCycle, 0);

    rb_define_method(cExtMAC, "dev_eui", get_dev_eui, 0);
    rb_define_method(cExtMAC, "join_eui", get_join_eui, 0);
    rb_define_method(cExtMAC, "name", get_name, 0);

    rb_define_method(cExtMAC, "otaa", otaa, 0);
    rb_define_method(cExtMAC, "forget", forget, 0);
    rb_define_method(cExtMAC, "cancel", cancel, 0);
    rb_define_method(cExtMAC, "unconfirmed", unconfirmed, -1);
    rb_define_method(cExtMAC, "confirmed", confirmed, -1);
    rb_define_method(cExtMAC, "radio_event", radioEvent, 1);
    rb_define_method(cExtMAC, "ticks_until_next_event", ticksUntilNextEvent, 0);
    rb_define_method(cExtMAC, "ready", ready, 0);
    rb_define_method(cExtMAC, "joined", joined, 0);
    rb_define_method(cExtMAC, "ticks", ticks, 0);

    rb_define_method(cExtMAC, "seconds_since_valid_downlink", seconds_since_valid_downlink, 0);

    cEUI = rb_const_get(cLDL, rb_intern("EUI"));
    cError = rb_const_get(cLDL, rb_intern("Error"));

    cErrNoChannel = rb_const_get(cLDL, rb_intern("ErrNoChannel"));
    cErrTooLarge = rb_const_get(cLDL, rb_intern("ErrTooLarge"));
    cErrRate = rb_const_get(cLDL, rb_intern("ErrRate"));
    cErrPort = rb_const_get(cLDL, rb_intern("ErrPort"));
    cErrBusy = rb_const_get(cLDL, rb_intern("ErrBusy"));
    cErrNotJoined = rb_const_get(cLDL, rb_intern("ErrNotJoined"));
    cErrPower = rb_const_get(cLDL, rb_intern("ErrPower"));
    cErrMACPriority = rb_const_get(cLDL, rb_intern("ErrMACPriority"));
}

void LDL_System_enterCriticalSection(void *app)
{
    VALUE self;
    VALUE mutex;

    self = (VALUE)app;
    mutex = rb_iv_get(self, "@mutex");

    rb_mutex_lock(mutex);
}

void LDL_System_leaveCriticalSection(void *app)
{
    VALUE self;
    VALUE mutex;

    assert(app != NULL);

    self = (VALUE)app;
    mutex = rb_iv_get(self, "@mutex");

    rb_mutex_unlock(mutex);
}

/* static functions ***************************************************/

static VALUE alloc_state(VALUE klass)
{
    return Data_Wrap_Struct(klass, 0, free, calloc(1, sizeof(struct ldl_mac)));
}

static VALUE initialize(int argc, VALUE *argv, VALUE self)
{
    static const uint8_t eui[] = {0,0,0,0,0,0,0,0};

    struct ldl_mac *mac;
    struct ldl_mac_init_arg arg = {0};

    VALUE name;
    VALUE region;
    VALUE radio;
    VALUE sm;
    VALUE options;
    VALUE join_eui;
    VALUE dev_eui;
    VALUE events;
    VALUE scenario;
    VALUE dev_nonce;
    VALUE default_eui = rb_str_new((char *)eui, sizeof(eui));

    Data_Get_Struct(self, struct ldl_mac, mac);

    (void)rb_scan_args(argc, argv, "30:&", &scenario, &sm, &radio, &options, &events);

    options = (options == Qnil) ? rb_hash_new() : options;

    join_eui = rb_hash_aref(options, ID2SYM(rb_intern("join_eui")));
    dev_eui = rb_hash_aref(options, ID2SYM(rb_intern("dev_eui")));
    region = rb_hash_aref(options, ID2SYM(rb_intern("region")));

    dev_nonce = rb_hash_aref(options, ID2SYM(rb_intern("dev_nonce")));
    dev_nonce = (dev_nonce == Qnil) ? UINT2NUM(0U) : dev_nonce;

    join_eui = rb_funcall(cEUI, rb_intern("new"), 1, (join_eui != Qnil) ? join_eui : default_eui);
    dev_eui = rb_funcall(cEUI, rb_intern("new"), 1, (dev_eui != Qnil) ? dev_eui : default_eui);

    name = rb_hash_aref(options, ID2SYM(rb_intern("name")));
    name = (name == Qnil) ? rb_funcall(dev_eui, rb_intern("to_s"), 1, rb_str_new2("")) : name;

    rb_iv_set(self, "@radio", radio);
    rb_iv_set(self, "@sm", sm);
    rb_iv_set(self, "@scenario", scenario);
    rb_iv_set(self, "@join_eui", join_eui);
    rb_iv_set(self, "@dev_eui", dev_eui);
    rb_iv_set(self, "@dev_nonce", dev_nonce);
    rb_iv_set(self, "@mutex", rb_mutex_new());
    rb_iv_set(self, "@events", events);
    rb_iv_set(self, "@name", name);
    rb_iv_set(self, "@log_header", name);

    arg.ticks = system_ticks;
    arg.rand = system_rand;

    arg.app = (void *)self;
    arg.handler = response;

    arg.radio = (struct ldl_radio *)radio;
    arg.radio_interface = &ext_radio_interface;

    arg.sm = (struct ldl_sm *)sm;
    arg.sm_interface = &ext_sm_interface;

    arg.joinEUI = RSTRING_PTR(rb_funcall(join_eui, rb_intern("bytes"), 0));
    arg.devEUI = RSTRING_PTR(rb_funcall(dev_eui, rb_intern("bytes"), 0));
    arg.devNonce = (uint16_t)NUM2UINT(dev_nonce);

    LDL_MAC_init(mac, symbol_to_region(region), &arg);

    return self;
}

static VALUE setRate(VALUE self, VALUE rate)
{
    enum ldl_mac_status retval;
    struct ldl_mac *this;
    Data_Get_Struct(self, struct ldl_mac, this);

    retval = LDL_MAC_setRate(this, (uint8_t)NUM2UINT(rate));

    statusToException(retval);

    return self;
}

static VALUE getRate(VALUE self)
{
    struct ldl_mac *this;
    Data_Get_Struct(self, struct ldl_mac, this);

    return UINT2NUM(LDL_MAC_getRate(this));
}

static VALUE setPower(VALUE self, VALUE power)
{
    enum ldl_mac_status retval;
    struct ldl_mac *this;
    Data_Get_Struct(self, struct ldl_mac, this);

    retval = LDL_MAC_setPower(this, (uint8_t)NUM2UINT(power));

    statusToException(retval);

    return self;
}

static VALUE getPower(VALUE self)
{
    struct ldl_mac *this;
    Data_Get_Struct(self, struct ldl_mac, this);

    return UINT2NUM(LDL_MAC_getPower(this));
}

static VALUE setMaxDCycle(VALUE self, VALUE value)
{
    struct ldl_mac *this;
    Data_Get_Struct(self, struct ldl_mac, this);

    LDL_MAC_setMaxDCycle(this, (uint8_t)NUM2UINT(value));

    return self;
}

static VALUE getMaxDCycle(VALUE self)
{
    struct ldl_mac *this;
    Data_Get_Struct(self, struct ldl_mac, this);

    return UINT2NUM(LDL_MAC_getMaxDCycle(this));
}

static VALUE getState(VALUE self)
{
    struct ldl_mac *this;
    Data_Get_Struct(self, struct ldl_mac, this);

    return UINT2NUM(LDL_MAC_state(this));
}

static VALUE getOp(VALUE self)
{
    struct ldl_mac *this;
    Data_Get_Struct(self, struct ldl_mac, this);

    return UINT2NUM(LDL_MAC_op(this));
}

static VALUE getADR(VALUE self)
{
    struct ldl_mac *this;
    Data_Get_Struct(self, struct ldl_mac, this);

    return LDL_MAC_getADR(this) ? Qtrue : Qfalse;
}

static VALUE setADR(VALUE self, VALUE value)
{
    struct ldl_mac *this;
    Data_Get_Struct(self, struct ldl_mac, this);

    LDL_MAC_setADR(this, (value != Qfalse));

    return self;
}

static void statusToException(enum ldl_mac_status status)
{
    switch(status){
    default:
    case LDL_STATUS_OK:
        break;
    case LDL_STATUS_NOCHANNEL:
        rb_raise(cErrNoChannel, "no channel available");
        break;
    case LDL_STATUS_SIZE:
        rb_raise(cErrTooLarge, "message is too large");
        break;
    case LDL_STATUS_RATE:
        rb_raise(cErrRate, "invalid rate setting");
        break;
    case LDL_STATUS_PORT:
        rb_raise(cErrPort, "invalid port setting");
        break;
    case LDL_STATUS_BUSY:
        rb_raise(cErrBusy, "MAC is busy");
        break;
    case LDL_STATUS_NOTJOINED:
        rb_raise(cErrNotJoined, "MAC must be joined");
        break;
    case LDL_STATUS_POWER:
        rb_raise(cErrPower, "invalid power setting");
        break;
    case LDL_STATUS_MACPRIORITY:
        rb_raise(cErrMACPriority, "MAC commands prioritised (try again)");
        break;
    }
}

static VALUE otaa(VALUE self)
{
    enum ldl_mac_status retval;
    struct ldl_mac *this;
    Data_Get_Struct(self, struct ldl_mac, this);

    retval = LDL_MAC_otaa(this);

    statusToException(retval);

    return self;
}

static VALUE forget(VALUE self)
{
    struct ldl_mac *this;
    Data_Get_Struct(self, struct ldl_mac, this);

    LDL_MAC_forget(this);

    return self;
}

static VALUE cancel(VALUE self)
{
    struct ldl_mac *this;
    Data_Get_Struct(self, struct ldl_mac, this);

    LDL_MAC_cancel(this);

    return self;
}

static VALUE unconfirmed(int argc, VALUE *argv, VALUE self)
{
    enum ldl_mac_status retval;
    VALUE port, data, options, check, nbTrans, dither, getTime;
    struct ldl_mac_data_opts opts;
    struct ldl_mac *this;
    Data_Get_Struct(self, struct ldl_mac, this);

    const void *ptr;
    uint8_t len;

    (void)memset(&opts, 0, sizeof(opts));

    (void)rb_scan_args(argc, argv, "20:", &port, &data, &options);

    ptr = RSTRING_PTR(data);
    len = RSTRING_LEN(data);

    if(options == Qnil){

        options = rb_hash_new();
    }

    check = rb_hash_aref(options, ID2SYM(rb_intern("check")));
    nbTrans = rb_hash_aref(options, ID2SYM(rb_intern("nbTrans")));
    dither = rb_hash_aref(options, ID2SYM(rb_intern("dither")));
    getTime = rb_hash_aref(options, ID2SYM(rb_intern("time")));

    opts.dither = (dither != Qnil) ? NUM2UINT(dither) : 0U;
    opts.nbTrans = (nbTrans != Qnil) ? NUM2UINT(nbTrans) : 0U;
    opts.check = (check == Qtrue);
    opts.getTime = (getTime == Qtrue);

    retval = LDL_MAC_unconfirmedData(this, NUM2UINT(port), ptr, len, &opts);

    statusToException(retval);

    return self;
}

static VALUE confirmed(int argc, VALUE *argv, VALUE self)
{
    enum ldl_mac_status retval;
    VALUE port, data, options, check, nbTrans, dither;
    struct ldl_mac_data_opts opts;
    struct ldl_mac *this;
    Data_Get_Struct(self, struct ldl_mac, this);

    const void *ptr;
    uint8_t len;

    (void)rb_scan_args(argc, argv, "20:", &port, &data, &options);

    ptr = RSTRING_PTR(data);
    len = RSTRING_LEN(data);

    if(options == Qnil){

        options = rb_hash_new();
    }

    check = rb_hash_aref(options, ID2SYM(rb_intern("check")));
    nbTrans = rb_hash_aref(options, ID2SYM(rb_intern("nbTrans")));
    dither = rb_hash_aref(options, ID2SYM(rb_intern("dither")));

    opts.dither = (dither != Qnil) ? NUM2UINT(dither) : 0U;
    opts.nbTrans = (nbTrans != Qnil) ? NUM2UINT(nbTrans) : 0U;
    opts.check = (check == Qtrue);

    retval = LDL_MAC_confirmedData(this, NUM2UINT(port), ptr, len, &opts);

    statusToException(retval);

    return self;
}

static VALUE process(VALUE self)
{
    uint32_t next;
    struct ldl_mac *this;
    Data_Get_Struct(self, struct ldl_mac, this);

    do{

        LDL_MAC_process(this);
        next = LDL_MAC_ticksUntilNextEvent(this);
    }
    while(next == 0);

    return self;
}

static void response(void *receiver, enum ldl_mac_response_type type, const union ldl_mac_response_arg *arg)
{
    VALUE self = (VALUE)receiver;
    VALUE events = rb_iv_get(self, "@events");
    VALUE param = rb_hash_new();
    VALUE event = Qnil;

    switch(type){
    case LDL_MAC_JOIN_COMPLETE:
        event = ID2SYM(rb_intern("join_complete"));
        break;
    case LDL_MAC_JOIN_TIMEOUT:
        event = ID2SYM(rb_intern("join_timeout"));
        break;
    case LDL_MAC_DATA_COMPLETE:
        event = ID2SYM(rb_intern("data_complete"));
        break;
    case LDL_MAC_DATA_TIMEOUT:
        event = ID2SYM(rb_intern("data_timeout"));
        break;
    case LDL_MAC_DATA_NAK:
        event = ID2SYM(rb_intern("data_nack"));
        break;
    case LDL_MAC_RX:
        event = ID2SYM(rb_intern("rx"));
        rb_hash_aset(param, ID2SYM(rb_intern("counter")), UINT2NUM(arg->rx.counter));
        rb_hash_aset(param, ID2SYM(rb_intern("port")), UINT2NUM(arg->rx.port));
        rb_hash_aset(param, ID2SYM(rb_intern("data")), rb_str_new((const char *)arg->rx.data, arg->rx.size));
        break;
    case LDL_MAC_STARTUP:
        event = ID2SYM(rb_intern("startup"));
        rb_hash_aset(param, ID2SYM(rb_intern("entropy")), UINT2NUM(arg->startup.entropy));
        break;
    case LDL_MAC_LINK_STATUS:
        event = ID2SYM(rb_intern("link_status"));
        rb_hash_aset(param, ID2SYM(rb_intern("gw_count")), UINT2NUM(arg->link_status.gwCount));
        rb_hash_aset(param, ID2SYM(rb_intern("gw_margin")), UINT2NUM(arg->link_status.margin));
        break;
    case LDL_MAC_SESSION_UPDATED:
        event = ID2SYM(rb_intern("session_updated"));
        break;
    case LDL_MAC_DEVICE_TIME:
        event = ID2SYM(rb_intern("device_time"));
        rb_hash_aset(param, ID2SYM(rb_intern("seconds")), UINT2NUM(arg->device_time.seconds));
        rb_hash_aset(param, ID2SYM(rb_intern("fractions")), UINT2NUM(arg->device_time.fractions));
        break;
    default:
        rb_raise(rb_eException, "unhandled event");
    }

    if(events != Qnil){

        rb_funcall(events, rb_intern("call"), 2, event, param);
    }
}

static VALUE radioEvent(VALUE self, VALUE ev)
{
    struct ldl_mac *this;
    Data_Get_Struct(self, struct ldl_mac, this);
    size_t i;

    VALUE signals[] = {
        ID2SYM(rb_intern("tx_complete")),
        ID2SYM(rb_intern("rx_ready")),
        ID2SYM(rb_intern("rx_timeout"))
    };

    for(i=0U; i < sizeof(signals)/sizeof(*signals); i++){

        if(ev == signals[i]){

            LDL_MAC_radioEvent(this, i);
            break;
        }
    }

    return self;
}

static VALUE ticksUntilNextEvent(VALUE self)
{
    struct ldl_mac *this;
    uint32_t next;
    Data_Get_Struct(self, struct ldl_mac, this);

    next = LDL_MAC_ticksUntilNextEvent(this);

    return (next == UINT32_MAX) ? Qnil : UINT2NUM(next);
}

static VALUE ready(VALUE self)
{
    bool retval;
    struct ldl_mac *this;
    Data_Get_Struct(self, struct ldl_mac, this);

    retval = LDL_MAC_ready(this);

    return ( retval ? Qtrue : Qfalse );
}

static VALUE joined(VALUE self)
{
    struct ldl_mac *this;
    Data_Get_Struct(self, struct ldl_mac, this);

    return (LDL_MAC_joined(this) ? Qtrue : Qfalse );
}

static VALUE ticks(VALUE self)
{
    return rb_funcall(rb_iv_get(self, "@scenario"), rb_intern("ticks"), 0);
}

static uint32_t system_ticks(void *app)
{
    VALUE self = (VALUE)app;

    return NUM2UINT(rb_funcall(self, rb_intern("ticks"), 0));
}

static uint32_t system_rand(void *app)
{
    return rb_genrand_int32();
}

static enum ldl_region symbol_to_region(VALUE symbol)
{
    enum ldl_region retval;
    size_t i;

    struct region_symbol_map {
        VALUE symbol;
        enum ldl_region region;
    };

    struct region_symbol_map map[] = {
        {
            .symbol = ID2SYM(rb_intern("EU_863_870")),
            .region = LDL_EU_863_870
        },
        {
            .symbol = ID2SYM(rb_intern("US_902_928")),
            .region = LDL_US_902_928
        },
        {
            .symbol = ID2SYM(rb_intern("AU_915_928")),
            .region = LDL_AU_915_928
        }
    };

    if(symbol != Qnil){

        if(rb_obj_is_kind_of(symbol, rb_cSymbol) != Qtrue){

            rb_raise(rb_eTypeError, "region must be kind of symbol");
        }

        for(i=0U; i < sizeof(map)/sizeof(*map); i++){

            if(map[i].symbol == symbol){

                retval = map[i].region;
                break;
            }
        }

        if(i == sizeof(map)/sizeof(*map)){

            rb_raise(rb_eArgError, "invalid region");
        }
    }
    else{

        retval = LDL_EU_863_870;
    }

    return retval;
}

static VALUE get_dev_eui(VALUE self)
{
    return rb_iv_get(self, "@dev_eui");
}

static VALUE get_join_eui(VALUE self)
{
    return rb_iv_get(self, "@join_eui");
}

static VALUE get_name(VALUE self)
{
    return rb_iv_get(self, "@name");
}

static VALUE seconds_since_valid_downlink(VALUE self)
{
    uint32_t retval;
    struct ldl_mac *this;
    Data_Get_Struct(self, struct ldl_mac, this);

    retval = LDL_MAC_secondsSinceValidDownlink(this);

    return (retval != INT32_MAX) ? UINT2NUM(retval) : Qnil;
}
