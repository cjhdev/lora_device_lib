#include <stdint.h>

uint32_t system_time = 0U;

uint32_t LDL_System_time(void)
{
    return system_time;
}
