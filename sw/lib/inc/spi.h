#pragma once


#include <stdint.h>
#include "config.h"

#define SPI_BASE_ADDR      0x0300C000
#define SPI_TXDATA         (*(volatile uint32_t *)(SPI_BASE_ADDR + 0x00))
#define SPI_RXDATA         (*(volatile uint32_t *)(SPI_BASE_ADDR + 0x04))
#define SPI_CTRL           (*(volatile uint32_t *)(SPI_BASE_ADDR + 0x08))
#define SPI_STATUS         (*(volatile uint32_t *)(SPI_BASE_ADDR + 0x0C))


void spi_write(uint8_t data);