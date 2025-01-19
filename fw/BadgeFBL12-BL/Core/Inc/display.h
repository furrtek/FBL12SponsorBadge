// FBL 12 sponsor badge bootloader
// For STM32F072CBT6
// furrtek 2024

#ifndef INC_DISPLAY_H_
#define INC_DISPLAY_H_

void PrintChar(uint8_t x, const char c);
void PrintTxt(uint8_t x, const char * src);
void PrintNum(uint16_t v);
void PrintHex(const uint8_t x, uint8_t v);
void ClearDisp();
void PrintSeg(const uint8_t v);

#endif /* INC_DISPLAY_H_ */
