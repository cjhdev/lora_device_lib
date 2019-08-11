#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdbool.h>

#include "cmocka.h"
#include "lora_event.h"

void LDL_Event_init(struct lora_event *self, struct lora_mac *mac, void *app)
{
}

void LDL_Event_receive(struct lora_event *self, enum on_input_types type, uint64_t time)
{
}

void LDL_Event_tick(struct lora_event *self)
{
}

void *LDL_Event_onInput(struct lora_event *self, enum on_input_types event, void *receiver, event_handler_t handler)
{
    return mock_ptr_type(void *);
}

void *LDL_Event_onTimeout(struct lora_event *self, uint64_t timeout, void *receiver, event_handler_t handler)
{
    return mock_ptr_type(void *);
}

void LDL_Event_cancel(struct lora_event *self, void **event)
{
}

uint64_t LDL_Event_intervalUntilNext(struct lora_event *self)
{
    return mock();
}
