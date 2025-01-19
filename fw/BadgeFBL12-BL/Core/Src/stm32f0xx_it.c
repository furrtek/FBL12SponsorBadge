// FBL 12 sponsor badge bootloader
// For STM32F072CBT6
// furrtek 2024

#include "main.h"
#include "stm32f0xx_it.h"
#include "luts.h"
#include "audio.h"
#include "touch.h"

extern PCD_HandleTypeDef hpcd_USB_FS;

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

void USB_IRQHandler(void) {
	HAL_PCD_IRQHandler(&hpcd_USB_FS);
}

void SPI2_IRQHandler(void) {
	// The TXE interrupt triggers when the TX FIFO is empty, not when the transfer is done
	// Just polling the busy flag might be enough without using the TXE interrupt
	if (LL_SPI_IsActiveFlag_TXE(SPI2)) {
		while(LL_SPI_IsActiveFlag_BSY(SPI2)) { };	// Poll busy flag to wait for the end of the transfer
		LL_GPIO_SetOutputPin(RCLK_GPIO_Port, RCLK_Pin);	// Rising edge on RCLK, update SR outputs

		if (scan_counter == 8) {
			LL_GPIO_ResetOutputPin(DIG8_GPIO_Port, DIG8_Pin);	// Enable individual LEDs
			scan_counter = 0;
		} else {
			LL_GPIO_SetOutputPin(DIG8_GPIO_Port, DIG8_Pin);		// Disable individual LEDs
			scan_counter++;
		}

		LL_SPI_DisableIT_TXE(SPI2);
	}
}

void TIM2_IRQHandler(void) {
	// This triggers at 8kHz (125us), this must be FAST

	// Update DAC
	if (!beep_timer) {
		LL_DAC_ConvertData8RightAligned(DAC, LL_DAC_CHANNEL_1, 127);
		Disable_Amp();
	} else {
		LL_DAC_ConvertData8RightAligned(DAC, LL_DAC_CHANNEL_1, (beep_acc & 0x8000) ? 127 : 0);
		beep_acc += beep_delta;
		beep_timer--;
	}

	LL_TIM_ClearFlag_UPDATE(TIM2);
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

	HAL_TSC_IOConfig(&htsc, &IOConfig);
	HAL_TSC_Start_IT(&htsc);

	LL_TIM_ClearFlag_UPDATE(TIM15);
}

void TSC_IRQHandler(void) {
	// Touch measurement of selected IO in all groups done
	HAL_TSC_IODischarge(&htsc, ENABLE);
	HAL_TSC_IRQHandler(&htsc);	// This clears EOAF

	touch_val[(touch_io << 2) + 0] = HAL_TSC_GroupGetValue(&htsc, TSC_GROUP1_IDX);
	touch_val[(touch_io << 2) + 1] = HAL_TSC_GroupGetValue(&htsc, TSC_GROUP5_IDX);

	// Cycle through channels in each group
	if (touch_io == 2)
		touch_io = 0;
	else
		touch_io++;
}
