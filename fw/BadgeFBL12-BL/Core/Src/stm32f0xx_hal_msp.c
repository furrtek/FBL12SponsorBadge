// FBL 12 sponsor badge bootloader
// For STM32F072CBT6
// furrtek 2024

#include "main.h"

void HAL_MspInit(void) {
	__HAL_RCC_SYSCFG_CLK_ENABLE();
	__HAL_RCC_PWR_CLK_ENABLE();
}

void HAL_TSC_MspInit(TSC_HandleTypeDef* htsc) {
	GPIO_InitTypeDef GPIO_InitStruct = {0};
	if (htsc->Instance == TSC) {
		/* Peripheral clock enable */
		__HAL_RCC_TSC_CLK_ENABLE();

		__HAL_RCC_GPIOA_CLK_ENABLE();
		__HAL_RCC_GPIOB_CLK_ENABLE();
		/**TSC GPIO Configuration
		PA0     ------> TSC_G1_IO1
		PA1     ------> TSC_G1_IO2
		PA2     ------> TSC_G1_IO3
		PA3     ------> TSC_G1_IO4
		PB11     ------> TSC_G6_IO1
		PB12     ------> TSC_G6_IO2
		PB13     ------> TSC_G6_IO3
		PB14     ------> TSC_G6_IO4
		PA9     ------> TSC_G4_IO1
		PA10     ------> TSC_G4_IO2
		PB3     ------> TSC_G5_IO1
		PB4     ------> TSC_G5_IO2
		PB6     ------> TSC_G5_IO3
		PB7     ------> TSC_G5_IO4
		*/
		GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_9;
		GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
		GPIO_InitStruct.Pull = GPIO_NOPULL;
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
		GPIO_InitStruct.Alternate = GPIO_AF3_TSC;
		HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

		GPIO_InitStruct.Pin = GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3|GPIO_PIN_10;
		GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
		GPIO_InitStruct.Pull = GPIO_NOPULL;
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
		GPIO_InitStruct.Alternate = GPIO_AF3_TSC;
		HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

		GPIO_InitStruct.Pin = GPIO_PIN_11|GPIO_PIN_3;
		GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
		GPIO_InitStruct.Pull = GPIO_NOPULL;
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
		GPIO_InitStruct.Alternate = GPIO_AF3_TSC;
		HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

		GPIO_InitStruct.Pin = GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_4
							  |GPIO_PIN_6|GPIO_PIN_7;
		GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
		GPIO_InitStruct.Pull = GPIO_NOPULL;
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
		GPIO_InitStruct.Alternate = GPIO_AF3_TSC;
		HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
	}
}

void HAL_TSC_MspDeInit(TSC_HandleTypeDef* htsc) {
	if (htsc->Instance==TSC) {
		__HAL_RCC_TSC_CLK_DISABLE();

		/**TSC GPIO Configuration
		PA0     ------> TSC_G1_IO1
		PA1     ------> TSC_G1_IO2
		PA2     ------> TSC_G1_IO3
		PA3     ------> TSC_G1_IO4
		PB11     ------> TSC_G6_IO1
		PB12     ------> TSC_G6_IO2
		PB13     ------> TSC_G6_IO3
		PB14     ------> TSC_G6_IO4
		PA9     ------> TSC_G4_IO1
		PA10     ------> TSC_G4_IO2
		PB3     ------> TSC_G5_IO1
		PB4     ------> TSC_G5_IO2
		PB6     ------> TSC_G5_IO3
		PB7     ------> TSC_G5_IO4
		*/
		HAL_GPIO_DeInit(GPIOA, GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3
							  |GPIO_PIN_9|GPIO_PIN_10);

		HAL_GPIO_DeInit(GPIOB, GPIO_PIN_11|GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14
							  |GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_6|GPIO_PIN_7);
	}
}
