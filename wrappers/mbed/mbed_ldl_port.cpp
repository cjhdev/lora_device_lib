#include <stdio.h>
#include <stdarg.h>
#include "mbed_trace.h"

#include "mbed_ldl_port.h"

#ifdef MBED_CONF_LDL_ENABLE_VERBOSE_DEBUG
static char buffer[1024];
static size_t pos;
#endif

void my_trace_begin(void)
{
#ifdef MBED_CONF_LDL_ENABLE_VERBOSE_DEBUG
    buffer[0] = 0;
    pos = 0U;
#endif
}

void my_trace_part(const char *fmt, ...)
{
#ifdef MBED_CONF_LDL_ENABLE_VERBOSE_DEBUG
    va_list args;
    va_start(args, fmt);
    int retval;

    retval = vsnprintf(&buffer[pos], sizeof(buffer)-pos, fmt, args);

    va_end(args);

    pos += (retval > 0) ? retval : 0U;
#endif
}

void my_trace_hex(const uint8_t *ptr, size_t len)
{
#ifdef MBED_CONF_LDL_ENABLE_VERBOSE_DEBUG
    int retval;
    size_t i;

    for(i=0U; i < len; i++){

        /* printf fmt wasn't doing this for me */
        retval = snprintf(&buffer[pos], sizeof(buffer)-pos, "%X%X",
            ptr[i] >> 4,
            ptr[i]
        );

        pos += (retval > 0) ? retval : 0U;
    }
#endif
}

void my_trace_bitstring(const uint8_t *ptr, size_t len)
{
#ifdef MBED_CONF_LDL_ENABLE_VERBOSE_DEBUG
    int retval;
    size_t i;

    for(i=0U; i < len; i++){

        retval = snprintf(&buffer[pos], sizeof(buffer)-pos, "%u%u%u%u%u%u%u%u",
            (ptr[i] >> 7) & 1U,
            (ptr[i] >> 6) & 1U,
            (ptr[i] >> 5) & 1U,
            (ptr[i] >> 4) & 1U,
            (ptr[i] >> 3) & 1U,
            (ptr[i] >> 2) & 1U,
            (ptr[i] >> 1) & 1U,
            ptr[i] & 1
        );

        pos += (retval > 0) ? retval : 0U;
    }
#endif
}

void my_trace_end(void)
{
#ifdef MBED_CONF_LDL_ENABLE_VERBOSE_DEBUG
    if(pos > 0){

        tr_debug(buffer);
    }
#endif
}
