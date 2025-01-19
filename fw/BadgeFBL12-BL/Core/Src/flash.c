// FBL 12 sponsor badge bootloader
// For STM32F072CBT6
// furrtek 2024

#include "main.h"
#include "flash.h"

uint8_t mcuflash_erase(uint32_t address) {
	FLASH_EraseInitTypeDef pEraseInit;
	uint32_t PageError = 0;

	address += ADDR_APP;

	if (address > FLASH_BANK1_END)
		return 1;

	__HAL_RCC_SYSCFG_CLK_ENABLE();	// Already done ?

	HAL_FLASH_Unlock();
	__HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ALL_ERRORS);
	HAL_FLASH_Lock();

	HAL_FLASH_Unlock();
	pEraseInit.NbPages = 1;
	pEraseInit.PageAddress = address;
	pEraseInit.TypeErase = FLASH_TYPEERASE_PAGES;
	if (HAL_FLASHEx_Erase(&pEraseInit, &PageError) != HAL_OK)
		return 1;
	HAL_FLASH_Lock();
	return 0;
}

uint8_t mcuflash_program(uint32_t address, uint8_t * buffer, const uint32_t size) {
	uint16_t * src_ptr;

	address += ADDR_APP;

	if (address + size > FLASH_BANK1_END)
		return 1;

	__HAL_RCC_SYSCFG_CLK_ENABLE();	// Already done ?

	HAL_FLASH_Unlock();
	__HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ALL_ERRORS);
	HAL_FLASH_Lock();

	// Program flash
	src_ptr = (uint16_t*)buffer;
	HAL_FLASH_Unlock();
	for (uint8_t c = 0; c < ((size + 1) >> 1); c++) {
		if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, address, *src_ptr) == HAL_OK) {
			// Check programmed word
			if (*(__IO uint16_t*)address != *src_ptr) {
				HAL_FLASH_Lock();
				return 1;
			}
			src_ptr++;
			address += 2;
		} else {
			HAL_FLASH_Lock();
			return 1;
		}
	}
	HAL_FLASH_Lock();
	return 0;
}

// Ext flash timing:
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

void SPI_tx(uint8_t data) {
	LL_SPI_TransmitData8(SPI1, data);
	while(LL_SPI_IsActiveFlag_BSY(SPI1)) {};
}

void extflash_WE() {
	// Write enable (0x06)
	LL_GPIO_ResetOutputPin(nFLASH_CS_GPIO_Port, nFLASH_CS_Pin);	// CS low
	SPI_tx(0x06);
	LL_GPIO_SetOutputPin(nFLASH_CS_GPIO_Port, nFLASH_CS_Pin);	// CS high
	ShortWait(2);
}

void extflash_PERASE(uint32_t address) {
	// Page erase (0x81 AA AA aa)
	LL_GPIO_ResetOutputPin(nFLASH_CS_GPIO_Port, nFLASH_CS_Pin);	// CS low
	SPI_tx(0x81);
	SPI_tx(address >> 16);
	SPI_tx(address >> 8);
	SPI_tx(address);
	LL_GPIO_SetOutputPin(nFLASH_CS_GPIO_Port, nFLASH_CS_Pin);	// CS high
}

void extflash_PWRITE(uint32_t address, uint8_t * buf) {
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

void extflash_PREAD(uint32_t address, uint8_t * buf) {
	// Page read (0x03 AA AA aa DD...)
	LL_GPIO_ResetOutputPin(nFLASH_CS_GPIO_Port, nFLASH_CS_Pin);	// CS low
	SPI_tx(0x03);
	SPI_tx(address >> 16);
	SPI_tx(address >> 8);
	SPI_tx(address);
	for (uint16_t d = 0; d < 256; d++) {
		SPI_tx(0x00);	// Data
		*buf++ = LL_SPI_ReceiveData8(SPI1);
	}
	LL_GPIO_SetOutputPin(nFLASH_CS_GPIO_Port, nFLASH_CS_Pin);	// CS high
}

void extflash_program(uint32_t address, uint8_t * buf) {
	extflash_WE();
	extflash_PERASE(address);
	LL_mDelay(12);	// Max page erase time
	extflash_WE();
	extflash_PWRITE(address, buf);
	LL_mDelay(3);	// Max page write time
}
