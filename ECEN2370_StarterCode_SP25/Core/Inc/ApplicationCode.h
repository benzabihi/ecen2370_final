/*
 * ApplicationCode.h
 *
 *  Created on: Dec 30, 2023
 *      Author: Xavion
 */

#include "LCD_Driver.h"
#include "stm32f4xx_hal.h"
#include "LCD_Driver.h"
#include "stmpe811.h"

#include <stdio.h>


#ifndef INC_APPLICATIONCODE_H_
#define INC_APPLICATIONCODE_H_

void ApplicationInit(void);
void LCD_Visual_Demo(void);

#if (COMPILE_TOUCH_FUNCTIONS == 1)
void LCD_Touch_Polling_Demo(void);
#endif // (COMPILE_TOUCH_FUNCTIONS == 1)



// --- Connect-4 menu API ---
typedef enum {
    MODE_1P = 0,
    MODE_2P
} GameMode;

// Draw the two buttons (“1-Player” / “2-Player”)
void showMenu(void);


// Block until user taps a button, then return the chosen mode
GameMode menuLoop(void);

// Start the gameplay screen in either 1- or 2-player mode
void playLoop(GameMode mode);
#endif /* INC_APPLICATIONCODE_H_ */
