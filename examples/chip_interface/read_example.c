#include <stdint.h>

extern void spi_chip_select(void);
extern void spi_chip_release(void);
extern void spi_write(uint8_t byte);
extern uint8_t spi_read(void);

void LDL_Chip_read(void *self, uint8_t addr, void *data, uint8_t size)
{
    /* unused in this example */
    (void)self;
    
    uint8_t *ptr = (uint8_t *)data;
    uint8_t i;

    spi_chip_select();
    {
        /* SX1272/6 clear the MSb of address to indicate read */
        spi_write(addr & 0x7fU);

        for(i=0U; i < size; i++){

            ptr[i] = spi_read();
        }
    }
    spi_chip_release();    
}
