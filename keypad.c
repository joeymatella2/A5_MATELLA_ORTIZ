/*
 *******************************************************************************
 * @file           : keypad.c
 * @brief          : keypad configuration functions and keypress detection functions
 * project         : EE 329 S'26 A3
 * authors         : Alex Tauber and Joseph Matella
 * version         : 0.2
 * date            : 230413
 * compiler        : STM32CubeIDE v.1.19.0 Build: 14980_20230301_1550 (UTC)
 * target          : NUCLEO-L4A6ZG
 * clocks          : 4 MHz MSI to AHB2
 * @attention      : (c) 2026 STMicroelectronics.  All rights reserved.
 *******************************************************************************
 * KEYPAD PLAN :
 * set columns as outputs, rows as inputs w pulldowns and 220 ohm protection
 * resistors create function to check if any key is pressed (returns 1 if any
 * key is pressed , 0 if not) incorporate debounce and settle time delays in
 * functiion to ensure stability. Create function to determine which key is
 * pressed after confirming any button was pressed (with debounce) iterate
 * through columns, setting each high and checking if any row inputs are high,
 * if so, decode row and column and output key value (1-9,*(A), #(10)) as int (0-15)
 *
 *******************************************************************************
 * KEYPAD WIRING 4 ROWS 3 COLS (pinout NUCLEO-L4A6ZG = L496ZG)
 *      peripheral – Nucleo I/O
 * keypad 1  COL 2 - PB1 = CN10 - OUT
 * keypad 2  ROW 1 - PA0 = CN10 - IN, PD ON
 * keypad 3  COL 1 - PB0 = CN10 - OUT
 * keypad 4  ROW 4 - PA3 = CN9  - IN, PD ON
 * keypad 5  COL 3 - PB2 = CN9  - OUT
 * keypad 6  ROW 3 - PA2 = CN10 - IN, PD ON
 * keypad 7  ROW 2 - PA1 = CN10 - IN, PD ON
 *******************************************************************************
 * Version History
 *  Ver.|   Date   |  Description
 *  ---------------------------------------------------------------------------
 *  0.1 | 20260413 | Basic functionality and debounce added through functions
 * 	               Keypad_Config, Keypad_IsAnyKeyPressed
 *  0.2 | 20260421 | Keypad_Transition_High and Keypad_SetPressed added.
 *******************************************************************************
 *  Header format adapted from [Code Appendix by Kevin Vo] pg 5
 */

#include "keypad.h"

/*
 *-----------------------------------------------------------------------------
 * function : Keypad_Config( )
 * INs      : none
 * OUTs     : none
 * action   : initializes GPIO pins for keypad interface: sets column pins as
 * high-speed push-pulloutputs and row pins as inputs with pull-down resistors
 * authors  : Alex Tauber & Joseph Matella
 *
 * usage    : called in main.c to initialize GPIO pins for keypad interface
 * version  : 0.1
 * date     : 4132026
 *
 *------------------------------------------------------------------------------
 */

void Keypad_Config(void){

		//initialize clock for peripheral A and B (for GPIOA and GPIOB)
		RCC -> AHB2ENR |= (RCC_AHB2ENR_GPIOAEN);
		RCC -> AHB2ENR |= (RCC_AHB2ENR_GPIOBEN);

	 	//initialize ROW_PORT as input mode (clear pins 0,1,2,3)
	  	ROW_PORT -> MODER &= ~(GPIO_MODER_MODE0
	  									| GPIO_MODER_MODE1
										| GPIO_MODER_MODE2
										| GPIO_MODER_MODE3);

		//clear ROW_PORT PUPDR for pins 0,1,2,3
	  	ROW_PORT -> PUPDR &= ~(GPIO_PUPDR_PUPD0
	              	         	| GPIO_PUPDR_PUPD1
										| GPIO_PUPDR_PUPD2
										| GPIO_PUPDR_PUPD3);

	  	//configure ROW-PORT PUPDR as pull-down (2'b10) for pins 0,1,2,3
	  	ROW_PORT -> PUPDR |= (GPIO_PUPDR_PUPD0_1
	  	              	       			| GPIO_PUPDR_PUPD1_1
									 	| GPIO_PUPDR_PUPD2_1
									 	| GPIO_PUPDR_PUPD3_1);

	  	//clear COL_PORT MODER register pins 1,2,3
	  	COL_PORT -> MODER &= ~(GPIO_MODER_MODE0
										| GPIO_MODER_MODE1
										| GPIO_MODER_MODE2);

	  	/* configure COL_PORT MODER register as
		general purpose output (2'b01) for pins 1,2,3 */
	  	COL_PORT -> MODER |= (GPIO_MODER_MODE0_0
	  						   	  		| GPIO_MODER_MODE1_0
									 	| GPIO_MODER_MODE2_0);

		//configure COL_PORT OTYPER output type as push-pull (1'b0) for pins 1,2,3
	  	COL_PORT -> OTYPER  &= ~(GPIO_OTYPER_OT0
	   						  	     	| GPIO_OTYPER_OT1
								        | GPIO_OTYPER_OT2);

		//configure COL_PORT OSPEEDR as very high speed (2'b11) for pins 1,2,3
	  	COL_PORT -> OSPEEDR |= (GPIO_OSPEEDR_OSPEED0
	   						       	    | GPIO_OSPEEDR_OSPEED1
								        | GPIO_OSPEEDR_OSPEED2);

		/*configure COL_PORT PUPDR as
		 no pull-up or pull-down (2'b00) for pins 1,2,3 */
	  	COL_PORT -> PUPDR &= ~(GPIO_PUPDR_PUPD0
	   							  	    | GPIO_PUPDR_PUPD1
								        | GPIO_PUPDR_PUPD2);

	   // preset PE0, PE1, PE2 to 0
	  	COL_PORT -> BRR = (GPIO_PIN_0
	   						  		    | GPIO_PIN_1
							     		| GPIO_PIN_2);
}

/*
 *---------------------------------------------------------------------------
 * function : Keypad_IsAnyKeyPressed( )
 * INs      : none
 * OUTs     : returns true if a key is pressed, false if not
 * action   : creates history variable storing previous states of keypad;
 * drive all COLUMNS HI; see if any ROWS are HI; if so, shift history
 * variable and input 1 in LSB; if not, shift history variable and input 0 in LSB;
 * check if history variable is 0xFF (8 consecutive reads of 1) or 0x00
 * (8 consecutive reads of 0); return accordingly; software for loop delays used
 * to allow for debounce and voltage settling
 *
 * usage    : called in main.c to check if any key is pressed before calling
 * 			Keypad_WhichKeyIsPressed() to determine which key is pressed
 *
 * authors  : Alex Tauber
 * version  : 0.1
 * date     : 4132026
 *--------------------------------------------------------------------------
 */

bool Keypad_IsAnyKeyPressed(void) {

   /* create nonzero status variable to store current and past IDR states
   (nonzero state gets processed as a 1) */
   uint8_t pastHistory = 0x01;

   // preset all columns HI
   COL_PORT -> BSRR = COL_PINS;

   /* allow column voltage to settle via software for loop delay
   (calibrated for 3.06 microseconds per iteration) */
   for ( uint16_t idx = 0; idx <  SETTLE_DELAY / 0.00000306; idx++ ) {}

   // loop until value is stable (until either 1 or 0 is returned)
   while(1) {

	// software for loop delay (calibrated for 3.06 microseconds per iteration)
   	for ( uint16_t idx = 0; idx < DEBOUNCE_DELAY / 0.00000306; idx++ ) {}

   		/* Check current status of IDR registerif press detected,
		input 1 in LSB and shift over */
   		pastHistory = ( pastHistory << 1 | ((ROW_PORT->IDR & ROW_PINS) != 0 ? 1 : 0) );

		// if 8 consecutive reads are high (0xFF), return 1
   		if ( pastHistory == 0xFF ) return 1;

		// if 8 consecutive reads are low (0x00), return 0
   		if ( pastHistory == 0x00 ) return 0;

		// otherwise, continue looping until stable value is reached
   }

}

/*
*-----------------------------------------------------------------------------
* function : Keypad_TransitionHigh
* INs      : void
* OUTs     : bool indicating if LOW to HIGH transition detected
*            1 : LOW to High transition detected
*            0 : LOW to High transition not detected
* action   : Detects if a button is pressed (LOW to HIGH). Prevents multiple
*            inputs registering for held down button press.
* usage    : Used in when setting timer count
* authors  : Joseph Matella
* version  : 0.1
* date     : 4212026
*-----------------------------------------------------------------------------
*/

bool Keypad_TransitionHigh(void) {
	// Create persistant variable to store previous state
	static bool prevKeypadState = 0;
	// Attain current state of keypad
	bool keypadState = Keypad_IsAnyKeyPressed();
	// Check for LOW to HIGH transition
	if (keypadState && !prevKeypadState) {
		prevKeypadState = keypadState;
		return 1;
	} else {
		prevKeypadState = keypadState;
		return 0;
	}
}

/*
 *-----------------------------------------------------------------------------
 * function : Keypad_WhichKeyIsPressed( )
 * INs      : none
 * OUTs     : returns number corresponding to the pressed key;
 * -1 if no key is pressed
 * action   : start columns low; iterate through columns setting each high and
 * checking if any row inputs are high, if so, use current row and column to
 * determine key press via switch statement decoder; return key value as int (0-11);
 * if no keypress detected, return -1; software for loop delay used to allow for
 * voltage settling
 *
 * usage 	: called in main.c to determine which key is pressed after confirming
 * a key was pressed with Keypad_IsAnyKeyPressed()
 *
 * authors  : Alex Tauber & Joseph Matella
 * version  : 0.1
 * date     : 4132026
 *--------------------------------------------------------------------------
 */

int Keypad_WhichKeyIsPressed(void) {

	// initialize variable to hold key value
	int key = 0;

	// ensure column pins start low
	COL_PORT -> BRR = (COL_PINS);

	// iterate through columns, setting each one high and checking for row inputs
	for ( uint8_t icol = 0; icol < NUM_OF_COLS; icol++ ) {

		// set particular column pin high
		COL_PORT -> BSRR = (GPIO_BSRR_BS0 << icol);

		/* allow column voltage to settle via software for loop delay
		(calibrated for 3.06 microseconds per iteration) */
		for ( uint16_t idx = 0; idx <  SETTLE_DELAY / 0.00000306; idx++ ) {}

		 // check if any row inputs are pulled high
		if ( (ROW_PORT -> IDR & ROW_PINS) != 0 ) {

			// decoder to determine row number from IDR
			switch ( ROW_PORT -> IDR & ROW_PINS ) {

				case 1 :key = (icol + 1); break;
				case 2 :key = (icol + 4); break;
				case 4 :key = (icol + 7); break;
				case 8 :
					if( icol==0 ){
						key = 0xA;
						break;
					} else if( icol == 2 ){
						key = 0xB;
						break;
					} else{
						key = 0;
						break;
					}

			}
			// clear columns when finished
			COL_PORT -> BRR = (COL_PINS);
			return key;
		}
		// clear columns before next iteration
		COL_PORT -> BRR = (COL_PINS);

	}
	//clear columns when finished
	COL_PORT -> BRR = (COL_PINS);

	// if no keypress detected, return -1
	return -1;
}

/*
*-----------------------------------------------------------------------------
* function : Keypad_SETPressed
* INs      : void
* OUTs     : bool indicating if SET key pressed
*            1 : SET key pressed
*            0 : Set key not pressed
* action   : Checks if SET key pressed
* usage    : Used in countdown loop to check if SET key pressed
* authors  : Joseph Matella
* version  : 0.1
* date     : 4212026
*-----------------------------------------------------------------------------
*/

bool Keypad_SETPressed(void) {
	// Ensure column pins start low
	COL_PORT -> BRR = (COL_PINS);

	// Set Column 1 HIGH
	COL_PORT -> BSRR = (GPIO_BSRR_BS0);

	/* allow column voltage to settle via software for loop delay
	(calibrated for 3.06 microseconds per iteration) */
	for ( uint16_t idx = 0; idx <  SETTLE_DELAY / 0.00000306; idx++ ) {}

	// Check if row 4  high
	if (ROW_PORT->IDR & (1 << 3)) {
		// Ensure column pins start low
		COL_PORT -> BRR = (COL_PINS);
		return 1;
	} else {
		// Ensure column are low
		COL_PORT -> BRR = (COL_PINS);
		return 0;
	}

}
