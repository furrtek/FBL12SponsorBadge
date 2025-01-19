// FBL 12 sponsor badge V1
// For STM32F072CBT6
// furrtek 2024

#ifndef INC_LUTS_H_
#define INC_LUTS_H_

extern const uint8_t char_lut[64];
extern const uint16_t key_lut[16];
extern const char key_name_lut[12][3];
extern const uint16_t dig_lut[9];
extern const uint16_t tone_lut[(3 * 12) + 1];
extern const uint8_t sin_lut[256];
extern const uint8_t lpf_lut[256];

#define SYMBOL_DOWN 14
#define SYMBOL_UP 62

#endif /* INC_LUTS_H_ */
