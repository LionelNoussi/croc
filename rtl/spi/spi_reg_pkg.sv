package spi_reg_pkg;

  parameter int AddressWidth = 12;


  typedef struct packed{
    logic [7:0] control;
    logic [8*16-1:0] tx_data;   // 16-byte TX buffer
    logic [7:0] address_low;
    logic [7:0] address_high;
    logic [7:0] length;
  } spi_reg2hw_t;

  // Writeable-by-HW
  typedef struct packed{
    logic [7:0] status;
    logic [8*16-1:0] rx_data;   // 16-byte RX buffer
  } spi_hw2reg_t;

  // Register Offsets
    parameter logic [AddressWidth-1:0] SPI_CONTROL_OFFSET     = 12'h000;
    parameter logic [AddressWidth-1:0] SPI_STATUS_OFFSET      = 12'h004;
    parameter logic [AddressWidth-1:0] SPI_TXBUFFER_OFFSET    = 12'h008;  // 0x008 - 0x017
    parameter logic [AddressWidth-1:0] SPI_RXBUFFER_OFFSET    = 12'h018;  // 0x018 - 0x027
    parameter logic [AddressWidth-1:0] SPI_ADDRESS_LO_OFFSET  = 12'h028;
    parameter logic [AddressWidth-1:0] SPI_ADDRESS_HI_OFFSET  = 12'h029;
    parameter logic [AddressWidth-1:0] SPI_LENGTH_OFFSET      = 12'h02C;


endpackage
