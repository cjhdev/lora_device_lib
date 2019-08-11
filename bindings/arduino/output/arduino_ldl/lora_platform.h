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

#ifndef LORA_PLATFORM_H
#define LORA_PLATFORM_H
#define LORA_TARGET_INCLUDE "platform.h"

/** @file */

/**
 * @addtogroup ldl_optional
 * 
 * @{
 * */

#ifdef DOXYGEN

    /** Include a target specific file in all headers.
     * 
     * @note This will ensure LORA_TARGET_INCLUDE is included in every LDL source and header file
     * 
     * Within this file you could define things like LORA_ERROR() and LORA_ASSERT().
     * Also use this file to include lora_aes_ctx and lora_cmac_ctx if they
     * have been changed.
     * 
     * Example (defined from a makefile):
     * 
     * @code
     * -DLORA_TARGET_INCLUDE='target_specific.h'
     * @endcode
     * 
     * Where `target_specific.h` would be kept somewhere on the include
     * search path.
     * 
     * */
    #define LORA_TARGET_INCLUDE
    #undef LORA_TARGET_INCLUDE
    
    

    /** 
     * Define this macro to remove the built-in AES implementation.
     * 
     * The effect of defining this macro is that:
     * 
     * - lora_aes.c will build to an empty object
     * - struct lora_aes_ctx is only a forward declaration
     * 
     * */
    #define LORA_ENABLE_PLATFORM_AES
    #undef LORA_ENABLE_PLATFORM_AES
    
    /** 
     * Define this macro to remove the built-in CMAC implementation.
     * 
     * The effect of defining this macro is that:
     * 
     * - lora_cmac.c will build to an empty object
     * - struct lora_cmac_ctx is only a forward declaration
     * 
     * */
    #define LORA_ENABLE_PLATFORM_CMAC
    #undef LORA_ENABLE_PLATFORM_CMAC
    
    /** 
     * Define to add support for SX1272
     * 
     * */
    #define LORA_ENABLE_SX1272
    #undef LORA_ENABLE_SX1272

    /** 
     * Define to add support for SX1276
     * 
     * */
    #define LORA_ENABLE_SX1276
    #undef LORA_ENABLE_SX1276 

    /** 
     * Define to remove parts of the codec not required for a device
     * 
     * */
     #define LORA_DISABLE_FULL_CODEC
     #undef LORA_DISABLE_FULL_CODEC

    /** 
     * Define to remove the link-check feature.
     * 
     * */
    #define LORA_DISABLE_CHECK 
    #undef LORA_DISABLE_CHECK

    /** 
     * Define to remove event generation when RX1 and RX2 are opened.
     * 
     * */
    #define LORA_DISABLE_SLOT_EVENT
    #undef LORA_DISABLE_SLOT_EVENT

    /** 
     * Define to remove event generation when TX completes.
     * 
     * */
    #define LORA_DISABLE_TX_COMPLETE_EVENT
    #undef LORA_DISABLE_TX_COMPLETE_EVENT 

    /** 
     * Define to remove event generation when TX begins.
     * 
     * */
    #define LORA_DISABLE_TX_BEGIN_EVENT  
    #undef LORA_DISABLE_TX_BEGIN_EVENT 

    /**
     * Define to remove event generation for downstream stats
     * 
     * */
    #define LORA_DISABLE_DOWNSTREAM_EVENT
    #undef LORA_DISABLE_DOWNSTREAM_EVENT
    
    /**
     * Define to remove event generation for chip error event
     * 
     * */
    #define LORA_DISABLE_CHIP_ERROR_EVENT
    #undef LORA_DISABLE_CHIP_ERROR_EVENT
    
    /**
     * Define to remove event generation for mac reset event
     * 
     * */
    #define LORA_DISABLE_MAC_RESET_EVENT
    #undef LORA_DISABLE_MAC_RESET_EVENT
    
    /**
     * Define to remove RX event
     * 
     * */
    #define LORA_DISABLE_RX_EVENT
    #undef LORA_DISABLE_RX_EVENT
    
    #define LORA_DISABLE_MAC_STARTUP_EVENT
    #undef LORA_DISABLE_MAC_STARTUP_EVENT
    
    /** 
     * Define to remove the startup delay safety feature
     * 
     * This feature extends the start-up delay to prevent
     * faulty nodes from exceeding the duty cycle limit if
     * they are stuck in a reset/transmit loop.
     * 
     * */
    #define LORA_DISABLE_START_DELAY
    #undef LORA_DISABLE_START_DELAY
    
    /**
     * Define to enable support for AU_915_928
     * 
     * */
    #define LORA_ENABLE_AU_915_928
    #undef LORA_ENABLE_AU_915_928
    
    /**
     * Define to enable support for EU_863_870
     * 
     * */
    #define LORA_ENABLE_EU_863_870
    #undef LORA_ENABLE_EU_863_870
    
    /**
     * Define to enable support for EU_433
     * 
     * */
    #define LORA_ENABLE_EU_433
    #undef LORA_ENABLE_EU_433
    
    /**
     * Define to enable support for US_902_928
     * 
     * */
    #define LORA_ENABLE_US_902_928
    #undef LORA_ENABLE_US_902_928

    /**
     * Define to keep RX buffer in mac state rather than
     * on the stack
     * 
     * */
    #define LORA_ENABLE_STATIC_RX_BUFFER
    #undef LORA_ENABLE_STATIC_RX_BUFFER
    
#endif

#ifdef LORA_TARGET_INCLUDE
    #include LORA_TARGET_INCLUDE    
#endif


/** @} */
#endif
