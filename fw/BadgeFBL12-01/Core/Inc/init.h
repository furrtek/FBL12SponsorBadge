// FBL 12 sponsor badge V1
// For STM32F072CBT6
// furrtek 2024

#ifndef INC_INIT_H_
#define INC_INIT_H_

void main_init();
void SystemClock_Config(void);
void MX_GPIO_Init(void);
void MX_USART2_UART_Init(void);
void MX_ADC_Init(void);
void MX_DAC_Init(void);
void MX_USB_PCD_Init(void);
void MX_TSC_Init(void);
void MX_I2C1_Init(void);
void MX_IRTIM_Init(void);
void MX_SPI1_Init(void);
void MX_SPI2_Init(void);
void MX_TIM2_Init(void);
void MX_TIM3_Init(void);
void MX_TIM14_Init(void);
void MX_TIM15_Init(void);
void MX_TIM16_Init(void);
void MX_TIM17_Init(void);

#endif /* INC_INIT_H_ */
