#include <ruby.h>
#include <stddef.h>
#include <assert.h>

#include "ext_ldl.h"
#include "ext_sm.h"
#include "ldl_sm.h"

static VALUE cSM;
static VALUE cKey;

static VALUE initialize(int argc, VALUE *argv, VALUE self);
static VALUE alloc_state(VALUE klass);

static VALUE get_keys(VALUE self);
static VALUE key_desc_to_sym(enum ldl_sm_key key);

static void updateSessionKey(struct ldl_sm *self, enum ldl_sm_key keyDesc, enum ldl_sm_key rootDesc, const void *iv);
static void beginUpdateSessionKey(struct ldl_sm *self);
static void endUpdateSessionKey(struct ldl_sm *self);
static uint32_t mic(struct ldl_sm *self, enum ldl_sm_key desc, const void *hdr, uint8_t hdrLen, const void *data, uint8_t dataLen);
static void ecb(struct ldl_sm *self, enum ldl_sm_key desc, void *b);
static void ctr(struct ldl_sm *self, enum ldl_sm_key desc, const void *iv, void *data, uint8_t len);

const struct ldl_sm_interface ext_sm_interface = {
    .update_session_key = updateSessionKey,
    .begin_update_session_key = beginUpdateSessionKey,
    .end_update_session_key = endUpdateSessionKey,
    .mic = mic,
    .ecb = ecb,
    .ctr = ctr
};

/* functions **********************************************************/

void ext_sm_init(void)
{
    rb_require("ldl/key.rb");

    cSM = rb_define_class_under(cLDL, "SM", rb_cObject);

    cKey = rb_const_get(cLDL, rb_intern("Key"));

    rb_define_alloc_func(cSM, alloc_state);

    rb_define_method(cSM, "initialize", initialize, -1);

    rb_define_method(cSM, "keys", get_keys, 0);
}

/* static functions ***************************************************/

static VALUE initialize(int argc, VALUE *argv, VALUE self)
{
    static const char key[] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

    struct ldl_sm *sm;

    VALUE scenario;
    VALUE options;

    VALUE app_key;
    VALUE nwk_key;

    VALUE default_key = rb_str_new(key, sizeof(key));

    Data_Get_Struct(self, struct ldl_sm, sm);

    (void)rb_scan_args(argc, argv, "10:", &scenario, &options);

    options = (options == Qnil) ? rb_hash_new() : options;

    app_key = rb_hash_aref(options, ID2SYM(rb_intern("app_key")));
    nwk_key = rb_hash_aref(options, ID2SYM(rb_intern("nwk_key")));

    app_key = rb_funcall(cKey, rb_intern("new"), 1, (app_key != Qnil) ? app_key : default_key);
    nwk_key = rb_funcall(cKey, rb_intern("new"), 1, (nwk_key != Qnil) ? nwk_key : default_key);

    LDL_SM_init(sm,
        RSTRING_PTR(rb_funcall(app_key, rb_intern("bytes"), 0)),
        RSTRING_PTR(rb_funcall(nwk_key, rb_intern("bytes"), 0))
    );

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

static void beginUpdateSessionKey(struct ldl_sm *self)
{
    (void)self;
}

static void endUpdateSessionKey(struct ldl_sm *self)
{
    (void)self;
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
