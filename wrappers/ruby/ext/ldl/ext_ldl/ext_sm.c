#include <ruby.h>
#include <stddef.h>
#include <assert.h>

#include "ext_ldl.h"
#include "ext_sm.h"
#include "ldl_sm.h"

static VALUE cSM;
static VALUE cKey;
static VALUE cSecureRandom;

static VALUE initialize(int argc, VALUE *argv, VALUE self);
static VALUE alloc_state(VALUE klass);

static VALUE get_keys(VALUE self);
static VALUE key_desc_to_sym(enum ldl_sm_key key);

static VALUE get_app_key(VALUE self);
static VALUE get_nwk_key(VALUE self);

static void updateSessionKey(struct ldl_sm *self, enum ldl_sm_key keyDesc, enum ldl_sm_key rootDesc, const void *iv);
static uint32_t mic(struct ldl_sm *self, enum ldl_sm_key desc, const void *hdr, uint8_t hdrLen, const void *data, uint8_t dataLen);
static void ecb(struct ldl_sm *self, enum ldl_sm_key desc, void *b);
static void ctr(struct ldl_sm *self, enum ldl_sm_key desc, const void *iv, void *data, uint8_t len);

const struct ldl_sm_interface ext_sm_interface = {
    .update_session_key = updateSessionKey,
    .mic = mic,
    .ecb = ecb,
    .ctr = ctr
};

/* functions **********************************************************/

void ext_sm_init(void)
{
    rb_require("ldl/key");
    rb_require("securerandom");

    cSecureRandom = rb_const_get(rb_cObject, rb_intern("SecureRandom"));

    cSM = rb_define_class_under(cLDL, "SM", rb_cObject);

    cKey = rb_const_get(cLDL, rb_intern("Key"));

    rb_define_alloc_func(cSM, alloc_state);

    rb_define_method(cSM, "initialize", initialize, -1);

    rb_define_method(cSM, "keys", get_keys, 0);
    rb_define_method(cSM, "nwk_key", get_nwk_key, 0);
    rb_define_method(cSM, "app_key", get_app_key, 0);
}

/* static functions ***************************************************/

static VALUE initialize(int argc, VALUE *argv, VALUE self)
{
    struct ldl_sm *sm;

    VALUE broker;
    VALUE clock;
    VALUE options;

    VALUE app_key;
    VALUE nwk_key;

    Data_Get_Struct(self, struct ldl_sm, sm);

    (void)rb_scan_args(argc, argv, "20:", &broker, &clock, &options);

    options = (options == Qnil) ? rb_hash_new() : options;

    app_key = rb_hash_aref(options, ID2SYM(rb_intern("app_key")));
    nwk_key = rb_hash_aref(options, ID2SYM(rb_intern("nwk_key")));

    if(app_key == Qnil){

        app_key = rb_funcall(cSecureRandom, rb_intern("bytes"), 1, INT2NUM(16));
    }
    else{

        app_key = rb_funcall(rb_funcall(cKey, rb_intern("new"), 1, app_key), rb_intern("bytes"), 0);

    }

    if(nwk_key == Qnil){

        nwk_key = rb_funcall(cSecureRandom, rb_intern("bytes"), 1, INT2NUM(16));
    }
    else{

        nwk_key = rb_funcall(rb_funcall(cKey, rb_intern("new"), 1, nwk_key), rb_intern("bytes"), 0);
    }

    rb_iv_set(self, "@nwk_key", nwk_key);
    rb_iv_set(self, "@app_key", app_key);

    LDL_SM_init(sm, RSTRING_PTR(app_key), RSTRING_PTR(nwk_key));

    return self;
}

static VALUE alloc_state(VALUE klass)
{
    return Data_Wrap_Struct(klass, 0, free, calloc(1, sizeof(struct ldl_sm)));
}

static void updateSessionKey(struct ldl_sm *self, enum ldl_sm_key keyDesc, enum ldl_sm_key rootDesc, const void *iv)
{
    struct ldl_sm *sm;

    Data_Get_Struct((VALUE)self, struct ldl_sm, sm);

    LDL_SM_updateSessionKey(sm, keyDesc, rootDesc, iv);
}

static uint32_t mic(struct ldl_sm *self, enum ldl_sm_key desc, const void *hdr, uint8_t hdrLen, const void *data, uint8_t dataLen)
{
    struct ldl_sm *sm;

    Data_Get_Struct((VALUE)self, struct ldl_sm, sm);

    return LDL_SM_mic(sm, desc, hdr, hdrLen, data, dataLen);
}

static void ecb(struct ldl_sm *self, enum ldl_sm_key desc, void *b)
{
    struct ldl_sm *sm;

    Data_Get_Struct((VALUE)self, struct ldl_sm, sm);

    LDL_SM_ecb(sm, desc, b);
}

static void ctr(struct ldl_sm *self, enum ldl_sm_key desc, const void *iv, void *data, uint8_t len)
{
    struct ldl_sm *sm;

    Data_Get_Struct((VALUE)self, struct ldl_sm, sm);

    LDL_SM_ctr(sm, desc, iv, data, len);
}

static VALUE get_keys(VALUE self)
{
    VALUE retval;
    struct ldl_sm *this;
    Data_Get_Struct(self, struct ldl_sm, this);
    size_t i;

    const enum ldl_sm_key keys[] = {
        LDL_SM_KEY_FNWKSINT,
        LDL_SM_KEY_APPS,
        LDL_SM_KEY_SNWKSINT,
        LDL_SM_KEY_NWKSENC,
        LDL_SM_KEY_JSENC,
        LDL_SM_KEY_JSINT,
        LDL_SM_KEY_APP,
        LDL_SM_KEY_NWK
    };

    retval = rb_hash_new();

    for(i=0; i< sizeof(keys)/sizeof(*keys); i++){

        rb_hash_aset(retval, key_desc_to_sym(i), rb_str_new((char *)this->keys[i].value, 16));
    }

    return retval;
}

static VALUE key_desc_to_sym(enum ldl_sm_key key)
{
    switch(key){
    default:
    case LDL_SM_KEY_FNWKSINT:
        return ID2SYM(rb_intern("fnwksint"));
    case LDL_SM_KEY_APPS:
        return ID2SYM(rb_intern("apps"));
    case LDL_SM_KEY_SNWKSINT:
        return ID2SYM(rb_intern("snwksint"));
    case LDL_SM_KEY_NWKSENC:
        return ID2SYM(rb_intern("nwksenc"));
    case LDL_SM_KEY_JSENC:
        return ID2SYM(rb_intern("jsenc"));
    case LDL_SM_KEY_JSINT:
        return ID2SYM(rb_intern("jsint"));
    case LDL_SM_KEY_APP:
        return ID2SYM(rb_intern("app"));
    case LDL_SM_KEY_NWK:
        return ID2SYM(rb_intern("nwk"));
    }
}

static VALUE get_nwk_key(VALUE self)
{
    return rb_iv_get(self, "@nwk_key");
}

static VALUE get_app_key(VALUE self)
{
    return rb_iv_get(self, "@app_key");
}
