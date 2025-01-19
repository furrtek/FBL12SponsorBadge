// FBL 12 sponsor badge V1
// For STM32F072CBT6
// furrtek 2024

#ifndef INC_TESTMODE_H_
#define INC_TESTMODE_H_

extern uint8_t test_mode;
extern uint8_t test_mode_timer;
extern uint8_t test_mode_step;

void enter_testmode(const uint8_t mode);
void handle_testmode();

#endif /* INC_TESTMODE_H_ */
