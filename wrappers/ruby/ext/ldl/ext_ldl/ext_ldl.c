#include "ext_ldl.h"
#include "ext_mac.h"
#include "ext_radio.h"
#include "ext_sm.h"

VALUE cLDL;

void Init_ext_ldl(void)
{
    cLDL = rb_define_module("LDL");

    ext_mac_init();
    ext_radio_init();
    ext_sm_init();
}
