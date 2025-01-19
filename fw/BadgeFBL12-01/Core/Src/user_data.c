// FBL 12 sponsor badge V1
// For STM32F072CBT6
// furrtek 2024

#include <string.h>
#include "main.h"
#include "user_data.h"

// Linker file should make this appear at 0x0801f000
// However, initializing this here will produce a 128kB binary with lots of empty space
/*const user_data_t __attribute__((section (".user_data"))) user_data = {
	1234,
	"BREK",
	"WOLF",...
};*/
const user_data_t * user_data_ptr = (user_data_t*)0x0801f800;

uint8_t user_data_valid() {
	return (user_data_ptr->user_id != 0xFFFF) ? 1 : 0;
}

void erase_user_data() {
	uint32_t PageError = 0;
	FLASH_EraseInitTypeDef pEraseInit;

	__HAL_RCC_SYSCFG_CLK_ENABLE();	// Already done ?

	HAL_FLASH_Unlock();
	__HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ALL_ERRORS);
	HAL_FLASH_Lock();

	// Erase the last page
	HAL_FLASH_Unlock();
	pEraseInit.NbPages = 1;
	pEraseInit.PageAddress = USER_DATA_ADDRESS;
	pEraseInit.TypeErase = FLASH_TYPEERASE_PAGES;
	if (HAL_FLASHEx_Erase(&pEraseInit, &PageError) != HAL_OK)
		return;	// Error :(
	HAL_FLASH_Lock();
}

void program_user_data(user_data_t * user_data_ptr) {
	uint16_t * src_ptr;
	uint32_t dest_addr;

	__HAL_RCC_SYSCFG_CLK_ENABLE();	// Already done ?

	HAL_FLASH_Unlock();
	__HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ALL_ERRORS);
	HAL_FLASH_Lock();

	// Program flash
	src_ptr = (uint16_t*)user_data_ptr;
	dest_addr = USER_DATA_ADDRESS;
	HAL_FLASH_Unlock();
	for (uint8_t c = 0; c < ((sizeof(user_data_t) + 1) >> 1); c++) {
		if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, dest_addr, *src_ptr) == HAL_OK) {
			// Check programmed word
			if (*(__IO uint16_t*)dest_addr != *src_ptr) {
				HAL_FLASH_Lock();
				return;	// Error :(
			}
			src_ptr++;
			dest_addr += 2;	// Halfword
		} else {
			HAL_FLASH_Lock();
			return;	// Error :(
		}
	}
	HAL_FLASH_Lock();
}

void set_id(const uint16_t user_id) {
	user_data_t user_data_temp;

	memcpy(&user_data_temp, user_data_ptr, sizeof(user_data_t));
	erase_user_data();
	user_data_temp.user_id = user_id;
	program_user_data(&user_data_temp);
}

void set_name(uint8_t length, char * buffer) {
	user_data_t user_data_temp;

	memcpy(&user_data_temp, user_data_ptr, sizeof(user_data_t));
	erase_user_data();
	strncpy(user_data_temp.name, buffer, 16);
	user_data_temp.name[length] = 0;	// Ensure null terminator presence
	program_user_data(&user_data_temp);
}

void set_species(const uint8_t slot, const char * str) {
	user_data_t user_data_temp;
	char * dst = slot ? user_data_temp.species_b : user_data_temp.species_a;

	memcpy(&user_data_temp, user_data_ptr, sizeof(user_data_t));
	erase_user_data();
	strncpy(dst, str, 16);
	dst[16] = 0;
	program_user_data(&user_data_temp);
}

void set_react_boop(const uint8_t sample_slot, const uint8_t pitch) {
	user_data_t user_data_temp;

	memcpy(&user_data_temp, user_data_ptr, sizeof(user_data_t));
	erase_user_data();
	user_data_temp.react_boop = sample_slot | (pitch << 4);
	program_user_data(&user_data_temp);
}

void set_react_lear(const uint8_t sample_slot, const uint8_t pitch) {
	user_data_t user_data_temp;

	memcpy(&user_data_temp, user_data_ptr, sizeof(user_data_t));
	erase_user_data();
	user_data_temp.react_lear = sample_slot | (pitch << 4);
	program_user_data(&user_data_temp);
}

void set_react_rear(const uint8_t sample_slot, const uint8_t pitch) {
	user_data_t user_data_temp;

	memcpy(&user_data_temp, user_data_ptr, sizeof(user_data_t));
	erase_user_data();
	user_data_temp.react_rear = sample_slot | (pitch << 4);
	program_user_data(&user_data_temp);
}

void set_react_alert(const uint8_t sample_slot, const uint8_t pitch) {
	user_data_t user_data_temp;

	memcpy(&user_data_temp, user_data_ptr, sizeof(user_data_t));
	erase_user_data();
	user_data_temp.react_alert = sample_slot | (pitch << 4);
	program_user_data(&user_data_temp);
}
