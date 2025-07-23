#include "spi.h"
#include "util.h"
#include "config.h"
#include <stdio.h>

// Write data to external memory via SPI
void spi_write(uint16_t addr, const uint8_t *data, uint8_t length) {
    // Set target address
    // SPI_ADDR_HI = (addr >> 8) & 0xFF;
    // SPI_ADDR_LO = addr & 0xFF;
    // uint8_t low = 0xA1;
    // uint8_t high = 0x1D;
    // SPI_ADDR_HI = low;
    // SPI_ADDR_LO = high;
    *(volatile uint8_t*)(SPI_BASE_ADDR + 0x28) = 0x34;
    *(volatile uint8_t*)(SPI_BASE_ADDR + 0x29) = 0x12;

    // Write data to TX buffer
    // for (uint8_t i = 0; i < length; i++) {
    //     SPI_TX(i) = data[i];
    // }

    // Set transfer length
    uint8_t length2= 0x46;
    SPI_LENGTH = length2;

    uint8_t testd = 0x82;
    SPI_TX(0) = testd;
    SPI_TX(1) = 0x33;
    SPI_TX(2) = 0xAF;
    // Start write: [7:3]=length, [2:1]=0b10 (write), [0]=1 (start)
    SPI_CTRL = (length << 3) | (2 << 1) | 0x1;

    // Wait until done
    // while ((SPI_STATUS & 0x1) == 0);
}

// Read data from external memory via SPI
void spi_read(uint16_t addr, uint8_t *data, uint8_t length) {
    // Set target address
    SPI_ADDR_HI = (addr >> 8) & 0xFF;
    SPI_ADDR_LO = addr & 0xFF;

    // Set transfer length
    SPI_LENGTH = length;

    // Start read: [7:3]=length, [2:1]=0b01 (read), [0]=1 (start)
    SPI_CTRL = (length << 3) | (1 << 1) | 0x1;

    // Wait until done
    while ((SPI_STATUS & 0x1) == 0);

    // Read from RX buffer
    for (uint8_t i = 0; i < length; i++) {
        data[i] = SPI_RX(i);
    }
}
