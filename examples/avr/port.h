#ifndef PORT_H
#define PORT_H

#include <util/atomic.h>

#define LDL_ENABLE_SX1276
#define LDL_ENABLE_EU_863_870
#define LDL_ENABLE_STATIC_RX_BUFFER
#define LDL_LITTLE_ENDIAN
#define LDL_MAX_PACKET 64

#define LDL_ENABLE_AVR

/* some hardware doesn't have stability to make SF12 rates reliable */
#define LDL_DISABLE_SF12

#define LDL_DISABLE_LINK_CHECK
#define LDL_DISABLE_DEVICE_TIME

#define LDL_PARAM_TPS       1000000
#define LDL_PARAM_A         40
#define LDL_PARAM_B         0
#define LDL_PARAM_ADVANCE   0

#define LDL_PARAM_L2_VERSION LDL_L2_VERSION_1_4

#define LDL_SYSTEM_ENTER_CRITICAL(APP) ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
#define LDL_SYSTEM_LEAVE_CRITICAL(APP) }

#endif
