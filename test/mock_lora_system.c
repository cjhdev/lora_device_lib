#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdlib.h>

#include "cmocka.h"

#include "lora_system.h"
#include "mock_lora_system.h"

#include <string.h>

void LDL_System_getIdentity(void *receiver, struct lora_system_identity *value)
{
    struct mock_system_param *self = (struct mock_system_param *)receiver;    
    (void)memcpy(value, &self->identity, sizeof(*value));
}

uint16_t LDL_System_getDown(void *receiver)
{
    struct mock_system_param *self = (struct mock_system_param *)receiver;    
    return self->downCounter;   
}

void LDL_System_setDown(void *receiver, uint16_t value)
{
    struct mock_system_param *self = (struct mock_system_param *)receiver;    
    self->downCounter = value;   
}

uint16_t LDL_System_getUp(void *receiver)
{
    struct mock_system_param *self = (struct mock_system_param *)receiver;    
    return self->upCounter;   
}

void LDL_System_setUp(void *receiver, uint16_t value)
{
    struct mock_system_param *self = (struct mock_system_param *)receiver;    
    self->upCounter = value;   
}

uint8_t LDL_System_rand(void)
{
    return mock();
}

uint8_t LDL_System_getBatteryLevel(void *receiver)
{
    struct mock_system_param *self = (struct mock_system_param *)receiver;    
    return self->battery_level;    
}

void mock_lora_system_init(struct mock_system_param *self)
{
    (void)memset(self, 0, sizeof(*self));
}

bool LDL_System_restoreContext(void *receiver, struct lora_mac_session *value)
{
    return false;
}

void LDL_System_saveContext(void *receiver, const struct lora_mac_session *value)
{
}

void LDL_System_usWait(uint8_t value)
{
}

void LDL_System_enterCriticalSection(void *app)
{
}

void LDL_System_leaveCriticalSection(void *app)
{
}

void LDL_System_atomic_setUint(void *app, volatile uint8_t *destination, uint8_t source)
{
    *destination = source;
}

uint8_t LDL_System_atomic_getUint(void *app, const volatile uint8_t *source)
{
    return *source;    
}
