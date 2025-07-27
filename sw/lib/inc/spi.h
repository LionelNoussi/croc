#pragma once

#include <stdint.h>
#include "config.h"

#define SPI_CTRL        (*(volatile uint8_t *)(SPI_BASE_ADDR + 0x000))
#define SPI_STATUS      (*(volatile uint8_t *)(SPI_BASE_ADDR + 0x004))
#define SPI_TX          (*(volatile uint8_t *)(SPI_BASE_ADDR + 0x008))
#define SPI_RX          (*(volatile uint8_t *)(SPI_BASE_ADDR + 0x00C))
#define SPI_ADDR_LO     (*(volatile uint8_t *)(SPI_BASE_ADDR + 0x010))
#define SPI_ADDR_HI     (*(volatile uint8_t *)(SPI_BASE_ADDR + 0x014))
#define SPI_LENGTH      (*(volatile uint8_t *)(SPI_BASE_ADDR + 0x018))
#define SPI_FIFOSTAT    (*(volatile uint8_t *)(SPI_BASE_ADDR + 0x01C))
#define SPI_MODE_CTRL   (*(volatile uint8_t *)(SPI_BASE_ADDR + 0x020))
#define SPI_BUSY        (*(volatile uint8_t *)(SPI_BASE_ADDR + 0x024))


// SPI read/write interface
void spi_write_full(uint16_t addr, uint8_t *data, uint8_t length);
void spi_read_full(uint16_t addr, uint8_t *data, uint8_t length);
void spi_write(uint16_t addr,uint8_t length);
void spi_read(uint16_t addr, uint8_t length);
void ssd_read(uint8_t* destination_array, uint16_t addr, uint8_t num_bytes);
void ssd_write(uint8_t* source_array, uint16_t addr, uint8_t num_bytes);
void spi_empty_rx();