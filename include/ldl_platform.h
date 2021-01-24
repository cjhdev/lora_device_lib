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
     * Define to add support for SX1261
     *
     * */
    #define LDL_ENABLE_SX1261
    //#undef LDL_ENABLE_SX126X

    /**
     * Define to add support for SX1262
     *
     * */
    #define LDL_ENABLE_SX1262
    //#undef LDL_ENABLE_SX126X


    #define LDL_DISABLE_CHECK
    #undef LDL_DISABLE_CHECK

    /**
     * Define to remove the link-check feature.
     *
     * This will save a small amount of Flash.
     *
     * */
    #define LDL_DISABLE_LINK_CHECK
    #undef LDL_DISABLE_LINK_CHECK

     /**
      * Define to remove deviceTime MAC command handling
      *
      * This will save a small amount of Flash.
      *
      * */
    #define LDL_DISABLE_DEVICE_TIME
    #undef LDL_DISABLE_DEVICE_TIME

    /**
     * Define to remove code associated with this MAC command.
     *
     * This will save a small amount of Flash.
     *
     * This code cannot be removed if you have enabled a region
     * that requires it.
     *
     * */
    #define LDL_DISABLE_TX_PARAM_SETUP
    #undef LDL_DISABLE_TX_PARAM_SETUP

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
     * Define to apply the proposed change to LoRaWAN 1.1 spec regarding
     * the construction of the A1 block when encrypting Fopts.
     *
     * */
    #define LDL_ENABLE_ERRATA_A1
    #undef LDL_ENABLE_ERRATA_A1

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

    /**
     * Define to enable class B features
     *
     * Currently a work in progress.
     *
     * */
    #define LDL_ENABLE_CLASS_B
    #undef LDL_ENABLE_CLASS_B

    /**
     * Define to add run-time control of OTAA dither interval
     *
     * This is useful for test platforms to speed up the joining
     * process.
     *
     * */
    #define LDL_ENABLE_OTAA_DITHER
    #undef  LDL_ENABLE_OTAA_DITHER

    /**
     * Define to enable test mode features
     *
     * DO NOT ENABLE FOR PRODUCTION
     *
     * */
    #define LDL_ENABLE_TEST_MODE
    #undef LDL_ENABLE_TEST_MODE

    /**
     * Define to prevent SF12 data rates being used.
     *
     * This is useful if you are using an SX127X radio with a standard
     * XTAL.
     *
     * Note that disabling SF12 may mean it is not possible to use
     * mode B.
     *
     * */
     #define LDL_DISABLE_SF12
     #undef LDL_DISABLE_SF12

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

#ifdef LDL_DEFAULT_RATE
    /* it's not a 1:1 replacement but LDL_DISABLE_SF12 is a more
     * reliable way to ensure the device will not use SF12 */
    #error "LDL_DEFAULT_RATE has been replaced by LDL_DISABLE_SF12"
#endif

#ifndef LDL_REDUNDANCY_MAX
    /** Redefine to limit the maximum redundancy setting used to send
     * unconfirmed data frames.
     *
     * (i.e. LinkADRReq.redundancy.NbTrans)
     *
     * The standard limits this value to 15 which is very high.
     * This implementation by default will not send more than three
     * redundant frames. You can raise this limit by
     * redefining LDL_REDUNDANCY_MAX.
     *
     * e.g.
     *
     * @code{.c}
     * #define LDL_REDUNDANCY_MAX 5
     * @endcode
     *
     * */
    #define LDL_REDUNDANCY_MAX 3
#endif

#ifndef LDL_STARTUP_DELAY
    /**
     * Define to add a delay (in milliseconds) to when a device can
     * send its first message after LDL_MAC_init().
     *
     * This is an optional safety feature to ensure a device stuck
     * in a reset loop doesn't transmit too often.
     *
     * */
    #define LDL_STARTUP_DELAY 0
#endif

#ifndef LDL_PARAM_XTAL_DELAY
    /**
     * Define to change the fixed millisecond delay that is inserted
     * before RX and TX operations.
     *
     * This delay is to give time for the xtal to start and stabilise.
     *
     * It can't be too large, it can't be too small.
     *
     *
     * */
    #define LDL_PARAM_XTAL_DELAY 25
#endif

#ifdef LDL_DISABLE_POINTONE
    #error "LDL_DISABLE_POINTONE is depreciated, use LDL_L2_VERSION=LDL_L2_VERSION_1_0_4"
#endif

#ifdef LDL_DISABLE_RANDOM_DEV_NONCE
    #error "LDL_DISABLE_RANDOM_DEV_NONCE is depreciated, use LDL_L2_VERSION=LDL_L2_VERSION_1_0_3"
#endif

#define LDL_L2_VERSION_1_0_3    0
#define LDL_L2_VERSION_1_0_4    1
#define LDL_L2_VERSION_1_1      2

#ifndef LDL_L2_VERSION
    /** Define to change the LoRaWAN L2 version from the default setting.
     *
     * Must be one of:
     *
     * - **LDL_L2_VERSION_1_0_3**
     * - **LDL_L2_VERSION_1_0_4**
     * - **LDL_L2_VERSION_1_1**
     *
     * e.g.
     *
     * @code
     * #define LDL_L2_VERSION   LDL_L2_VERSION_1_0_4
     * @endcode
     *
     * */
    #define LDL_L2_VERSION LDL_L2_VERSION_1_0_4
#endif

#if (LDL_L2_VERSION == LDL_L2_VERSION_1_0_3)
    #define LDL_ENABLE_L2_1_0_3
    #ifdef LDL_ENABLE_CLASS_B
        #error "class b is not supported for LDL_L2_VERSION_1_0_3"
    #endif
#elif (LDL_L2_VERSION == LDL_L2_VERSION_1_0_4)
    #define LDL_ENABLE_L2_1_0_4
#elif (LDL_L2_VERSION == LDL_L2_VERSION_1_1)
    #define LDL_ENABLE_L2_1_1
#else
    #error "unrecognised LDL_L2_VERSION"
#endif

#ifdef LDL_DISABLE_TX_PARAM_SETUP
    #if defined(LDL_ENABLE_AU_915_928)
        /* AU_915_928 region requires the tx param setup mac command */
        #errror "LDL_DISABLE_TX_PARAM_SETUP cannot be defined if LDL_ENABLE_AU_915_928 is defined"
    #endif
#else
    #if !defined(LDL_ENABLE_AU_915_928)
        /* enable the optimisation if there are no regions that require
         * this MAC command */
        #define LDL_DISABLE_TX_PARAM_SETUP
    #endif
#endif

#if defined(LDL_DISABLE_CHECK) && !defined(LDL_DISABLE_LINK_CHECK)
    #warning "LDL_DISABLE_CHECK is depreciated, use LDL_DISABLE_LINK_CHECK"
    #define LDL_DISABLE_LINK_CHECK
#endif

/** @} */




#endif
