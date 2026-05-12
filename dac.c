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

// Ground LDAC
// Tie SHDN to Vdd
void DAC_init(void) {
	// enable clock for GPIOA & SPI1
	RCC->AHB2ENR |= (RCC_AHB2ENR_GPIOAEN);                // GPIOA: DAC NSS/SCK/SDO
	RCC->APB2ENR |= (RCC_APB2ENR_SPI1EN);                 // SPI1 port
	/* USER ADD GPIO configuration of MODER/PUPDR/OTYPER/OSPEEDR registers HERE */
	// configure AFR for SPI1 function (1 of 3 SPI bits shown here)
	// SPI1_MOSI Port A pin 7

	// PA4=NSS, PA5=SCK, PA6=MISO, PA7=MOSI: alternate function mode
	GPIOA->MODER &= ~(GPIO_MODER_MODE4 |
	                  GPIO_MODER_MODE5 |
	                  GPIO_MODER_MODE6 |
	                  GPIO_MODER_MODE7);

	GPIOA->MODER |=  (GPIO_MODER_MODE4_1 |
	                  GPIO_MODER_MODE5_1 |
	                  GPIO_MODER_MODE6_1 |
	                  GPIO_MODER_MODE7_1);

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


	//configure COL_PORT OTYPER output type as push-pull (1'b0) for pins 1,2,3
  	GPIOA -> OTYPER  &= ~(GPIO_OTYPER_OT4
   						  	     | GPIO_OTYPER_OT5
							        | GPIO_OTYPER_OT6
									  | GPIO_OTYPER_OT7);

	//configure COL_PORT OSPEEDR as very high speed (2'b11) for pins 1,2,3
  	GPIOA -> OSPEEDR |= (GPIO_OSPEEDR_OSPEED4
   						        | GPIO_OSPEEDR_OSPEED5
							        | GPIO_OSPEEDR_OSPEED6
									  | GPIO_OSPEEDR_OSPEED7);

	/*configure COL_PORT PUPDR as
	 no pull-up or pull-down (2'b00) for pins 1,2,3 */
  	GPIOA -> PUPDR &= ~(GPIO_PUPDR_PUPD4
   							  	  | GPIO_PUPDR_PUPD5
							        | GPIO_PUPDR_PUPD6
									  | GPIO_PUPDR_PUPD7);

   // preset PE0, PE1, PE2 to 0
  	GPIOA -> BRR = (GPIO_PIN_4
   						  		| GPIO_PIN_5
						     		| GPIO_PIN_6
									| GPIO_PIN_7);

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

// Converts a desired output voltage in centivolts to a calibrated 12-bit DAC
// data code, capped at the maximum allowed output voltage.
uint16_t DAC_volt_conv(uint16_t target_centivolts)
{
    uint32_t millivolts;
    uint32_t microvolts;
    uint32_t dn;
    int32_t corrected_microvolts;

    if (target_centivolts > 330U) {
        target_centivolts = 330U;
    }

    millivolts = (uint32_t)target_centivolts * 10U;
    microvolts = millivolts * 1000U;

    corrected_microvolts = microvolts - DAC_OFFSET_UV;

    if (corrected_microvolts < 0) {
   	 return 0;
    }

    dn = corrected_microvolts / DAC_SLOPE_UV_PER_CODE;

    if (dn > 4095U) {
        dn = 4095U;
    }

    return (uint16_t)dn;
}

void DAC_write(uint16_t data) {
	// Wait until transmit buffer/FIFO can accept data
	 while (!(SPI1->SR & SPI_SR_TXE)) {
		  ;
	 }

	 // Configure upper 4 bits for DAC
	 data &= ~(0xF << 12);
	 // Configured for 2x gain
	 data |= (0x1 << 12);

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




