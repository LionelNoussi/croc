#include "spi.h"
#include "util.h"
#include "config.h"
#include <stdio.h>



void spi_write(uint8_t data) {
    SPI_TXDATA = data;
    SPI_CTRL = (1 << 0) | (2 << 1); // start=1, clk_div=2
    while (!(SPI_STATUS & 0x1));    // wait for done
    uint8_t rx = SPI_RXDATA;
    printf("Received: 0x%02x\n", rx);
}