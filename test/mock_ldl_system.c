#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdlib.h>

#include "cmocka.h"

#include "ldl_system.h"
#include "mock_ldl_system.h"

#include <string.h>

void mock_lora_system_init(struct mock_system_param *self)
{
    (void)memset(self, 0, sizeof(*self));
}

uint32_t system_time = 0U;

uint32_t LDL_System_ticks(void *app)
{
    return system_time;
}

uint32_t LDL_System_tps(void)
{
    return 1000000UL;
}

uint32_t LDL_System_eps(void)
{
    return 0UL;
}

uint8_t LDL_System_rand(void *app)
{
    return mock();
}

uint8_t LDL_System_getBatteryLevel(void *receiver)
{
    struct mock_system_param *self = (struct mock_system_param *)receiver;    
    return self->battery_level;    
}

bool LDL_System_restoreContext(void *receiver, struct ldl_mac_session *value)
{
    return false;
}

void LDL_System_saveContext(void *receiver, const struct ldl_mac_session *value)
{
}
