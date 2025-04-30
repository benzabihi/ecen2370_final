/*
 * ApplicationCode.c
 *
 *  Created on: Dec 30, 2023 (updated 11/12/2024) Thanks Donavon! 
 *      Author: Xavion
 */

#include "ApplicationCode.h"

/* Static variables */


extern void initialise_monitor_handles(void); 

#if COMPILE_TOUCH_FUNCTIONS == 1
static STMPE811_TouchData StaticTouchData;
#endif // COMPILE_TOUCH_FUNCTIONS

void ApplicationInit(void)
{
	initialise_monitor_handles(); // Allows printf functionality
    LTCD__Init();
    LTCD_Layer_Init(0);
    LCD_Clear(0,LCD_COLOR_WHITE);

    #if COMPILE_TOUCH_FUNCTIONS == 1
	InitializeLCDTouch();

	// This is the orientation for the board to be direclty up where the buttons are vertically above the screen
	// Top left would be low x value, high y value. Bottom right would be low x value, low y value.
	StaticTouchData.orientation = STMPE811_Orientation_Portrait_1;

	#endif // COMPILE_TOUCH_FUNCTIONS
}

void LCD_Visual_Demo(void)
{
	visualDemo();
}

#if COMPILE_TOUCH_FUNCTIONS == 1
void LCD_Touch_Polling_Demo(void)
{
	LCD_Clear(0,LCD_COLOR_GREEN);
	while (1) {
		/* If touch pressed */
		if (returnTouchStateAndLocation(&StaticTouchData) == STMPE811_State_Pressed) {
			/* Touch valid */
			printf("\nX: %03d\nY: %03d\n", StaticTouchData.x, StaticTouchData.y);
			LCD_Clear(0, LCD_COLOR_RED);
		} else {
			/* Touch not pressed */
			printf("Not Pressed\n\n");
			LCD_Clear(0, LCD_COLOR_GREEN);
		}
	}
}
#endif // COMPILE_TOUCH_FUNCTIONS

// Simple filled-rectangle
static void fillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t col) {
  for (uint16_t yy = y; yy < y+h; yy++)
    for (uint16_t xx = x; xx < x+w; xx++)
      LCD_Draw_Pixel(xx, yy, col);
}


#define BTN_MARGIN    20
#define BTN_HEIGHT    60
#define BTN_WIDTH     ((LCD_PIXEL_WIDTH - 3*BTN_MARGIN) / 2)  // (240-60)/2 = 90 px
#define BTN_Y         100

// Draw two big buttons
void showMenu(void) {
  LCD_Clear(0, LCD_COLOR_WHITE);

  // left button
  fillRect(BTN_MARGIN, BTN_Y,
           BTN_WIDTH, BTN_HEIGHT,
           LCD_COLOR_BLUE);

  // right button
  fillRect(2*BTN_MARGIN + BTN_WIDTH, BTN_Y,
           BTN_WIDTH, BTN_HEIGHT,
           LCD_COLOR_GREEN);

  // text setup
  LCD_SetTextColor(LCD_COLOR_WHITE);
  LCD_SetFont(&Font16x24);

  // label "1P" centered in left button
  uint16_t textX1 = BTN_MARGIN
                   + (BTN_WIDTH  - 2*Font16x24.Width) / 2;
  uint16_t textY  = BTN_Y
                   + (BTN_HEIGHT - Font16x24.Height) / 2;
  LCD_DisplayChar(textX1,              textY, '1');
  LCD_DisplayChar(textX1 + Font16x24.Width, textY, 'P');

  // label "2P" centered in right button
  uint16_t base2  = 2*BTN_MARGIN + BTN_WIDTH;
  uint16_t textX2 = base2
                   + (BTN_WIDTH  - 2*Font16x24.Width) / 2;
  LCD_DisplayChar(textX2,              textY, '2');
  LCD_DisplayChar(textX2 + Font16x24.Width, textY, 'P');
}


// Wait for a touch and map it to a button
GameMode menuLoop(void) {
  STMPE811_TouchData td = { .orientation = STMPE811_Orientation_Portrait_1 };
  showMenu();

  while (1) {
    if (STMPE811_ReadTouch(&td) == STMPE811_State_Pressed) {
      // Portrait_1 already did: td.x = 239 - rawX; td.y = 319 - rawY
      // Undo the X inversion so 0…239 is left→right again:
      uint16_t x = (LCD_PIXEL_WIDTH - 1) - td.x;
      uint16_t y = td.y;

      // left button?
      if (x >= BTN_MARGIN
       && x < BTN_MARGIN + BTN_WIDTH
       && y >= BTN_Y
       && y < BTN_Y + BTN_HEIGHT)
        return MODE_1P;

      // right button?
      if (x >= 2*BTN_MARGIN + BTN_WIDTH
       && x < 2*BTN_MARGIN + 2*BTN_WIDTH
       && y >= BTN_Y
       && y < BTN_Y + BTN_HEIGHT)
        return MODE_2P;
    }
    HAL_Delay(50);
  }
}


