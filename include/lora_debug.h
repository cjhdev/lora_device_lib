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

#ifndef LORA_DEBUG_H
#define LORA_DEBUG_H

/** @file */

/**
 * @addtogroup ldl_optional
 * 
 * @{
 * */

#include "lora_platform.h"

#ifndef LORA_ERROR
    /** A printf-like function that captures run-time error level messages 
     * 
     * Example (defined from a makefile):
     * 
     * @code
     * -DLORA_ERROR(...)='do{printf(__VA_ARGS__);printf("\n");}while(0);'
     * @endcode
     * 
     * This could also be defined within the file included by #LORA_TARGET_INCLUDE.
     * 
     * If not defined, all LORA_ERROR() messages will be left out of the build.
     * 
     * */
    #define LORA_ERROR(...)
#endif

#ifndef LORA_INFO
    /** A printf-like function that captures run-time info level messages 
     * 
     * Example (defined from a makefile):
     * 
     * @code
     * -DLORA_INFO(...)='do{printf(__VA_ARGS__);printf("\n");}while(0);'
     * @endcode
     * 
     * This could also be defined within the file included by #LORA_TARGET_INCLUDE.
     * 
     * If not defined, all LORA_INFO() messages will be left out of the build.
     * 
     * */
    #define LORA_INFO(...)
#endif

#ifndef LORA_DEBUG
    /** A printf-like function that captures run-time debug level messages with 
     * varaidic arguments 
     * 
     * Example (defined from a makefile):
     * 
     * @code
     * -DLORA_DEBUG(...)='do{printf(__VA_ARGS__);printf("\n");}while(0);'
     * @endcode
     * 
     * This could also be defined within the file included by #LORA_TARGET_INCLUDE.
     * 
     * If not defined, all LORA_DEBUG() messages will be left out of the build.
     * 
     * */
    #define LORA_DEBUG(...)
#endif

#ifndef LORA_ASSERT
    /** An assert-like function that performs run-time assertions on 'X' 
     * 
     * @note Regular assertions are recommended for production
     * 
     * Example (defined from a makefile):
     * 
     * @code
     * -DLORA_ASSERT(X)='assert(X);'
     * @endcode
     * 
     * This could also be defined within the file included by #LORA_TARGET_INCLUDE.
     * 
     * If not defined, all LORA_ASSERT() checks will be left out of the build.
     * 
     * */
    #define LORA_ASSERT(X)
#endif

#ifndef LORA_PEDANTIC
    /** A assert-like function that performs run-time assertions on 'X' 
     * 
     * @note Pedantic assertions are not required for production
     * 
     * Example (defined from a makefile):
     * 
     * @code
     * -DLORA_PEDANTIC(X)='assert(X);'
     * @endcode
     * 
     * This could also be defined within the file included by #LORA_TARGET_INCLUDE.
     * 
     * If not defined, all LORA_PEDANTIC() checks will be left out of the build.
     * 
     * */
    #define LORA_PEDANTIC(X)
#endif

/** @} */
#endif
