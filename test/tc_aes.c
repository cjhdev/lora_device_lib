#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>

#include "cmocka.h"

#include "ldl_aes.h"

#include <string.h>

static void test_LDL_AES_init(void **user)
{
    struct ldl_aes_ctx aes;
    static const uint8_t key[] = {0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11};

    LDL_AES_init(&aes, key);
}

static void test_LDL_AES_encrypt(void **user)
{
    static const uint8_t key[] = {0x10,0xa5,0x88,0x69,0xd7,0x4b,0xe5,0xa3,0x74,0xcf,0x86,0x7c,0xfb,0x47,0x38,0x59};
    static const uint8_t pt[] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
    static const uint8_t ct[] = {0x6d,0x25,0x1e,0x69,0x44,0xb0,0x51,0xe0,0x4e,0xaa,0x6f,0xb4,0xdb,0xf7,0x84,0x65};

    struct ldl_aes_ctx aes;
    uint8_t out[16U];

    memcpy(out, pt, sizeof(out));
    LDL_AES_init(&aes, key);
    LDL_AES_encrypt(&aes, out);

    assert_memory_equal(ct, out, sizeof(ct));
}

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_LDL_AES_init),
        cmocka_unit_test(test_LDL_AES_encrypt)
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
