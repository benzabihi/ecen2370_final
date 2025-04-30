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
	StaticTouchData.orientation = STMPE811_Orientation_Portrait_2;

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
  STMPE811_TouchData td = { .orientation = STMPE811_Orientation_Portrait_2 };
  showMenu();

  while (1) {
    if (STMPE811_ReadTouch(&td) == STMPE811_State_Pressed) {
      // Portrait_1 already did: td.x = 239 - rawX; td.y = 319 - rawY
      // Undo the X inversion so 0…239 is left→right again:
      uint16_t x = td.x;
      uint16_t y = LCD_PIXEL_HEIGHT - td.y;

      // left button?
      if (x >= BTN_MARGIN
       && x <  BTN_MARGIN + BTN_WIDTH
       && y >= BTN_Y
       && y <  BTN_Y     + BTN_HEIGHT)
          return MODE_1P;
      // right button?
      if (x >= 2*BTN_MARGIN + BTN_WIDTH
       && x < 2*BTN_MARGIN + 2*BTN_WIDTH
       && y >= BTN_Y
       && y <  BTN_Y     + BTN_HEIGHT)
          return MODE_2P;
    }
    HAL_Delay(50);
  }
}


#define COLS           7
#define ROWS           6
#define CELL_SIZE      (LCD_PIXEL_WIDTH  / COLS)       // 240/7 = 34 px
#define BOARD_HEIGHT   (CELL_SIZE * ROWS)              // 34*6 = 204 px
#define DROP_BTN_H     40                              // bottom 40 px = “Drop” button
#define GRID_COLOR     LCD_COLOR_BLACK
#define BOARD_COLOR    LCD_COLOR_BLUE
#define HOLE_COLOR     LCD_COLOR_WHITE
#define COIN_RADIUS    ((CELL_SIZE/2) - 2)

static uint8_t board[ROWS][COLS];    // 0=empty, 1=player1, 2=player2/AI
static uint8_t curCol;               // 0…COLS-1
static uint8_t currentPlayer;        // 1 or 2

static void drawBoard(void) {
  // fill board background
  for (int y = 0; y < BOARD_HEIGHT; y++)
    for (int x = 0; x < LCD_PIXEL_WIDTH; x++)
      LCD_Draw_Pixel(x, y, BOARD_COLOR);

  // draw holes
  for (int r = 0; r < ROWS; r++) {
    for (int c = 0; c < COLS; c++) {
      int cx = c * CELL_SIZE + CELL_SIZE/2;
      int cy = r * CELL_SIZE + CELL_SIZE/2;
      LCD_Draw_Circle_Fill(cx, cy, COIN_RADIUS, HOLE_COLOR);
    }
  }

  // grid lines
  for (int c = 0; c <= COLS; c++)
    LCD_Draw_Vertical_Line(c*CELL_SIZE, 0, BOARD_HEIGHT, GRID_COLOR);
  for (int r = 0; r <= ROWS; r++)
    for (int x = 0; x < LCD_PIXEL_WIDTH; x++)
      LCD_Draw_Pixel(x, r*CELL_SIZE, GRID_COLOR);
}


static void drawHoverCoin(void) {
  // erase top row area
  for (int y = BOARD_HEIGHT; y < BOARD_HEIGHT + CELL_SIZE; y++)
    for (int x = 0; x < LCD_PIXEL_WIDTH; x++)
      LCD_Draw_Pixel(x, y, LCD_COLOR_WHITE);

  // draw the hover coin at (curCol, just above row 0)
  int cx = curCol * CELL_SIZE + CELL_SIZE/2;
  int cy = BOARD_HEIGHT + CELL_SIZE/2;
  uint16_t color = (currentPlayer == 1 ? LCD_COLOR_RED : LCD_COLOR_YELLOW);
  LCD_Draw_Circle_Fill(cx, cy, COIN_RADIUS, color);
}


static bool placeCoin(uint8_t col) {
  // from bottom up
  for (int r = ROWS-1; r >= 0; r--) {
    if (board[r][col] == 0) {
      board[r][col] = currentPlayer;
      // draw the coin in its slot
      int cx = col * CELL_SIZE + CELL_SIZE/2;
      int cy = r   * CELL_SIZE + CELL_SIZE/2;
      uint16_t color = (currentPlayer == 1 ? LCD_COLOR_RED : LCD_COLOR_YELLOW);
      LCD_Draw_Circle_Fill(cx, cy, COIN_RADIUS, color);
      return true;
    }
  }
  return false;  // column full
}


void playLoop(GameMode mode) {
  // clear state
  memset(board, 0, sizeof(board));
  curCol        = COLS/2;
  currentPlayer = 1;

  // initial draw
  LCD_Clear(0, LCD_COLOR_WHITE);
  drawBoard();
  drawHoverCoin();

  // draw “Drop” button
  for (int y = BOARD_HEIGHT; y < BOARD_HEIGHT + DROP_BTN_H; y++)
    for (int x = 0; x < LCD_PIXEL_WIDTH; x++)
      LCD_Draw_Pixel(x, y, GRID_COLOR);
  LCD_SetTextColor(BOARD_COLOR);
  LCD_SetFont(&Font16x24);
  // center “DROP”
  int tx = (LCD_PIXEL_WIDTH - 4*Font16x24.Width)/2;
  int ty = BOARD_HEIGHT + (DROP_BTN_H - Font16x24.Height)/2;
  LCD_DisplayChar(tx,   ty, 'D');
  LCD_DisplayChar(tx+16,ty, 'R');
  LCD_DisplayChar(tx+32,ty, 'O');
  LCD_DisplayChar(tx+48,ty, 'P');

  STMPE811_TouchData td = { .orientation = STMPE811_Orientation_Portrait_2 };

  while (1) {
    if (STMPE811_ReadTouch(&td) == STMPE811_State_Pressed) {
      uint16_t x = td.x;
      uint16_t y = LCD_PIXEL_HEIGHT - td.y;

      if (y >= BOARD_HEIGHT && y < BOARD_HEIGHT + DROP_BTN_H) {
        // drop
        if (placeCoin(curCol)) {
          // switch player (or insert AI turn here for 1-player)
          currentPlayer = (currentPlayer==1?2:1);
          drawHoverCoin();
        }
      } else {
        // move left/right
        if (x < LCD_PIXEL_WIDTH/2) {
          if (curCol > 0) curCol--;
        } else {
          if (curCol < COLS-1) curCol++;
        }
        drawHoverCoin();
      }
    }
    HAL_Delay(50);
  }
}


