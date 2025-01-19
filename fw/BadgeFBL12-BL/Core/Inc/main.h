// FBL 12 sponsor badge bootloader
// For STM32F072CBT6
// furrtek 2024

#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f0xx_hal.h"
#include "stm32f0xx_ll_dac.h"
#include "stm32f0xx_ll_crs.h"
#include "stm32f0xx_ll_rcc.h"
#include "stm32f0xx_ll_bus.h"
#include "stm32f0xx_ll_system.h"
#include "stm32f0xx_ll_exti.h"
#include "stm32f0xx_ll_cortex.h"
#include "stm32f0xx_ll_utils.h"
#include "stm32f0xx_ll_pwr.h"
#include "stm32f0xx_ll_spi.h"
#include "stm32f0xx_ll_tim.h"
#include "stm32f0xx_ll_gpio.h"
#include "stm32f0xx_ll_usart.h"
#include "gpio.h"

#define AMPSD_POLARITY_POS 1	// Set to 0 if PAM8302 is used

#define ADDR_BL	(uint32_t)0x08000000	// Bootloader 32kB
#define ADDR_APP (uint32_t)0x08008000	// App 128kB - 32kB - 2kB
#define ADDR_UD	(uint32_t)0x0801F800	// User data 2kB
#define ADDR_APPSIZE (ADDR_UD - 8)		// Next to last dword of app
#define ADDR_APPSUM (ADDR_UD - 4)		// Last dword of app
//#define ADDR_SYSMEM (uint32_t)0x1FFFC800	// RM0091 page 52

#define SIZE_APP_MAX (uint32_t)(ADDR_UD - ADDR_APP)
#define SIZE_MCUFLASH_MAX (uint32_t)(FLASH_BANK1_END + 1 - ADDR_APP)
#define SIZE_EXTFLASH_MAX (uint32_t)(0x00080000)	// 512kB external flash

extern TSC_HandleTypeDef htsc;
extern PCD_HandleTypeDef hpcd_USB_FS;
extern volatile uint8_t flag_tick;
extern volatile uint8_t disp_buf[9];
extern uint32_t uid[3];
extern uint8_t app_valid;
extern uint32_t app_checksum;
extern char str_buffer[256];

void set_brightness(uint8_t v);
void uart_print(char * buffer);
void check_app();
void ShortWait(uint32_t wait);
void Error_Handler(void);

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
