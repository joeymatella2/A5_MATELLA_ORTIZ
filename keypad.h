/*
 *******************************************************************************
 * @file           : keypad.h
 * @brief          : header file for keypad.c; contains function prototypes for
 * keypad configuration and keypress detection functions; as well as conveninent
 * masks and defines
 * project         : EE 329 S'26 A3
 * authors         : Alex Tauber and Joseph Matella
 * version         : 0.2
 * date            : 230413
 * compiler        : STM32CubeIDE v.1.19.0 Build: 14980_20230301_1550 (UTC)
 * target          : NUCLEO-L4A6ZG
 * clocks          : 4 MHz MSI to AHB2
 * @attention      : (c) 2026 STMicroelectronics.  All rights reserved.
 *******************************************************************************
 * Header format adapted from [Code Appendix by Kevin Vo] pg 5
 */

#ifndef INC_KEYPAD_H_
#define INC_KEYPAD_H_

#include "stm32l4xx_hal.h"
#define ROW_PINS 0xF
#define COL_PINS 0x7

#define NUM_OF_ROWS 4
#define NUM_OF_COLS 3

#define ROW_PORT GPIOA
#define COL_PORT GPIOB

// Delay time in seconds
#define DEBOUNCE_DELAY 0.001
#define SETTLE_DELAY 0.001

typedef int bool;

// Keypad.c Function declarations
void Keypad_Config(void);
bool Keypad_IsAnyKeyPressed(void);
bool Keypad_TransitionHigh(void);
int Keypad_WhichKeyIsPressed(void);
bool Keypad_SETPressed(void);

#endif /* INC_KEYPAD_H_ */
