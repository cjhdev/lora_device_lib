#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>

#include "cmocka.h"

#include "lora_region.h"


int main(void)
{
    const struct CMUnitTest tests[] = {
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
