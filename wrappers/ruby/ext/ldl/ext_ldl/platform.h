#ifndef DEBUG_H
#define DEBUG_H

#include "ext_ldl.h"

#define LDL_ENABLE_AU_915_928
#define LDL_ENABLE_US_902_928
#define LDL_ENABLE_EU_433
#define LDL_ENABLE_EU_863_870

#define LDL_ENABLE_SX1272
#define LDL_ENABLE_SX1276

#define LDL_ENABLE_FAST_DEBUG

#define LDL_DEFAULT_RATE 0U

void LDL_System_enterCriticalSection(void *app);
void LDL_System_leaveCriticalSection(void *app);

#define LDL_SYSTEM_ENTER_CRITICAL(APP) LDL_System_enterCriticalSection(APP);
#define LDL_SYSTEM_LEAVE_CRITICAL(APP) LDL_System_leaveCriticalSection(APP);

#define LDL_PARAM_TPS       1000000
#define LDL_PARAM_ADVANCE   0
#define LDL_PARAM_A         0
#define LDL_PARAM_B         0

#include "ldl_system.h"

#define LDL_ERROR(FMT, ...) \
    do{\
        VALUE msg = rb_sprintf("%s: " FMT, __FUNCTION__, ##__VA_ARGS__);\
        rb_funcall(rb_const_get(cLDL, rb_intern("Scenario")), rb_intern("log_error"), 1, msg);\
    }while(0);

#define LDL_DEBUG(FMT, ...) \
    do{\
        VALUE msg = rb_sprintf("%s: " FMT, __FUNCTION__, ##__VA_ARGS__);\
        rb_funcall(rb_const_get(cLDL, rb_intern("Scenario")), rb_intern("log_debug"), 1, msg);\
    }while(0);

#define LDL_INFO(FMT, ...) \
    do{\
        VALUE msg = rb_sprintf("%s: " FMT, __FUNCTION__, ##__VA_ARGS__);\
        rb_funcall(rb_const_get(cLDL, rb_intern("Scenario")), rb_intern("log_info"), 1, msg);\
    }while(0);

/* LDL_ASSERT will raise a LDL::LoraAssert */
#define LDL_ASSERT(X) \
    if(!(X)){\
        VALUE args[] = {\
            rb_funcall(rb_cFile, rb_intern("basename"), 1, rb_str_new_cstr(__FILE__)),\
            UINT2NUM(__LINE__),\
            rb_str_new_cstr(__FUNCTION__),\
            rb_str_new_cstr(#X)\
        };\
        VALUE msg = rb_str_format(sizeof(args)/sizeof(*args), args, rb_str_new_cstr("%s: %u: %s(): assertion failed: %s"));\
        VALUE ex = rb_funcall(rb_const_get(rb_const_get(rb_cObject, rb_intern("LDL")), rb_intern("LoraAssert")), rb_intern("new"), 1, msg);\
        rb_funcall(rb_mKernel, rb_intern("raise"), 1, ex);\
    }

/* LDL_PEDANTIC will expand to LDL_ASSERT */
#define LDL_PEDANTIC(X) LDL_ASSERT(X)

#endif
