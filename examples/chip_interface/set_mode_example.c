#include "ldl_chip.h"

/* this example is for an SX1276 */

void LDL_Chip_reset(bool state)
{
    if(state){

        // drive high
    }
    else{

        // hiz
    }
}

/* On more sophisticated hardware you may also need to switch:
 *
 * - on/off an oscillator
 * - rfi circuit
 * - rfo circuit
 * - boost circuit
 *
 * */
void LDL_Chip_setMode(void *self, enum ldl_chip_mode mode)
{
    switch(mode){
    case LDL_CHIP_MODE_RESET:
        LDL_Chip_reset(true);
        break;
    case LDL_CHIP_MODE_SLEEP:
        LDL_Chip_reset(false);
        break;
    case LDL_CHIP_MODE_STANDBY:
        break;
    case LDL_CHIP_MODE_RX:
        break;
    case LDL_CHIP_MODE_TX_BOOST:
        break;
    case LDL_CHIP_MODE_TX_RFO:
        break;
    }
}
