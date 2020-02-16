#include <stdint.h>

extern void spi_chip_select(void);
extern void spi_chip_release(void);
extern void spi_write(uint8_t byte);

void LDL_Chip_write(void *self, uint8_t addr, const void *data, uint8_t size)
{
    /* unused in this example */
    (void)self;
    
    const uint8_t *ptr = (uint8_t *)data;
    uint8_t i;

    spi_chip_select();
    {
        /* SX1272/6 set the MSb of address indicate write */
        spi_write(addr | 0x80U);

        for(i=0; i < size; i++){

            spi_write(ptr[i]);
        }
    }   
    spi_chip_release();    
}
