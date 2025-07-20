package spi_reg_pkg;
    parameter int AddressWidth = 12;

    typedef struct packed {
        logic [7:0] control;
        logic [7:0] tx_data;
    } spi_reg2hw_t;

    typedef struct packed {
        logic [7:0] status;
        logic [7:0] rx_data;
    } spi_hw2reg_t;

    parameter logic [AddressWidth-1:0] SPI_CONTROL_OFFSET  = 12'h000;
    parameter logic [AddressWidth-1:0] SPI_STATUS_OFFSET   = 12'h004;
    parameter logic [AddressWidth-1:0] SPI_TXDATA_OFFSET   = 12'h008;
    parameter logic [AddressWidth-1:0] SPI_RXDATA_OFFSET   = 12'h00C;
endpackage