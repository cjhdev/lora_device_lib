#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>

#include "cmocka.h"

#include "lora_frame.h"

size_t LDL_Frame_putData(enum lora_frame_type type, const void *key, const struct lora_frame_data *f, void *out, size_t max)
{
    return mock_type(size_t);
}

size_t LDL_Frame_putJoinRequest(const void *key, const struct lora_frame_join_request *f, void *out, size_t max)
{
    return mock_type(size_t);
}

size_t LDL_Frame_putJoinAccept(const void *key, const struct lora_frame_join_accept *f, void *out, size_t max)
{
    
    return mock_type(size_t);
}

bool LDL_Frame_decode(const void *appKey, const void *nwkSKey, const void *appSKey, void *in, size_t len, struct lora_frame *f)
{
    *f = *mock_ptr_type(struct lora_frame *);
    return mock_type(bool);
}

size_t LDL_Frame_getPhyPayloadSize(size_t dataLen, size_t optsLen)
{
    return mock_type(size_t);
}

bool LDL_Frame_isUpstream(enum lora_frame_type type)
{
    return mock_type(bool);
}
