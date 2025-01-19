// FBL 12 sponsor badge bootloader
// For STM32F072CBT6
// furrtek 2024

#include <string.h>
#include "main.h"
#include "user_data.h"
#include "flash.h"

// Linker file should make this appear at 0x0801f000
// However, initializing this here will produce a 128kB binary with a lot of empty space
/*const user_data_t __attribute__((section (".user_data"))) user_data = {
	1234,
	"BREK",
	"WOLF",
	"",
	0,
	SOUND_LVL_1,
	BOOP_PREDEF,
	0
};*/
const user_data_t * user_data_ptr = (user_data_t*)0x0801f800;

uint8_t user_data_valid() {
	return (user_data_ptr->user_id != 0xFFFF) ? 1 : 0;
}

void set_id(const uint16_t user_id) {
	user_data_t user_data_temp;

	memcpy(&user_data_temp, user_data_ptr, sizeof(user_data_t));
	mcuflash_erase(USER_DATA_ADDRESS);
	user_data_temp.user_id = user_id;
	mcuflash_program(USER_DATA_ADDRESS, (uint8_t*)&user_data_temp, sizeof(user_data_t));
}

void set_name(uint8_t length, char * buffer) {
	user_data_t user_data_temp;

	memcpy(&user_data_temp, user_data_ptr, sizeof(user_data_t));
	mcuflash_erase(USER_DATA_ADDRESS);
	strncpy(user_data_temp.name, buffer, 16);
	user_data_temp.name[length] = 0;	// Null terminator
	mcuflash_program(USER_DATA_ADDRESS, (uint8_t*)&user_data_temp, sizeof(user_data_t));
}

void set_species(const uint8_t slot, const char * str) {
	user_data_t user_data_temp;
	char * dst = slot ? user_data_temp.species_b : user_data_temp.species_a;

	memcpy(&user_data_temp, user_data_ptr, sizeof(user_data_t));
	mcuflash_erase(USER_DATA_ADDRESS);
	strncpy(dst, str, 16);
	dst[16] = 0;
	mcuflash_program(USER_DATA_ADDRESS, (uint8_t*)&user_data_temp, sizeof(user_data_t));
}
