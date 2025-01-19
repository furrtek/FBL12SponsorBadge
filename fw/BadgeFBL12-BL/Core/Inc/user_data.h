// FBL 12 sponsor badge bootloader
// For STM32F072CBT6
// furrtek 2024

#ifndef INC_USER_DATA_H_
#define INC_USER_DATA_H_

#define USER_DATA_ADDRESS (63 * 2048)

typedef enum {
	SOUND_LVL_OFF = 0,
	SOUND_LVL_1 = 1,
	SOUND_LVL_2 = 2,
	SOUND_LVL_3 = 3
} sound_level_t;

typedef enum {
	BOOP_OFF = 0,
	BOOP_PREDEF,
	BOOP_REC_A,
	BOOP_REC_RAND
} boop_type_t;

typedef struct {
	 uint16_t user_id;			// Badge number
	 char name[17];
	 char species_a[17];
	 char species_b[17];
	 uint16_t fav_id;
	 sound_level_t sound_level;
	 boop_type_t boop_type;		// Predef sample, or recorded
	 uint8_t flags;				// Boop enable, fav enable...
} user_data_t;

//extern const user_data_t __attribute__((section (".user_data"))) user_data;
extern const user_data_t * user_data_ptr;

uint8_t user_data_valid();
void set_id(const uint16_t user_id);
void set_name(uint8_t length, char * buffer);
void set_species(const uint8_t slot, const char * str);

#endif /* INC_USER_DATA_H_ */
