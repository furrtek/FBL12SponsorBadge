// FBL 12 sponsor badge bootloader
// For STM32F072CBT6
// furrtek 2024

#ifndef INC_FLASH_H_
#define INC_FLASH_H_

#define EXTFLASH_SIZE (512 * 1024)
#define FLASH_FLAG_ALL_ERRORS (FLASH_FLAG_EOP | FLASH_FLAG_WRPERR | FLASH_FLAG_PGERR)

void extflash_WE();
void extflash_PERASE(uint32_t address);
void extflash_PWRITE(uint32_t address, uint8_t * buf);
void extflash_PREAD(uint32_t address, uint8_t * buf);
void extflash_program(uint32_t address, uint8_t * buf);

uint8_t mcuflash_erase(uint32_t address);
uint8_t mcuflash_program(uint32_t address, uint8_t * buffer, const uint32_t size);

#endif /* INC_FLASH_H_ */
