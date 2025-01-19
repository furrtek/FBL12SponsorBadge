// FBL 12 sponsor badge V1
// For STM32F072CBT6
// furrtek 2024

#ifndef INC_MISC_H_
#define INC_MISC_H_

#include "main.h"
#include "misc.h"

extern uint8_t disp_brightness;

uint8_t read_batt();
uint8_t is_charging();
void led_on(uint8_t led_mask);
void led_off(uint8_t led_mask);
void led_blink(uint8_t led_mask);
uint16_t BSDChecksum(const char * src);
uint32_t XORShift();
void set_brightness(uint8_t v);
void ShortWait(uint32_t wait);

#endif /* INC_MISC_H_ */
