// FBL 12 sponsor badge V1
// For STM32F072CBT6
// furrtek 2024

#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f0xx_hal.h"
#include "stm32f0xx_ll_adc.h"
#include "stm32f0xx_ll_dac.h"
#include "stm32f0xx_ll_i2c.h"
#include "stm32f0xx_ll_crs.h"
#include "stm32f0xx_ll_rcc.h"
#include "stm32f0xx_ll_bus.h"
#include "stm32f0xx_ll_system.h"
#include "stm32f0xx_ll_exti.h"
#include "stm32f0xx_ll_cortex.h"
#include "stm32f0xx_ll_utils.h"
#include "stm32f0xx_ll_pwr.h"
#include "stm32f0xx_ll_dma.h"
#include "stm32f0xx_ll_spi.h"
#include "stm32f0xx_ll_tim.h"
#include "stm32f0xx_ll_gpio.h"
#include "stm32f0xx_ll_usart.h"
#include "gpio.h"

#define ADDR_BL	(uint32_t)0x08000000	// Bootloader 32kB
#define ADDR_APP (uint32_t)0x08008000	// App 128kB - 32kB - 2kB
#define ADDR_UD	(uint32_t)0x0801F800	// User data 2kB
#define ADDR_APPSUM (ADDR_UD - 4)		// Last dword of app

// Audio
#define AMPSD_POLARITY_POS 1	// Set to 0 if PAM8302 is used

#define LED_MASK_STOP 32
#define LED_MASK_REC 64
#define LED_MASK_BATT 128
#define LED_MASK_PLAY 1

#define UI_RATE 20						// 20Hz time base
#define TICK_PERIOD (600 / UI_RATE)		// From 600Hz ticks
#define BEACON_PERIOD (UI_RATE * 3)		// 3s
//#define IDLE_TIMEOUT (UI_RATE * 2)		// 2s
#define COMM_TIMEOUT (UI_RATE * 1)		// 1s
//#define SCROLL_PERIOD (UI_RATE / 2)		// 0.5s
#define NAME_PAGE_PERIOD (UI_RATE * 2)	// 2s
#define BATT_PERIOD (UI_RATE * 10)		// 10s
#define NAME_ANIM_PERIOD (UI_RATE * 4)	// 4s
#define BLINK_PERIOD (UI_RATE / 2)		// 0.5s

#define TIMEOUT (UI_RATE * 7)	// 7s

typedef enum {
	STATE_IDLE = 0,
	STATE_TESTMODE,
	STATE_TEMP,
	//STATE_FAV,
	//STATE_MATCH,
	STATE_REC_SEL,
	STATE_REC_PREDEF,
	STATE_REC_ACTIVE,
	STATE_REC_PITCH,
	STATE_SYNTH,
	STATE_ALERT
} state_t;

extern TSC_HandleTypeDef h_tsc;
extern PCD_HandleTypeDef hpcd_USB_FS;
extern volatile uint8_t flag_tick;
extern volatile uint8_t disp_buf[9];
extern uint8_t blink_mask;
extern state_t state;
extern uint8_t uid[12];
extern uint16_t touched_keys;
extern uint8_t synth_type;
extern uint32_t xs_state;

void common_enter();

void uart_print(char * buffer);
void enter_alert();
void enter_temp(const uint8_t time);

void ShortWait(uint32_t wait);
void Error_Handler(void);

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
