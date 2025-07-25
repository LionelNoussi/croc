package spi_reg_pkg;

  parameter int AddressWidth = 12;


  typedef struct packed{
    logic [7:0] control;
    logic [7:0] tx_data;   // 16-byte TX buffer
    logic [7:0] address_low;
    logic [7:0] address_high;
    logic [7:0] length;
    logic [7:0] mode_ctrl;
  } spi_reg2hw_t;


  typedef struct packed{
    logic [7:0] status;
    logic [7:0] fifo_status;
    logic [7:0] rx_data;   // 16-byte RX buffer
  } spi_hw2reg_t;

  // Register Offsets
  parameter logic [AddressWidth-1:0] SPI_CONTROL_OFFSET     = 12'h000;
  parameter logic [AddressWidth-1:0] SPI_STATUS_OFFSET      = 12'h004;
  parameter logic [AddressWidth-1:0] SPI_TXBUFFER_OFFSET    = 12'h008; // write one byte at a time
  parameter logic [AddressWidth-1:0] SPI_RXBUFFER_OFFSET    = 12'h00C; // read one byte at a time
  parameter logic [AddressWidth-1:0] SPI_ADDRESS_LO_OFFSET  = 12'h010;
  parameter logic [AddressWidth-1:0] SPI_ADDRESS_HI_OFFSET  = 12'h014;
  parameter logic [AddressWidth-1:0] SPI_LENGTH_OFFSET      = 12'h018;
  parameter logic [AddressWidth-1:0] SPI_FIFOSTAT_OFFSET    = 12'h01C;
  parameter logic [AddressWidth-1:0] SPI_MODE_CTRL_OFFSET   = 12'h020;


endpackage
