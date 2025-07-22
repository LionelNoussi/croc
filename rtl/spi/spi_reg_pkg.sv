package spi_reg_pkg;
    parameter int AddressWidth = 12;

    typedef struct packed {
        logic [7:0] control;
        logic [7:0] txrx_buffer [0:31];   
        logic [15:0] address;
        logic [7:0] length;
    } spi_reg2hw_t;

    typedef struct packed {
        logic [7:0] status;
    } spi_hw2reg_t;

    parameter logic [AddressWidth-1:0] SPI_CONTROL_OFFSET   = 12'h000;
    parameter logic [AddressWidth-1:0] SPI_STATUS_OFFSET    = 12'h004;
    parameter logic [AddressWidth-1:0] SPI_BUFFER_OFFSET    = 12'h008; // base of txrx_buffer[0]
    parameter logic [AddressWidth-1:0] SPI_ADDRESS_OFFSET   = 12'h028; // 0x008 + 32 bytes
    parameter logic [AddressWidth-1:0] SPI_LENGTH_OFFSET    = 12'h02A; // 0x028 + 2 bytes
endpackage