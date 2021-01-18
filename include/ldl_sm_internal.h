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

#ifndef LDL_SM_INTERNAL_H
#define LDL_SM_INTERNAL_H

/** @file */

/**
 * @defgroup ldl_tsm Security Module
 *
 * LDL depends on the interfaces in this group for performing cryptographic operations.
 *
 * @{
 *
 * */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/** SM state */
struct ldl_sm;

/** SM key descriptor */
enum ldl_sm_key {

    LDL_SM_KEY_FNWKSINT,   /**< FNwkSInt forwarding/uplink (also used as NwkSKey) */
    LDL_SM_KEY_APPS,       /**< AppSKey */
    LDL_SM_KEY_SNWKSINT,   /**< SNwkSInt serving/downlink */
    LDL_SM_KEY_NWKSENC,    /**< NwkSEnc */

    LDL_SM_KEY_JSENC,      /**< JSEncKey */
    LDL_SM_KEY_JSINT,      /**< JSIntKey */

    LDL_SM_KEY_APP,        /**< application root key */
    LDL_SM_KEY_NWK         /**< network root key */
};

struct ldl_sm_interface {

    void (*update_session_key)(struct ldl_sm *self, enum ldl_sm_key key_desc, enum ldl_sm_key root_desc, const void *iv);
    void (*begin_update_session_key)(struct ldl_sm *self);
    void (*end_update_session_key)(struct ldl_sm *self);
    uint32_t (*mic)(struct ldl_sm *self, enum ldl_sm_key desc, const void *hdr, uint8_t hdrLen, const void *data, uint8_t dataLen);
    void (*ecb)(struct ldl_sm *self, enum ldl_sm_key desc, void *b);
    void (*ctr)(struct ldl_sm *self, enum ldl_sm_key desc, const void *iv, void *data, uint8_t len);
};

/** MAC can use this interface to talk to the default SM implementation
 *
 * e.g.
 *
 * @code
 * struct ldl_mac_init_arg arg = {
 *
 *      // ...
 *
 *      .sm_interface = LDL_SM_getInterface()
 *
 *      // ...
 * };
 * @endcode
 *
 * */
const struct ldl_sm_interface *LDL_SM_getInterface(void);

/** Update a session key and save the result in the key store
 *
 * LoRaWAN session keys are derived from clear text encrypted with a
 * root key.
 *
 * @param[in] self
 * @param[in] keyDesc   #ldl_sm_key the key to update
 * @param[in] rootDesc  #ldl_sm_key the key to use as root key in derivation
 * @param[in] iv        16B of text used to derive key
 *
 *
 * */
void LDL_SM_updateSessionKey(struct ldl_sm *self, enum ldl_sm_key keyDesc, enum ldl_sm_key rootDesc, const void *iv);

/** Signal the beginning of session key update transaction
 *
 * SM implementations that perform batch updates can use
 * this signal to initialise a cache prior to receiving multiple
 * LDL_SM_updateSessionKey() calls.
 *
 * @param[in] self
 *
 * */
void LDL_SM_beginUpdateSessionKey(struct ldl_sm *self);

/** Signal the end session key update transaction
 *
 * Always follows a previous call to LDL_SM_beginUpdateSessionKey().
 *
 * SM implementations that perform batch updates can use
 * this signal to perform the actual update operation on the cached
 * key material.
 *
 * @param[in] self
 *
 *
 * */
void LDL_SM_endUpdateSessionKey(struct ldl_sm *self);

/** Lookup a key and use it to produce a MIC
 *
 * The MIC is the four least-significant bytes of an AES-128 CMAC digest of (hdr|data), intepreted
 * as a little-endian integer.
 *
 * Note that sometimes hdr will be empty (hdr=NULL and hdrLen=0).
 *
 * @param[in] self
 * @param[in] desc      #ldl_sm_key
 * @param[in] hdr       may be NULL
 * @param[in] hdrLen
 * @param[in] data
 * @param[in] dataLen
 *
 * */
uint32_t LDL_SM_mic(struct ldl_sm *self, enum ldl_sm_key desc, const void *hdr, uint8_t hdrLen, const void *data, uint8_t dataLen);

/** Lookup a key and use it to perform ECB AES-128 in-place
 *
 * @param[in] self
 * @param[in] desc       #ldl_sm_key
 * @param[in] b         16B block to encrypt in-place (arbitrary alignment)
 *
 * */
void LDL_SM_ecb(struct ldl_sm *self, enum ldl_sm_key desc, void *b);

/** Lookup a key and use it to perform CTR AES-128 in-place
 *
 * @param[in] self
 * @param[in] desc       #ldl_sm_key
 * @param[in] iv        16B block to be used as a nonce/intial value (word aligned)
 * @param[in] data
 * @param[in] len
 *
 * */
void LDL_SM_ctr(struct ldl_sm *self, enum ldl_sm_key desc, const void *iv, void *data, uint8_t len);


#ifdef __cplusplus
}
#endif

/** @} */

#endif
