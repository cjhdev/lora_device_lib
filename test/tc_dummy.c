#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>

#include "cmocka.h"

#include <string.h>

static void test_dummy(void **user)
{
}

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_dummy)
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
