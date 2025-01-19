// FBL 12 sponsor badge bootloader
// For STM32F072CBT6
// furrtek 2024

#include "main.h"
#include "audio.h"
#include "luts.h"

volatile uint16_t beep_delta, beep_timer = 0, beep_acc = 0;

void beep(const uint16_t delta, const uint16_t duration) {
	Enable_Amp();
	beep_delta = delta;
	beep_timer = duration;
}

void Disable_Amp() {
#if AMPSD_POLARITY_POS
	LL_GPIO_SetOutputPin(AMPSD_GPIO_Port, AMPSD_Pin);
#else
	LL_GPIO_ResetOutputPin(AMPSD_GPIO_Port, AMPSD_Pin);
#endif
}

void Enable_Amp() {
#if AMPSD_POLARITY_POS
	LL_GPIO_ResetOutputPin(AMPSD_GPIO_Port, AMPSD_Pin);
#else
	LL_GPIO_SetOutputPin(AMPSD_GPIO_Port, AMPSD_Pin);
#endif
}
