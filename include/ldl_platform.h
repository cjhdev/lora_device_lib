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

#ifndef LDL_PLATFORM_H
#define LDL_PLATFORM_H

/** @file */

/**
 * @defgroup ldl_build_options Build Options
 * @ingroup ldl
 *
 * # Build Options
 *
 * These can be defined in two ways:
 *
 * - using your build system (e.g. using -D)
 * - in a header file which is then included using the #LDL_TARGET_INCLUDE macro
 *
 * @{
 * */



#ifdef DOXYGEN

    /** Include a target specific file in all headers.
     *
     * This file can be used to define:
     *
     * - @ref ldl_build_options
     * - anything else you want included
     *
     * Example (defined from a makefile):
     *
     * @code
     * -DLDL_TARGET_INCLUDE='"target_specific.h"'
     * @endcode
     *
     * Where `target_specific.h` would be kept somewhere on the include
     * search path.
     *
     * */
    #define LDL_TARGET_INCLUDE
    #undef LDL_TARGET_INCLUDE

    /**
     * Define to add support for SX1272
     *
     * */
    #define LDL_ENABLE_SX1272
    //#undef LDL_ENABLE_SX1272

    /**
     * Define to add support for SX1276
     *
     * */
    #define LDL_ENABLE_SX1276
    //#undef LDL_ENABLE_SX1276

    /**
     * Define to remove the link-check feature.
     *
     * */
    #define LDL_DISABLE_CHECK
    #undef LDL_DISABLE_CHECK

    /**
     * Define to enable support for AU_915_928
     *
     * */
    #define LDL_ENABLE_AU_915_928
    //#undef LDL_ENABLE_AU_915_928

    /**
     * Define to enable support for EU_863_870
     *
     * */
    #define LDL_ENABLE_EU_863_870
    //#undef LDL_ENABLE_EU_863_870

    /**
     * Define to enable support for EU_433
     *
     * */
    #define LDL_ENABLE_EU_433
    //#undef LDL_ENABLE_EU_433

    /**
     * Define to enable support for US_902_928
     *
     * */
    #define LDL_ENABLE_US_902_928
    //#undef LDL_ENABLE_US_902_928

    /**
     * Define to keep RX buffer in mac state rather than
     * on the stack.
     *
     * This will save the stack from growing by #LDL_MAX_PACKET bytes
     * when LDL_MAC_process() is called.
     *
     * */
    #define LDL_ENABLE_STATIC_RX_BUFFER
    #undef LDL_ENABLE_STATIC_RX_BUFFER

    /**
     * Define to make use of PROGMEM if using avr-libc
     *
     * */
    #define LDL_ENABLE_AVR
    #undef  LDL_ENABLE_AVR

    /**
     * Define to remove all LoRaWAN 1.1 features
     *
     * This must not be used with 1.1 servers
     *
     * */
    #define LDL_DISABLE_POINTONE
    #undef LDL_DISABLE_POINTONE

    /** Define to apply the proposed change to LoRaWAN 1.1 spec regarding
     * the construction of the A1 block when encrypting Fopts.
     *
     * */
    #define LDL_ENABLE_POINTONE_ERRATA_A1
    #undef LDL_ENABLE_POINTONE_ERRATA_A1

    /** To be true to the 1.0.x specification, if LDL_DISABLE_POINTONE
     * is defined, the devNonce will be initialised from random.
     *
     * Define this build option (with LDL_DISABLE_POINTONE) to have
     * devNonce initialised from a counter.
     *
     * This will avoid the problem of OTAA failing because a random
     * devNonce has already been used.
     *
     * */
    #define LDL_DISABLE_RANDOM_DEV_NONCE
    #undef LDL_DISABLE_RANDOM_DEV_NONCE

     /**
      * Define to remove deviceTime MAC command handling
      *
      * */
     #define LDL_DISABLE_DEVICE_TIME
     #undef LDL_DISABLE_DEVICE_TIME

     /**
      * Define to optimise for little-endian targets
      *
      * */
    #define LDL_LITTLE_ENDIAN
    #undef LDL_LITTLE_ENDIAN

    /**
     * Define to enable verbose radio interface debugging that will
     * buffer and print register access via LDL_TRACE_*
     *
     * This feature uses a buffer to defer printing until the driver
     * is past a timing sensitive part.
     *
     * Note: you need to define LDL_TRACE_* macros to get output.
     *
     * */
    #define LDL_ENABLE_RADIO_DEBUG
    #undef LDL_ENABLE_RADIO_DEBUG

    /**
     * Define to remove delays that are normally inserted in front of
     * operations and retries. These delays are essential for ensuring
     * groups of devices don't continually talk over themselves in the field,
     * but slow down development sometimes.
     *
     * Recommend only using this for nodes that are on your desk and under
     * development.
     *
     * */
    #define LDL_ENABLE_FAST_DEBUG
    #undef LDL_ENABLE_FAST_DEBUG

    /**
     * Define to set #ldl_mac_init_arg.tps at compile time
     *
     * */
    #define LDL_PARAM_TPS
    #undef LDL_PARAM_TPS

    /**
     * Define to set #ldl_mac_init_arg.a at compile time
     *
     * */
    #define LDL_PARAM_A
    #undef LDL_PARAM_A

    /**
     * Define to set #ldl_mac_init_arg.b at compile time
     *
     * */
    #define LDL_PARAM_B
    #undef LDL_PARAM_B

    /**
     * Define to set #ldl_mac_init_arg.advance at compile time
     *
     * */
    #define LDL_PARAM_ADVANCE
    #undef LDL_PARAM_ADVANCE

#endif

#ifdef LDL_TARGET_INCLUDE
    #include LDL_TARGET_INCLUDE
#endif

#ifndef LDL_MAX_PACKET
    /** Redefine this to reduce the stack and data memory footprint.
     *
     * The maximum allowable size is UINT8_MAX bytes
     *
     * */
    #define LDL_MAX_PACKET UINT8_MAX
#endif

#ifndef LDL_DEFAULT_RATE
    /** Redefine to limit maximum rate to this setting
     *
     * Useful if you want to avoid a large spreading factor if your
     * hardware doesn't support it.
     *
     * */
    #define LDL_DEFAULT_RATE 1U

#endif

#ifndef LDL_REDUNDANCY_MAX
    /** Redefine to limit the maximum redundancy setting
     *
     * (i.e. LinkADRReq.redundancy.NbTrans)
     *
     * The implementation and that standard limit this value to 15. Since
     * the network can set this value, if you feel it is way too high to
     * ever consider using, you can use this macro to further limit it.
     *
     * e.g.
     *
     * @code{.c}
     * #define LDL_REDUNDANCY_MAX 3
     * @endcode
     *
     * would ensure there will never be more than 3 redundant frames.
     *
     * */
    #define LDL_REDUNDANCY_MAX 0xfU
#endif

#ifndef LDL_STARTUP_DELAY
    /**
     * Define to add a startup delay (in milliseconds) to when bands become
     * available.
     *
     * This is an optional safety feature to ensure a device stuck
     * in a reset loop doesn't transmit too often.
     *
     * */
    #define LDL_STARTUP_DELAY 0UL
#endif

/** @} */
#endif
