#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>

#include <string.h>

#include "cmocka.h"

#include "lora_cmac.h"

void LDL_CMAC_init(struct lora_cmac_ctx *ctx, const struct lora_aes_ctx *aes_ctx)
{
}

void LDL_CMAC_update(struct lora_cmac_ctx *ctx, const void *data, uint8_t len)
{
}

void LDL_CMAC_finish(const struct lora_cmac_ctx *ctx, void *out, uint8_t outMax)
{
    memset(out, 0, outMax);
}
