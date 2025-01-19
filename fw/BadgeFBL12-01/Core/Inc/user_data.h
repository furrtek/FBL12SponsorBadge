// FBL 12 sponsor badge V1
// For STM32F072CBT6
// furrtek 2024

#ifndef INC_USER_DATA_H_
#define INC_USER_DATA_H_

#define USER_DATA_ADDRESS (FLASH_BASE + (63 * 2048))
#define FLASH_FLAG_ALL_ERRORS (FLASH_FLAG_EOP | FLASH_FLAG_WRPERR | FLASH_FLAG_PGERR)

typedef struct {
	 uint16_t user_id;			// Badge number
	 char name[17];
	 char species_a[17];
	 char species_b[17];
	 // Padding byte
	 uint16_t fav_id;
	 uint8_t react_boop;		// [3:0]:Sample slot, [7:4]:Pitch(0~4)
	 uint8_t react_lear;		// [3:0]:Sample slot, [7:4]:Pitch(0~4)
	 uint8_t react_rear;		// [3:0]:Sample slot, [7:4]:Pitch(0~4)
	 uint8_t react_alert;		// [3:0]:Sample slot, [7:4]:Pitch(0~4)
	 uint8_t flags;				// Unused for now
	 //sound_level_t sound_level;	// Unused for now
} user_data_t;

//extern const user_data_t __attribute__((section (".user_data"))) user_data;
extern const user_data_t * user_data_ptr;

uint8_t user_data_valid();
void set_id(const uint16_t user_id);
void set_name(uint8_t length, char * buffer);
void set_species(const uint8_t slot, const char * str);
void set_react_boop(const uint8_t react_boop, const uint8_t pitch);
void set_react_lear(const uint8_t react_boop, const uint8_t pitch);
void set_react_rear(const uint8_t react_boop, const uint8_t pitch);
void set_react_alert(const uint8_t react_boop, const uint8_t pitch);

#endif /* INC_USER_DATA_H_ */
