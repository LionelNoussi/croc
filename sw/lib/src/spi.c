#include "spi.h"
#include "util.h"
#include "config.h"
#include <stdio.h>
// #include "uart.h"


uint8_t spi_write(uint8_t data) {
    SPI_TXDATA = 0xD4;
    SPI_CTRL = 0x1;
 // start=1, clk_div=2
    while (!(SPI_STATUS & 0x1));    // wait for done
    return SPI_RXDATA;
}