// FBL 12 sponsor badge bootloader
// For STM32F072CBT6
// furrtek 2024

#include <string.h>
#include "main.h"
#include "init.h"
#include "touch.h"
#include "audio.h"
#include "display.h"
#include "usb_device.h"

// MSC bootloader started by touching right ear and nose on power on

// MCU flash memory map: 128kB, 2kB pages, 64 pages, 32 sectors
// Pages 0~15: Bootloader
// Pages 16~62: App
// Page 63: User data (user_data_ptr, 0x0801F800)

// ADC for audio input and battery voltage monitoring
// DAC for audio output
// I2C1 for communication with XW12A (can't use because of wonky format !)
// SPI1 for communication with external flash
// SPI2 for LED scanning
// TIM2 for ADC/DAC timing
// TIM14 for LED PWM
// TIM15 for display scan and global tick

uint32_t uid[3];	// MCU UID
uint8_t app_valid = 0;
uint32_t app_checksum = 0;

TSC_HandleTypeDef htsc;

volatile uint8_t flag_tick;
volatile uint8_t disp_buf[9];

const char str_bootloader[] = "U S B";

char str_buffer[256];

void set_brightness(uint8_t v) {
	LL_TIM_OC_SetCompareCH1(TIM14, ((uint16_t)v * 1000) >> 8);
}

void ShortWait(uint32_t wait) {
	volatile uint32_t wait_loop_index = wait;	// Units of ~10us at 12MHz
	while (wait_loop_index) {
		wait_loop_index--;
	}
}

void run_app() {
	typedef void (*function_ptr_t)(void);
	uint32_t appStack;
	function_ptr_t appEntry;

	appStack = (uint32_t) *((__IO uint32_t *) ADDR_APP);
	appEntry = (function_ptr_t) *(__IO uint32_t*) (ADDR_APP + 4);

	// Copy the app vector table to RAM
	volatile uint32_t *VectorTable = (volatile uint32_t *)0x20000000;
	for (int i = 0; i < 48; i++)
		VectorTable[i] = *(__IO uint32_t*)(ADDR_APP + (i<<2));

	__HAL_RCC_AHB_FORCE_RESET();
	__HAL_RCC_SYSCFG_CLK_ENABLE();
	__HAL_RCC_AHB_RELEASE_RESET();
	__DSB();
	__HAL_SYSCFG_REMAPMEMORY_SRAM();	// Set 0x00000000 to start of RAM where app vector table now is
	__DSB();
	__ISB();

	__set_MSP(appStack);

	appEntry();
}

void uart_print(char * buffer) {
	char c;

	while ((c = *buffer++)) {
	    while (!LL_USART_IsActiveFlag_TXE(USART2)) {};
	    LL_USART_TransmitData8(USART2, c);
	}
}

void start_msc() {
	uart_print("MSC start\n");
	set_brightness(200);
	PrintTxt(0, str_bootloader);
	beep(8000, 2000);
	LL_mDelay(100);
	MX_USB_DEVICE_Init();

	while (1) {	};
}

void check_app() {
	app_valid = 0;

	uint32_t app_size = (*(uint32_t*)ADDR_APPSIZE);
	if ((app_size == 0) || (app_size > SIZE_APP_MAX)) {
		uart_print("App size invalid\n");
		return;
	}

	app_checksum = 0;
	uint32_t * addr = (uint32_t*)ADDR_APP;
	uint32_t * end = (uint32_t*)(ADDR_APP + app_size);
	while (addr < end)
		app_checksum += (*addr++);

	if (app_checksum != (*(uint32_t*)ADDR_APPSUM))
		uart_print("App checksum invalid\n");
	else
		app_valid = 1;
}

int main(void) {
	uint32_t tick_counter = 0;

	ClearDisp();
	disp_buf[8] = 0;

	main_init();

	uart_print("BL V1\n");

	check_app();

	if (!app_valid)
		start_msc();

	while ((touch_val[TOUCH_NOSE] < touch_thr[TOUCH_NOSE]) &&
			(touch_val[TOUCH_REAR] < touch_thr[TOUCH_REAR])) {
		if (flag_tick) {
			// 600Hz
			flag_tick = 0;
			if (tick_counter >= 200) {
				uart_print("Manual entry\n");
				start_msc();
			} else
				tick_counter++;
		}
	}

	uart_print("Starting app\n");
    while (!LL_USART_IsActiveFlag_TC(USART2)) {};

	run_app();
}

void Error_Handler(void) {
	uart_print("Error :(\n");
	__disable_irq();
	while (1) {
	}
}
