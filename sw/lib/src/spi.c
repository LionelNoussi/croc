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
// the +3 instead of +4 is intentional, off by 1 error in the module
// SPI_STATUS contains the # of bytes received. It's the total number, so subtract 4 to figure out the actual data bytes received or sent
// SPI_FIFOSTAT contains the information on the TX and RX buffers. It is 8 bit number structured as follows.
//                      READ-BUFFER                                                WRITE-BUFFER      
//       FULL | ALMOST_FULL | EMPTY | ALMOST EMPTY          |        FULL | ALMOST_FULL | EMPTY | ALMOST EMPTY |

// IMPORTANT: Always pull ALL of the received bytes. If you send 10 data bytes and 4 control bytes, make sure that you pull 14 bytes from the rx

// you can change the contents of the SPI in verilator/memory.hex. As of now the lower 8 bits of the address are stored at each address.

// Write data to external memory via SPI
void spi_write_full(uint16_t addr, uint8_t *data, uint8_t length) {
    // Set target address

    SPI_MODE_CTRL = 0b00000000;

    uint8_t addr_hi = (addr >> 8) & 0xFF;  // upper 8 bits
    uint8_t addr_lo = addr & 0xFF; 
    // uint8_t control: 5 bit clock scaling factor, 2 bit read/write (10 write, 01 read), 1 bit ready flag
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


// Write function for only spi control. Packets deposited into TX are sent in FIFO order, 
// make sure the data is available after the 4 Control bytes were sent
void spi_write(uint16_t addr, uint8_t length) {
    // Set target address

    SPI_MODE_CTRL = 0b00000000;

    uint8_t addr_hi = (addr >> 8) & 0xFF;  // upper 8 bits
    uint8_t addr_lo = addr & 0xFF; 
    // uint8_t control = (5 << 3) | (2 << 1) | 0x1;
    uint8_t control = 0b00101101;

    SPI_TX = control;
    // delay_cycles(3);
    SPI_TX = length;
    // delay_cycles(3);
    SPI_TX = addr_hi;
    // delay_cycles(3);
    SPI_TX = addr_lo;
    // delay_cycles(3);

    // Set transfer length
    SPI_LENGTH = length + 3;

    SPI_CTRL = control;
    delay_cycles(3);
    SPI_CTRL = 0b00101100; // reset the ready flag so that just one spi transaction happens

    // Wait until done, you can check the status yourself as described above, 
    // make sure to pull 4+length times from the rx buffer before the next transaction though
    delay_cycles(15); //this delay is necessary because the next read/write otherwise resets the status 
    while (SPI_STATUS != length + 3);
    uint8_t dummy;
    for (uint8_t i= 0; i < 4 + length; i++){
        dummy = SPI_RX;
        delay_cycles(3);
    }
}

//make sure to read out length+4 bytes
void spi_read(uint16_t addr, uint8_t length) {
    // Set target address

    SPI_MODE_CTRL = 0b00000000;

    uint8_t addr_hi = (addr >> 8) & 0xFF;  // upper 8 bits
    uint8_t addr_lo = addr & 0xFF; 
    // uint8_t control = (5 << 3) | (1 << 1) | 0x1;
    uint8_t control = 0b00101011;

    SPI_TX = control;
    // delay_cycles(3);
    SPI_TX = length;
    // delay_cycles(3);
    SPI_TX = addr_hi;
    // delay_cycles(3);
    SPI_TX = addr_lo;
    // delay_cycles(3);

    // Set transfer length
    SPI_LENGTH = length + 3;

    SPI_CTRL = control;
    delay_cycles(3);
    SPI_CTRL = 0b00101010; // reset the ready flag so that just one spi transaction happens
}

// Read data from external memory via SPI
void spi_read_full(uint16_t addr, uint8_t *data, uint8_t length) {

    uint8_t addr_hi = (addr >> 8) & 0xFF;  // upper 8 bits
    uint8_t addr_lo = addr & 0xFF; 
    // uint8_t control = (5 << 3) | (2 << 1) | 0x1;
    uint8_t control = 0b00101011;
    uint8_t control_rst = (5 << 3) | (2 << 1) | 0x0;

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
    delay_cycles(5);
    SPI_CTRL = control_rst;
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



#define SPI_STATUS_RX_EMPTY_MASK 32
void ssd_read(uint8_t* destination_array, uint16_t addr, uint8_t num_bytes) {
    uint8_t status;
    const uint8_t addr_hi = (addr >> 8) & 0xFF;  // upper 8 bits
    const uint8_t addr_lo = addr & 0xFF;
    const uint8_t control_on = (5 << 3) | (1 << 1) | 0x1;
    const uint8_t control_rst = (5 << 3) | (1 << 1) | 0x0;

    // Pre-fill SPI TX Buffer with correct bytes to start SSD protocol without pause
    SPI_TX = control_on;
    SPI_TX = num_bytes;
    SPI_TX = addr_hi;
    SPI_TX = addr_lo;

    // Telling the SPI how many transactions it should do
    // This is done to facilitate unbroken communication
    // If the TX buffer is empty at any point, it will continue to send 0x0
    SPI_LENGTH = num_bytes + 3;
    SPI_CTRL = control_on;
    SPI_CTRL = control_rst;
    
    // Load four first dummy responses
    for (uint8_t i = 0; i < 4; i++) {

        // Stall while, RX buffer is empty
        while (SPI_FIFOSTAT & SPI_STATUS_RX_EMPTY_MASK);

        SPI_RX;
    }

    for (uint8_t i = 0; i < num_bytes; i++) {
        // Stall while, RX buffer is empty
        while (SPI_FIFOSTAT & SPI_STATUS_RX_EMPTY_MASK);

        // Store result in destination array
        destination_array[i] = SPI_RX;
    }
}

# define SPI_STATUS_TX_ALMOST_FULL 3
void ssd_write(uint8_t* source_array, uint16_t addr, uint8_t num_bytes) {
    uint8_t status, dummy;
    const uint8_t addr_hi = (addr >> 8) & 0xFF;  // upper 8 bits
    const uint8_t addr_lo = addr & 0xFF;
    const uint8_t control_on = (5 << 3) | (2 << 1) | 0x1;
    const uint8_t control_rst = (5 << 3) | (2 << 1) | 0x0;

    // Pre-fill SPI TX Buffer with correct bytes to start SSD protocol without pause
    SPI_TX = control_on;
    SPI_TX = num_bytes;
    SPI_TX = addr_hi;
    SPI_TX = addr_lo;

    // Telling the SPI how many transactions it should do
    // This is done to facilitate unbroken communication
    // If the TX buffer is empty at any point, it will continue to send 0x0
    SPI_LENGTH = num_bytes + 3;
    SPI_CTRL = control_on;
    SPI_CTRL = control_rst;
    
    // // Load four first dummy responses
    // for (uint8_t i = 0; i < 4; i++) {

    //     // Stall while, RX buffer is empty
    //     while (SPI_FIFOSTAT & SPI_STATUS_RX_EMPTY_MASK);

    //     dummy = SPI_RX;
    // }

    for (uint8_t i = 0; i < num_bytes; i++) {
        // Stall while, TX buffer full
        while (!(SPI_FIFOSTAT & SPI_STATUS_TX_ALMOST_FULL));

        // Write source array into SPI TX
        SPI_TX =  source_array[i];
    }
    //need to also check if the com
    do {
        while (!(SPI_FIFOSTAT & SPI_STATUS_RX_EMPTY_MASK)) {
            dummy = SPI_RX;
        }
    } while(SPI_STATUS < num_bytes + 4);
    dummy = SPI_RX;
}

// clears the buffer only once, if you want to clear it after a write you need to wait for the write to be finished
void spi_empty_rx(){
    uint8_t dummy;
    while (!(SPI_FIFOSTAT & SPI_STATUS_RX_EMPTY_MASK)) {
            dummy = SPI_RX;
    }
}