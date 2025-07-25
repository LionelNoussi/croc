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

// warning: for old spi.sv module, the amount of packets to be sent must be 1 lower then the intended total
// Write data to external memory via SPI
void spi_write(uint16_t addr, uint8_t *data, uint8_t length) {
    // Set target address

    uint8_t addr_hi = (addr >> 8) & 0xFF;  // upper 8 bits
    uint8_t addr_lo = addr & 0xFF; 
    // uint8_t control = (5 << 3) | (2 << 1) | 0x1;
    uint8_t control = 0b00101101;
    uint8_t control_rst = (length << 3) | (2 << 1) | 0x0;

    SPI_TX = control;
    // delay_cycles(3);
    SPI_TX = length;
    // delay_cycles(3);
    SPI_TX = addr_hi;
    // delay_cycles(3);
    SPI_TX = addr_lo;
    // delay_cycles(3);
    for (uint8_t i = 0; i < length; i++) {
        SPI_TX = data[i];
        // delay_cycles(3);
    }

    // Set transfer length
    SPI_LENGTH = length + 3;

    // Start write: [7:3]=length, [2:1]=0b10 (write), [0]=1 (start)
    SPI_CTRL = control;
    delay_cycles(3);
    SPI_CTRL = 0b00101100;

    // Wait until done
    delay_cycles(15);
    while (SPI_STATUS != length + 3);
    uint8_t dummy;
    for (uint8_t i= 0; i < 4; i++){
        dummy = SPI_RX;
        delay_cycles(3);
    }
    for(uint8_t i = 0; i < length; i++){
        uint8_t status = SPI_FIFOSTAT;
        // uint8_t stat = SPI_STATUS
        // if(status & )
        data[i] = SPI_RX;
        delay_cycles(3);
    }
}

// bool data_ready (){
    
// }
// Read data from external memory via SPI
void spi_read(uint16_t addr, uint8_t *data, uint8_t length) {

    uint8_t addr_hi = (addr >> 8) & 0xFF;  // upper 8 bits
    uint8_t addr_lo = addr & 0xFF; 
    // uint8_t control = (5 << 3) | (2 << 1) | 0x1;
    uint8_t control = 0b00101011;
    uint8_t control_rst = (4 << 3) | (2 << 1) | 0x0;

    SPI_TX = control;
    // delay_cycles(2);
    SPI_TX = length;
    // delay_cycles(2);
    SPI_TX = addr_hi;
    // delay_cycles(2);
    SPI_TX = addr_lo;
    // delay_cycles(2);
    // for (uint8_t i = 0; i < length; i++) {
    //     SPI_TX = data[i];
    //     delay_cycles(2000);
    // }

    // Set transfer length
    SPI_LENGTH = length + 3;

    // Start write: [7:3]=length, [2:1]=0b10 (write), [0]=1 (start)
    SPI_CTRL = control;
    SPI_LENGTH = length + 3;
    SPI_CTRL = 0b00101010;
    delay_cycles(15);
    // Wait until done
    while (SPI_STATUS != length + 3);
    uint8_t dummy;
    for (uint8_t i = 0; i < 4; i++){
        dummy = SPI_RX;
        // delay_cycles(2);
    }
    for(uint8_t i = 0; i < length; i++){
        uint8_t status = SPI_FIFOSTAT;
        // uint8_t stat = SPI_STATUS
        // if(status & )
        data[i] = SPI_RX;
        // delay_cycles(2);
    }
}


