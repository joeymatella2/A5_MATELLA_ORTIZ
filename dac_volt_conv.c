/*
 *******************************************************************************
 * @file           : dac.c
 * @brief          : X
 * project         : EE 329 S'26 AX
 * authors         : joeym
 * version         : 0.1
 * date            : May 11, 2026
 * compiler        : STM32CubeIDE v.1.19.0 Build: 14980_20230301_1550 (UTC)
 * target          : NUCLEO-L4A6ZG
 * clocks          : 4 MHz MSI to AHB2
 * @attention      : (c) 2026 STMicroelectronics.  All rights reserved.
 *******************************************************************************
 * Description: X
 *
 *******************************************************************************
 * GPIO Wiring
 * |   Component    | GPIO Identifier | Connector Location | Config
 *-----------------------------------------------------------------------------
 * | LCD - DB4 - 11 | PC0             | CN9-3              | OUT
 *******************************************************************************
 * Version History
 *  Ver.|   Date   |  Description
 *  ---------------------------------------------------------------------------
 *      |          | 
 *******************************************************************************
 *
 * Header format adapted from [Code Appendix by Kevin Vo] pg 5
 */
#include "dac.h"

void DAC_init(void) {
	// enable clock for GPIOA & SPI1
	RCC->AHB2ENR |= (RCC_AHB2ENR_GPIOAEN);                // GPIOA: DAC NSS/SCK/SDO
	RCC->APB2ENR |= (RCC_APB2ENR_SPI1EN);                 // SPI1 port
	/* USER ADD GPIO configuration of MODER/PUPDR/OTYPER/OSPEEDR registers HERE */
	// configure AFR for SPI1 function (1 of 3 SPI bits shown here)
	// SPI1_MOSI Port A pin 7
	GPIOA->AFR[0] &= ~((0x000F << GPIO_AFRL_AFSEL7_Pos)); // clear nibble for bit 7 AF
	GPIOA->AFR[0] |=  ((0x0005 << GPIO_AFRL_AFSEL7_Pos)); // set b7 AF to SPI1 (fcn 5)
	// SPI1_MISO Port A pin 6
	GPIOA->AFR[0] &= ~((0x000F << GPIO_AFRL_AFSEL6_Pos)); // clear nibble for bit 6 AF
	GPIOA->AFR[0] |=  ((0x0005 << GPIO_AFRL_AFSEL6_Pos)); // set b6 AF to SPI1 (fcn 5)
	// SPI1_SCK Port A pin 5
	GPIOA->AFR[0] &= ~((0x000F << GPIO_AFRL_AFSEL5_Pos));
	GPIOA->AFR[0] |=  ((0x0005 << GPIO_AFRL_AFSEL5_Pos));
	// SPI1_NSS Port A pin 4
	GPIOA->AFR[0] &= ~((0x000F << GPIO_AFRL_AFSEL4_Pos));
	GPIOA->AFR[0] |=  ((0x0005 << GPIO_AFRL_AFSEL4_Pos));
	// SPI config as specified @ STM32L4 RM0351 rev.9 p.1459
	// called by or with DAC_init()
	// build control registers CR1 & CR2 for SPI control of peripheral DAC
	// assumes no active SPI xmits & no recv data in process (BSY=0)
	// CR1 (reset value = 0x0000)
	SPI1->CR1 &= ~( SPI_CR1_SPE );             	// disable SPI for config
	SPI1->CR1 &= ~( SPI_CR1_RXONLY );          	// recv-only OFF
	SPI1->CR1 &= ~( SPI_CR1_LSBFIRST );        	// data bit order MSb:LSb
	SPI1->CR1 &= ~( SPI_CR1_CPOL | SPI_CR1_CPHA ); // SCLK polarity:phase = 0:0
	SPI1->CR1 |=	 SPI_CR1_MSTR;              	// MCU is SPI controller
	// CR2 (reset value = 0x0700 : 8b data)
	SPI1->CR2 &= ~( SPI_CR2_TXEIE | SPI_CR2_RXNEIE ); // disable FIFO intrpts
	SPI1->CR2 &= ~( SPI_CR2_FRF);              	// Moto frame format
	SPI1->CR2 |=	 SPI_CR2_NSSP;              	// auto-generate NSS pulse
	SPI1->CR2 |=	 SPI_CR2_DS;                	// 16-bit data
	SPI1->CR2 |=	 SPI_CR2_SSOE;              	// enable SS output
	// CR1
	SPI1->CR1 |=	 SPI_CR1_SPE;               	// re-enable SPI for ops
}

uint16_t DAC_volt_conv(float voltage) {
	// Clamp input to valid range (0 – 3.3 V)
	if (voltage < 0.0f)   voltage = 0.0f;
	if (voltage > 3.3f)   voltage = 3.3f;

	// Scale voltage to 12-bit code: code = round(voltage / 3.3 * 4095)
	uint16_t code = (uint16_t)((voltage / 3.3f) * 4095.0f + 0.5f);
	code &= 0x0FFF;       // mask to 12 bits (defensive)

	return code;          // control bits applied in DAC_write()
}

void DAC_write(uint16_t data) {
	// Wait until transmit buffer/FIFO can accept data
	while (!(SPI1->SR & SPI_SR_TXE)) {
		;
	}
	// Configure upper 4 bits for DAC
	data &= ~(0xF << 12);
	data |= (0x3 << 12);
	// Write 16-bit data to SPI data register
	SPI1->DR = data;
	// Wait until transmit buffer is empty
	while (!(SPI1->SR & SPI_SR_TXE)) {
		;
	}
	// Wait until SPI is no longer busy shifting bits out
	while (SPI1->SR & SPI_SR_BSY) {
		;
	}
}
