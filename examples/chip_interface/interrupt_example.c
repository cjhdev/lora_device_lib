/*

LDL must be connected to the rising edge of some interrupt lines.

On SX127X this is DIO0 and DIO1.

On SX126X this is DIO1.

*/


void dio0_rising_edge_isr(void)
{
    LDL_Radio_handleInterrupt(&radio, 0);
}

void dio1_rising_edge_isr(void)
{
    LDL_Radio_handleInterrupt(&radio, 1);
}