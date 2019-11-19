/* Copyright (c) 2019 Cameron Harper
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

#ifndef LDL_OPS_H
#define LDL_OPS_H

/* "operations" 
 * 
 * For lack of a better name, this connects the mac to the security module
 * and frame codec.
 * 
 * It also provides some handy interfaces for testing that the MIC
 * and encryption works as expected.
 * 
 * For our sanity these functions depend on state in ldl_mac but
 * they should never change it directly.
 * 
 * */

#include <stdint.h>
#include <stdbool.h>

struct ldl_mac;
struct ldl_frame_data;
struct ldl_frame_down;
struct ldl_frame_join_request;
struct ldl_system_identity;

/* derive all session keys and write to ldl_sm */
void LDL_OPS_deriveKeys(struct ldl_mac *self);

/* decode and verify a frame (depends on ldl_mac state but does not modify directly) */
bool LDL_OPS_receiveFrame(struct ldl_mac *self, struct ldl_frame_down *f, uint8_t *in, uint8_t len);

/* encode a frame  (depends on ldl_mac state but does not modify directly)  */
uint8_t LDL_OPS_prepareData(struct ldl_mac *self, const struct ldl_frame_data *f, uint8_t *out, uint8_t max);
uint8_t LDL_OPS_prepareJoinRequest(struct ldl_mac *self, const struct ldl_frame_join_request *f, uint8_t *out, uint8_t max);

/* derive expected 32 bit downcounter from 16 least significant bits and update the copy in ldl_mac */
void LDL_OPS_syncDownCounter(struct ldl_mac *self, uint8_t port, uint16_t counter);

#endif
