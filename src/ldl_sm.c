/* Copyright (c) 2019-2020 Cameron Harper
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * */

#include "ldl_sm.h"
#include "ldl_ops.h"
#include "ldl_aes.h"
#include "ldl_cmac.h"
#include "ldl_ctr.h"
#include "ldl_debug.h"
#include "ldl_internal.h"

#include <string.h>

static void *getKey(struct ldl_sm *self, enum ldl_sm_key desc);

static const struct ldl_sm_interface interface = {
    .update_session_key = LDL_SM_updateSessionKey,
    .mic = LDL_SM_mic,
    .ecb = LDL_SM_ecb,
    .ctr = LDL_SM_ctr
};

/* functions **********************************************************/

#if defined(LDL_ENABLE_L2_1_0_3) || defined(LDL_ENABLE_L2_1_0_4)
/* LoRaWAN 1.0.x */
void LDL_SM_init(struct ldl_sm *self, const void *appKey)
{
    /* not a mistake, internally we call this LDL_SM_KEY_NWK */
    (void)memcpy(getKey(self, LDL_SM_KEY_NWK), appKey, (appKey != NULL) ? LDL_KEY_SIZE : 0U);
}
#endif

#if defined(LDL_ENABLE_L2_1_1)
/* LoRaWAN 1.1 */
void LDL_SM_init(struct ldl_sm *self, const void *appKey, const void *nwkKey)
{
    (void)memcpy(getKey(self, LDL_SM_KEY_APP), appKey, (appKey != NULL) ? LDL_KEY_SIZE : 0U);
    (void)memcpy(getKey(self, LDL_SM_KEY_NWK), nwkKey, (nwkKey != NULL) ? LDL_KEY_SIZE : 0U);
}
#endif

const struct ldl_sm_interface *LDL_SM_getInterface(void)
{
    return &interface;
}

void LDL_SM_updateSessionKey(struct ldl_sm *self, enum ldl_sm_key keyDesc, enum ldl_sm_key rootDesc, const void *iv)
{
    struct ldl_aes_ctx ctx;

    switch(keyDesc){
    case LDL_SM_KEY_FNWKSINT:
    case LDL_SM_KEY_APPS:
    case LDL_SM_KEY_SNWKSINT:
    case LDL_SM_KEY_NWKSENC:
    case LDL_SM_KEY_JSINT:
    case LDL_SM_KEY_JSENC:

        LDL_AES_init(&ctx, getKey(self, rootDesc));

        (void)memcpy(getKey(self, keyDesc), iv, LDL_KEY_SIZE);

        LDL_AES_encrypt(&ctx, getKey(self, keyDesc));
        break;

    default:
        /* not a session key*/
        break;
    }
}

uint32_t LDL_SM_mic(struct ldl_sm *self, enum ldl_sm_key desc, const void *hdr, uint8_t hdrLen, const void *data, uint8_t dataLen)
{
    uint32_t retval;
    uint8_t mic[sizeof(retval)];
    struct ldl_aes_ctx aes_ctx;
    struct ldl_cmac_ctx ctx;

    LDL_AES_init(&aes_ctx, getKey(self, desc));
    LDL_CMAC_init(&ctx, &aes_ctx);
    LDL_CMAC_update(&ctx, hdr, hdrLen);
    LDL_CMAC_update(&ctx, data, dataLen);
    LDL_CMAC_finish(&ctx, &mic, U8(sizeof(mic)));

    /* intepret the 4th byte as most significant */
    retval = mic[3];
    retval <<= 8;
    retval |= mic[2];
    retval <<= 8;
    retval |= mic[1];
    retval <<= 8;
    retval |= mic[0];

    /* LoRaWAN will encode this least significant byte first */
    return retval;
}

void LDL_SM_ecb(struct ldl_sm *self, enum ldl_sm_key desc, void *b)
{
    struct ldl_aes_ctx ctx;

    LDL_AES_init(&ctx, getKey(self, desc));
    LDL_AES_encrypt(&ctx, b);
}

void LDL_SM_ctr(struct ldl_sm *self, enum ldl_sm_key desc, const void *iv, void *data, uint8_t len)
{
    struct ldl_aes_ctx ctx;

    LDL_AES_init(&ctx, getKey(self, desc));
    LDL_CTR_encrypt(&ctx, iv, data, data, len);
}

/* static functions ***************************************************/

static void *getKey(struct ldl_sm *self, enum ldl_sm_key desc)
{
    size_t i = (size_t)desc;

#if defined(LDL_ENABLE_L2_1_0_3) || defined(LDL_ENABLE_L2_1_0_4)
    /* map 1.1.x key set to 1.0.x key set */
    switch(desc){
    case LDL_SM_KEY_APP:
    case LDL_SM_KEY_NWK:
        i = 0;
        break;
    case LDL_SM_KEY_FNWKSINT:
    case LDL_SM_KEY_SNWKSINT:
    case LDL_SM_KEY_NWKSENC:
    case LDL_SM_KEY_JSINT:
    case LDL_SM_KEY_JSENC:
        i = 1;
        break;
    case LDL_SM_KEY_APPS:
    default:
        i = 2;
        break;
    }
#endif

    LDL_PEDANTIC(i < sizeof(self->keys)/sizeof(*self->keys))

    return self->keys[i].value;
}
