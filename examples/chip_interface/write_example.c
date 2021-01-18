#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

extern void spi_chip_select(void);
extern void spi_chip_release(void);
extern void spi_write(uint8_t byte);
extern bool read_busy_pin(void);

bool LDL_Chip_write(void *self, const void *opcode, size_t opcode_size, const void *data, size_t size)
{
    /* unused in this example */
    (void)self;

    size_t i;

    spi_chip_select();
    {
        /* required for the SX126X series but not the SX127X series
         *
         * This must occur after selecting the chip since NSS is
         * used to wake from sleep, during which busy pin will be set.
         *
         * consider adding a timeout to recover from a faulty chip */
        while(read_busy_pin());

        for(i=0U; i < opcode_size; i++){

            spi_write(((const uint8_t *)opcode)[i]);
        }

        for(i=0U; i < size; i++){

            spi_write(((const uint8_t *)data)[i]);
        }
    }
    spi_chip_release();

    /* this would return false if there was a busy timeout */
    return true;
}
