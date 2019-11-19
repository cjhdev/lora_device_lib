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

#ifndef LDL_DEBUG_H
#define LDL_DEBUG_H

/** @file */

/**
 * @addtogroup ldl_build_options
 * 
 * @{
 * */

#include "ldl_platform.h"

#ifndef LDL_ERROR
    /** A printf-like function that captures run-time error level messages 
     * 
     * Example:
     * 
     * @code{.c}
     * #define LDL_ERROR(...) do{printf(__VA_ARGS__);printf("\n");}while(0);
     * @endcode
     * 
     * If not defined, all LDL_ERROR() messages will be left out of the build.
     * 
     * @param[in] APP app from LDL_MAC_init() or NULL if not available
     * 
     * */
    #define LDL_ERROR(APP,...)
#endif

#ifndef LDL_INFO
    /** A printf-like function that captures run-time info level messages 
     * 
     * Example:
     * 
     * @code{.c}
     * #define LDL_INFO(...) do{printf(__VA_ARGS__);printf("\n");}while(0);
     * @endcode
     * 
     * If not defined, all LDL_INFO() messages will be left out of the build.
     * 
     * @param[in] APP app from LDL_MAC_init() or NULL if not available
     * 
     * */
    #define LDL_INFO(APP,...)
#endif

#ifndef LDL_DEBUG
    /** A printf-like function that captures run-time debug level messages with 
     * varaidic arguments 
     * 
     * Example:
     * 
     * @code{.c}
     * #define LDL_DEBUG(APP, ...) do{printf(__VA_ARGS__);printf("\n");}while(0);
     * @endcode
     * 
     * If not defined, all LDL_DEBUG() messages will be left out of the build.
     * 
     * @param[in] APP app from LDL_MAC_init() or NULL if not available
     * 
     * */
    #define LDL_DEBUG(APP, ...)
#endif

#ifndef LDL_ASSERT
    /** An assert-like function that performs run-time assertions on 'X' 
     * 
     * Example:
     * 
     * @code{.c}
     * #define LDL_ASSERT(X) assert(X);
     * @endcode
     * 
     * If not defined, all LDL_ASSERT() checks will be left out of the build.
     * 
     * */
    #define LDL_ASSERT(X)
#endif

#ifndef LDL_PEDANTIC
    /** A assert-like function that performs run-time assertions on 'X' 
     * 
     * These assertions are considered pedantic. They are useful for development
     * but excessive for production.
     * 
     * Example:
     * 
     * @code{.c}
     * #define LDL_PEDANTIC(X) assert(X);
     * @endcode
     * 
     * If not defined, all LDL_PEDANTIC() checks will be left out of the build.
     * 
     * */
    #define LDL_PEDANTIC(X)
#endif

/** @} */
#endif
