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

#ifndef LDL_PLATFORM_H
#define LDL_PLATFORM_H
#define LDL_TARGET_INCLUDE "platform.h"

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
     * Define to remove event generation when RX1 and RX2 are opened.
     * 
     * */
    #define LDL_DISABLE_SLOT_EVENT
    #undef LDL_DISABLE_SLOT_EVENT

    /** 
     * Define to remove event generation when TX completes.
     * 
     * */
    #define LDL_DISABLE_TX_COMPLETE_EVENT
    #undef LDL_DISABLE_TX_COMPLETE_EVENT 

    /** 
     * Define to remove event generation when TX begins.
     * 
     * */
    #define LDL_DISABLE_TX_BEGIN_EVENT  
    #undef LDL_DISABLE_TX_BEGIN_EVENT 

    /**
     * Define to remove event generation for downstream stats
     * 
     * */
    #define LDL_DISABLE_DOWNSTREAM_EVENT
    #undef LDL_DISABLE_DOWNSTREAM_EVENT
    
    /**
     * Define to remove event generation for chip error event
     * 
     * */
    #define LDL_DISABLE_CHIP_ERROR_EVENT
    #undef LDL_DISABLE_CHIP_ERROR_EVENT
    
    /**
     * Define to remove event generation for mac reset event
     * 
     * */
    #define LDL_DISABLE_MAC_RESET_EVENT
    #undef LDL_DISABLE_MAC_RESET_EVENT
    
    /**
     * Define to remove RX event
     * 
     * */
    #define LDL_DISABLE_RX_EVENT
    #undef LDL_DISABLE_RX_EVENT
    
    /**
     * Define to remove startup event
     * 
     * */
    #define LDL_DISABLE_MAC_STARTUP_EVENT
    #undef LDL_DISABLE_MAC_STARTUP_EVENT
    
    /**
     * Define to remove join timeout event
     * 
     * */
    #define LDL_DISABLE_JOIN_TIMEOUT_EVENT
    #undef LDL_DISABLE_JOIN_TIMEOUT_EVENT
    
    /**
     * Define to remove data complete event
     * 
     * */
    #define LDL_DISABLE_DATA_COMPLETE_EVENT
    #undef LDL_DISABLE_DATA_COMPLETE_EVENT
    
    /**
     * Define to remove data timeout event
     * 
     * */
    #define LDL_DISABLE_DATA_TIMEOUT_EVENT
    #undef LDL_DISABLE_DATA_TIMEOUT_EVENT
    
    
    /**
     * Define to remove data NAK event
     * 
     * */
    #define LDL_DISABLE_DATA_NAK_EVENT
    #undef LDL_DISABLE_DATA_NAK_EVENT
    
    /**
     * Define to remove join complete event
     * 
     * */
    #define LDL_DISABLE_JOIN_COMPLETE_EVENT
    #undef LDL_DISABLE_JOIN_COMPLETE_EVENT
    
    /** 
     * Define to add a startup delay (in milliseconds) to when bands become
     * available.
     * 
     * This is an optional safety feature to ensure a device stuck
     * in a reset loop doesn't transmit too often.
     *
     * If undefined this defaults to zero.
     * 
     * */
    #define LDL_STARTUP_DELAY
    #undef LDL_STARTUP_DELAY
    
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
     * Define to remove the #LDL_MAC_SESSION_UPDATED event
     * 
     * This event is useful when session is saved to non-volatile memory
     * 
     * */
    #define LDL_DISABLE_SESSION_UPDATE
    #undef LDL_DISABLE_SESSION_UPDATE

    /**
     * Define to make use of PROGMEM if using avr-libc
     * 
     * */
    #define LDL_ENABLE_AVR
    #undef  LDL_ENABLE_AVR

    /**
     * Define to have LDL generate devNonce from random
     * 
     * This should not be used with LoRaWAN 1.1 servers
     * 
     * */
     #define LDL_ENABLE_RANDOM_DEV_NONCE
     #undef  LDL_ENABLE_RANDOM_DEV_NONCE
     
     /**
      * Define to remove deviceTime MAC command handling
      * 
      * */
     #define LDL_DISABLE_DEVICE_TIME
     #undef LDL_DISABLE_DEVICE_TIME

    /**
     * Define to reduce maximum number of channels from 16 to 8.
     * 
     * The specification requires memory for up to 16 channels to 
     * support regions with dynamic channels.
     * 
     * At time of writing most (all?) gateways only give 8 channels.
     * This build option is useful for demonstrations that are short
     * on memory since this will claw back at least 64B.
     * 
     * */
    #define LDL_DISABLE_FULL_CHANNEL_CONFIG
    #undef LDL_DISABLE_FULL_CHANNEL_CONFIG
        
    /**
     * Define to remove full handling of the DlChannelReq/Ans MAC command.
     * 
     * LDL will continue to parse and respond to this but it won't
     * do anything.
     * 
     * This is useful only if you are desperate to recover RAM and
     * don't need this feature.
     * 
     * */
     #define LDL_DISABLE_CMD_DL_CHANNEL
     #undef LDL_DISABLE_CMD_DL_CHANNEL

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

/** @} */
#endif
