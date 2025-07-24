#include "spi.h"
#include "util.h"
#include "config.h"
#include <stdio.h>
#include <stdint.h>

static inline void delay_cycles(volatile uint32_t cycles) {
    while (cycles--) {
        __asm__ volatile ("nop");
    }
}


// Write data to external memory via SPI
void spi_write(uint16_t addr, const uint8_t *data, uint8_t length) {
    // Set target address
    SPI_ADDR_HI = (addr >> 8) & 0xFF;
    SPI_ADDR_LO = addr & 0xFF;

    for (uint8_t i = 0; i < length; i++) {
        SPI_TX = data[i];
    }

    // Set transfer length
    SPI_LENGTH = length;

    length = 5;
    // Start write: [7:3]=length, [2:1]=0b10 (write), [0]=1 (start)
    SPI_CTRL = (length << 3) | (2 << 1) | 0x1;
    SPI_LENGTH = length;
    SPI_CTRL = (length << 3) | (2 << 1) | 0x0;

    // Wait until done
    while (SPI_STATUS != length);
    uint8_t read = SPI_RX;
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

    uint8_t status = SPI_STATUS;
    
    
    SPI_CTRL = (length << 3) | (1 << 1) | 0x0;

    // Wait until done
    while ((SPI_STATUS & 0x1) == 0);

    // Read from RX buffer
    for (uint8_t i = 0; i < length; i++) {
        data[i] = SPI_RX;
        // delay_cycles(10000);
    }

    for (uint8_t i = 0; i< length; i++)  {
        SPI_TX = data[i];
    }

    
}


