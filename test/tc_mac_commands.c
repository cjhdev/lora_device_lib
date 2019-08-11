#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>

#include "cmocka.h"
#include "lora_mac_commands.h"
#include "lora_stream.h"

static void test_putLinkCheckReq(void **user)
{
    uint8_t buffer[50U];
    struct lora_stream s;
    LDL_Stream_init(&s, buffer, sizeof(buffer));    
    bool retval;
    
    uint8_t expected[] = "\x02";
    
    retval = LDL_MAC_putLinkCheckReq(&s);    
    
    assert_true(retval);
    
    assert_int_equal(sizeof(expected)-1U, LDL_Stream_tell(&s));
    assert_memory_equal(expected, buffer, LDL_Stream_tell(&s));    
}

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_putLinkCheckReq),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
