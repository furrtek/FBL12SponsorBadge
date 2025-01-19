// FBL 12 sponsor badge V1
// For STM32F072CBT6
// furrtek 2024

#include "main.h"
#include "misc.h"

uint8_t read_batt() {
	LL_ADC_REG_StopConversion(ADC1);	// Just in case, the rest fails if a conversion is ongoing
	LL_ADC_Disable(ADC1);
	LL_ADC_SetResolution(ADC1, LL_ADC_RESOLUTION_12B);
	LL_ADC_REG_SetTriggerSource(ADC1, LL_ADC_REG_TRIG_SOFTWARE);
	LL_ADC_REG_SetSequencerChannels(ADC1, LL_ADC_CHANNEL_VBAT);
	LL_ADC_DisableIT_EOC(ADC1);
	LL_ADC_Enable(ADC1);

	uint32_t mean = 0;
	for (uint8_t c = 0; c < 16; c++) {
		LL_ADC_REG_StartConversion(ADC1);
		while (!LL_ADC_IsActiveFlag_EOC(ADC1)) {};
		LL_ADC_ClearFlag_EOC(ADC1);

		// 4.2V max / 2 = 2.1V on VBAT, max val: 1302
		// min: 3.2V / 2 = 1.6V on VBAT, val: 992
		mean += (uint32_t)LL_ADC_REG_ReadConversionData12(ADC1);
		LL_ADC_REG_StopConversion(ADC1);	// Just in case, the rest fails if a conversion is ongoing
	}
	mean >>= 4;
	uint16_t temp = mean;

	// DEBUG
	char str_buffer[16];
	sprintf(str_buffer, "Vb: %d\n", temp);
	uart_print(str_buffer);

	if (temp < 1510)
		return 0;
	temp = ((temp - 1510) * 195) >> 8;
	if (temp > 99)
		return 99;

	return (uint8_t)temp;
}

uint8_t is_charging() {
	return (LL_GPIO_IsInputPinSet(nCHRG_GPIO_Port, nCHRG_Pin)) ? 0 : 1;
}

void led_on(uint8_t led_mask) {
	blink_mask &= ~led_mask;
	disp_buf[8] |= led_mask;
}

void led_off(uint8_t led_mask) {
	blink_mask &= ~led_mask;
	disp_buf[8] &= ~led_mask;
}

void led_blink(uint8_t led_mask) {
	blink_mask |= led_mask;
}

// Used to generate a 16-bit "hash" from the species strings
uint16_t BSDChecksum(const char * src) {
	uint16_t checksum = 0;
	char c;
	uint8_t i = 0;

	while ((c = *src++) && ((i++) < 16)) {
		checksum = (checksum << 15) + (checksum >> 1);	// Rotate right
		checksum += c;
	}

	return checksum;
}

uint32_t XORShift() {
	xs_state ^= xs_state << 13;
	xs_state ^= xs_state >> 17;
	xs_state ^= xs_state << 5;
	return xs_state;
}

uint8_t disp_brightness;

void set_brightness(uint8_t v) {
	disp_brightness = v;
	LL_TIM_OC_SetCompareCH1(TIM14, ((uint16_t)v * 1000) >> 8);
}

void ShortWait(uint32_t wait) {
	volatile uint32_t wait_loop_index = wait;	// Units of ~10us at 12MHz
	while (wait_loop_index) {
		wait_loop_index--;
	}
}
