// FBL 12 sponsor badge V1
// For STM32F072CBT6
// furrtek 2024

#ifndef INC_FLASH_H_
#define INC_FLASH_H_

#define EXTFLASH_SIZE (512 * 1024)
#define REC_MAX_ADDRESS (EXTFLASH_SIZE / 2)

void flash_WE();
void flash_PERASE(uint32_t address);
void flash_PWRITE(uint32_t address, uint8_t * buf);
uint16_t flash_readword(uint32_t address);
void flash_PREAD(uint32_t address, uint8_t * buf);
void flash_program(uint32_t address, uint8_t * buf);

#endif /* INC_FLASH_H_ */
