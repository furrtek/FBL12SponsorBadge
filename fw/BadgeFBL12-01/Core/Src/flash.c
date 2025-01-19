// FBL 12 sponsor badge V1
// For STM32F072CBT6
// furrtek 2024

#include "main.h"
#include "flash.h"

// Ext flash timing
// 6MHz SPI
// 256-byte pages
// 8ms uniform erase time for page or 4kB sectors (64 bytes @ 8000Hz)
//	Max 12ms (96 bytes @ 8000Hz)
// 1.6ms page program time (13 bytes @ 8000Hz)
//	Max 2.5ms (20 bytes @ 8000Hz)
// To program 256 bytes (32ms @ 8000Hz):
//	Page erase command: 4 * 8 * 166.7ns = 5.3us
//	Page erase: 8ms
//	Transfer: (4 + 256) * 8 * 166.7ns = 347us
//	Page program: 1.6ms
//	Total: 9.95ms (31% of 32ms)
//	Max: 14.9ms (47% of 32ms)

// Buffer fill		Operation
// 0				Start page erase
// 64				Page erase done, start transfer
// 67				Transfer done, start page program
// 80				Page program done
// ...255

uint8_t SPI_tx(uint8_t data) {
	while (LL_SPI_GetRxFIFOLevel(SPI1)) {
		LL_SPI_ReceiveData8(SPI1);	// Empty RXFIFO
	}
	LL_SPI_TransmitData8(SPI1, data);
	while(LL_SPI_IsActiveFlag_BSY(SPI1)) {};
	return LL_SPI_ReceiveData8(SPI1);
}

void flash_WE() {
	// Write enable (0x06)
	LL_GPIO_ResetOutputPin(nFLASH_CS_GPIO_Port, nFLASH_CS_Pin);	// CS low
	SPI_tx(0x06);
	LL_GPIO_SetOutputPin(nFLASH_CS_GPIO_Port, nFLASH_CS_Pin);	// CS high
	ShortWait(2);
}

void flash_PERASE(uint32_t address) {
	// Page erase (0x81 AA AA aa)
	LL_GPIO_ResetOutputPin(nFLASH_CS_GPIO_Port, nFLASH_CS_Pin);	// CS low
	SPI_tx(0x81);
	SPI_tx(address >> 16);
	SPI_tx(address >> 8);
	SPI_tx(address);
	LL_GPIO_SetOutputPin(nFLASH_CS_GPIO_Port, nFLASH_CS_Pin);	// CS high
}

void flash_PWRITE(uint32_t address, uint8_t * buf) {
	// Page program (0x02 AA AA aa DD...)
	LL_GPIO_ResetOutputPin(nFLASH_CS_GPIO_Port, nFLASH_CS_Pin);	// CS low
	SPI_tx(0x02);
	SPI_tx(address >> 16);
	SPI_tx(address >> 8);
	SPI_tx(address);
	for (uint16_t d = 0; d < 256; d++)
		SPI_tx(*buf++);	// Data
	LL_GPIO_SetOutputPin(nFLASH_CS_GPIO_Port, nFLASH_CS_Pin);	// CS high
}

uint16_t flash_readword(uint32_t address) {
	uint8_t temp_buf[2];

	// Page read (0x03 AA AA aa DD...)
	LL_GPIO_ResetOutputPin(nFLASH_CS_GPIO_Port, nFLASH_CS_Pin);	// CS low
	SPI_tx(0x03);
	SPI_tx(address >> 16);
	SPI_tx(address >> 8);
	SPI_tx(address);
	temp_buf[0] = SPI_tx(0x00);	// Data
	temp_buf[1] = SPI_tx(0x00);	// Data
	LL_GPIO_SetOutputPin(nFLASH_CS_GPIO_Port, nFLASH_CS_Pin);	// CS high

	return *(uint16_t*)temp_buf;
}

void flash_PREAD(uint32_t address, uint8_t * buf) {
	// Page read (0x03 AA AA aa DD...)
	LL_GPIO_ResetOutputPin(nFLASH_CS_GPIO_Port, nFLASH_CS_Pin);	// CS low
	SPI_tx(0x03);
	SPI_tx(address >> 16);
	SPI_tx(address >> 8);
	SPI_tx(address);
	for (uint16_t d = 0; d < 256; d++) {
		*buf++ = SPI_tx(0x00);	// Data
	}
	LL_GPIO_SetOutputPin(nFLASH_CS_GPIO_Port, nFLASH_CS_Pin);	// CS high
}

void flash_program(uint32_t address, uint8_t * buf) {
	flash_WE();
	flash_PERASE(address);
	LL_mDelay(12);	// Max page erase time
	flash_WE();
	flash_PWRITE(address, buf);
	LL_mDelay(3);	// Max page write time
}
