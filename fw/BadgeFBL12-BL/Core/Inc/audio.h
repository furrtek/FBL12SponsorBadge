// FBL 12 sponsor badge bootloader
// For STM32F072CBT6
// furrtek 2024

#ifndef INC_AUDIO_H_
#define INC_AUDIO_H_

extern volatile uint16_t beep_delta, beep_timer, beep_acc;

void beep(const uint16_t delta, const uint16_t duration);
void Disable_Amp();
void Enable_Amp();

#endif /* INC_AUDIO_H_ */
