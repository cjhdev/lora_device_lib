/* Copyright (c) 2020 Cameron Harper
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

#ifndef LDL_INTERNAL_H
#define LDL_INTERNAL_H

#include <stdint.h>

#include "ldl_platform.h"

/* these macros are used to cast literals to a specific width
 *
 * It's pedantic but PCLINT in MISRA 2012 mode complains a lot about widths
 *
 *  */
#define U32(X) ((uint32_t)(X))
#define U16(X) ((uint16_t)(X))
#define S16(X) ((int16_t)(X))
#define U8(X) ((uint8_t)(X))

#ifdef LDL_DISABLE_POINTONE
    #define SESS_VERSION(sess) (0U)
#else
    #define SESS_VERSION(sess) sess.version
#endif

#endif
