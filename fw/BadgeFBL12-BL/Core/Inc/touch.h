// FBL 12 sponsor badge bootloader
// For STM32F072CBT6
// furrtek 2024

#ifndef INC_TOUCH_H_
#define INC_TOUCH_H_

#define TOUCH_A ((0 << 2) + 0)
#define TOUCH_OK ((0 << 2) + 1)
#define TOUCH_LEAR ((1 << 2) + 0)
#define TOUCH_NOSE ((1 << 2) + 1)
#define TOUCH_REAR ((2 << 2) + 0)
#define TOUCH_B ((2 << 2) + 1)

extern volatile uint16_t touch_val[12];
extern const uint16_t touch_thr[12];
extern uint8_t touched[12];
extern uint8_t touched_prev[12];
extern uint8_t touched_rise[12];

#endif /* INC_TOUCH_H_ */
