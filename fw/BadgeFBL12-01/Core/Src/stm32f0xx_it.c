// FBL 12 sponsor badge V1
// For STM32F072CBT6
// furrtek 2024

#include "main.h"
#include "stm32f0xx_it.h"
#include "luts.h"
#include "audio.h"
#include "touch.h"
#include "ir.h"
#include "misc.h"

volatile uint8_t flag_update_sr = 0;
volatile uint16_t scan_counter = 0;
volatile uint8_t touch_io = 0;

void NMI_Handler(void) {
	while (1) { }
}

void HardFault_Handler(void) {
	while (1) { }
}

void SVC_Handler(void) {
}

void PendSV_Handler(void) {
}

void SysTick_Handler(void) {
	HAL_IncTick();
}

void EXTI4_15_IRQHandler(void) {
	// Proto B: PB5 edge
	if (LL_EXTI_IsActiveFlag_0_31(LL_EXTI_LINE_5)) {
		irrx_irq_handler();
		LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_5);
	}
}

void SPI2_IRQHandler(void) {
	// The TXE interrupt triggers when the TX FIFO is empty, not when the transfer is done
	// Just polling the busy flag might be enough without using the TXE interrupt
	if (LL_SPI_IsActiveFlag_TXE(SPI2)) {
		while(LL_SPI_IsActiveFlag_BSY(SPI2)) { };	// Poll busy flag to wait for the end of the transfer
		LL_GPIO_SetOutputPin(RCLK_GPIO_Port, RCLK_Pin);	// Rising edge on RCLK, update SR outputs

		if (scan_counter == 8) {
			LL_TIM_OC_SetCompareCH1(TIM14, 1000);
			LL_GPIO_ResetOutputPin(DIG8_GPIO_Port, DIG8_Pin);	// Enable individual LEDs
			scan_counter = 0;
		} else {
			if (scan_counter == 0)
				set_brightness(disp_brightness);
			LL_GPIO_SetOutputPin(DIG8_GPIO_Port, DIG8_Pin);		// Disable individual LEDs
			scan_counter++;
		}

		LL_SPI_DisableIT_TXE(SPI2);
	}
}

extern PCD_HandleTypeDef hpcd_USB_FS;

void ADC1_COMP_IRQHandler(void) {
	if (state == STATE_REC_ACTIVE) {
		audio_buffer[audio_buffer_put] = LL_ADC_REG_ReadConversionData8(ADC1);
		audio_buffer_put = (audio_buffer_put + 1) & 511;
		if ((audio_buffer_put == 0) || (audio_buffer_put == 256)) {
			flag_full = 1;	// Signal buffer full
		}
	}

	LL_ADC_ClearFlag_EOC(ADC1);
}

void TIM2_IRQHandler(void) {
	// This triggers at 8kHz (125us), this must be FAST

	// Update DAC
	if (audio_op == AUDIO_OP_PLAY) {
		LL_DAC_ConvertData8RightAligned(DAC, LL_DAC_CHANNEL_1, audio_buffer[audio_buffer_get]);

		audio_pitch_acc += audio_pitch_delta;

		while (audio_pitch_acc >= 0x4000) {
			audio_buffer_get = (audio_buffer_get + 1) & 511;
			if ((audio_buffer_get == 0) || (audio_buffer_get == 256)) {
				flag_empty = 1;	// Signal buffer empty
			}
			audio_pitch_acc -= 0x4000;
		}

	} else if (audio_op == AUDIO_OP_BEEP) {
		LL_DAC_ConvertData8RightAligned(DAC, LL_DAC_CHANNEL_1, (tone_a_acc & 0x8000) ? 127 : 0);

		if (!audio_melody_timer) {
			audio_melody_timer = (uint16_t)audio_melody_ptr[audio_melody_index].duration << 6;	// 64 / 8000 = 8ms steps
			if (!audio_melody_timer) {
				// End of melody
				audio_op = AUDIO_OP_NOP;
				Disable_SpkAmp();
			} else {
				tone_a_delta = tone_lut[audio_melody_ptr[audio_melody_index].note];
				audio_melody_index++;
			}
		} else
			audio_melody_timer--;

		tone_a_acc += tone_a_delta;
	} else if (audio_op == AUDIO_OP_SYNTH) {
		LL_DAC_ConvertData8RightAligned(DAC, LL_DAC_CHANNEL_1, (*synth_gen[synth_type & 3])());

		tone_a_acc += tone_a_delta;
		tone_b_acc += tone_b_delta;
	} else {
		// TODO: Do this only once
		LL_DAC_ConvertData8RightAligned(DAC, LL_DAC_CHANNEL_1, 127);
	}

	LL_TIM_ClearFlag_UPDATE(TIM2);
}

void TIM3_IRQHandler(void) {
	// TIM3 overflow: IR timeout, reset IR RX
	irrx_bitidx = 0;
	irrx_byteidx = 0;
	irrx_state = IRRX_STATE_IDLE;

	LL_TIM_ClearFlag_UPDATE(TIM3);
}

void TIM15_IRQHandler(void) {
	TSC_IOConfigTypeDef IOConfig;

	// This triggers at 600Hz (1667us)
	flag_tick = 1;

	// Do LED scanning via SPI2
	// Updating the SR takes 16 / 3M = 5.34us
	LL_GPIO_ResetOutputPin(RCLK_GPIO_Port, RCLK_Pin);
	LL_SPI_TransmitData16(SPI2, dig_lut[scan_counter] | disp_buf[scan_counter]);
	LL_SPI_EnableIT_TXE(SPI2);	// Interrupt to latch new data once SPI transfer is done

	// Request a touch channel measurement
	// Documentation for the TSC HAL is shit and seems way more complicated than it should be
	// See Table 5 on page 20 in datasheet for TSC IOs and groups
	// All groups are acquired at the same time
	// Change the enabled analog switches in each group, 3 steps are nough to scan everything
	IOConfig.ShieldIOs = 0;

	if (touch_io == 0) {
		IOConfig.ChannelIOs = TSC_GROUP1_IO2 | TSC_GROUP5_IO2;
	} else if (touch_io == 1) {
		IOConfig.ChannelIOs = TSC_GROUP1_IO3 | TSC_GROUP5_IO3;
	} else if (touch_io == 2) {
		IOConfig.ChannelIOs = TSC_GROUP1_IO4 | TSC_GROUP5_IO4;
	}
	IOConfig.SamplingIOs = TSC_GROUP1_IO1 | TSC_GROUP5_IO1;	// Never changes

	HAL_TSC_IOConfig(&h_tsc, &IOConfig);

	DBGPIN_TOGGLE

	// Testing
	HAL_TSC_IODischarge(&h_tsc, ENABLE);
	ShortWait(10);

	HAL_TSC_Start_IT(&h_tsc);

	LL_TIM_ClearFlag_UPDATE(TIM15);
}

void TIM16_IRQHandler(void) {
	irtx_irq_handler();
	LL_TIM_ClearFlag_UPDATE(TIM16);
}

void TSC_IRQHandler(void) {
	HAL_TSC_IRQHandler(&h_tsc);
}

void HAL_TSC_ConvCpltCallback(TSC_HandleTypeDef* htsc) {
	// Touch measurement of selected IO in all groups done
	HAL_TSC_IODischarge(htsc, ENABLE);

	// Ignore max count errors, might happen because of unused TSC_GROUP6_IO2

	touch_val[(touch_io << 2) + 0] = HAL_TSC_GroupGetValue(htsc, TSC_GROUP1_IDX);
	touch_val[(touch_io << 2) + 1] = HAL_TSC_GroupGetValue(htsc, TSC_GROUP5_IDX);

	// Cycle through channels in each group
	if (touch_io == 2)
		touch_io = 0;
	else
		touch_io++;
}
