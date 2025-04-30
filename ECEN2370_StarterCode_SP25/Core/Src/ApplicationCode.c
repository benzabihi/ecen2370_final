/*
 * ApplicationCode.c
 *
 *  Created on: Dec 30, 2023 (updated 11/12/2024) Thanks Donavon! 
 *      Author: Xavion
 */

#include "ApplicationCode.h"
#include "main.h"

/* Static variables */

extern RNG_HandleTypeDef hrng;
extern void initialise_monitor_handles(void); 

#if COMPILE_TOUCH_FUNCTIONS == 1
static STMPE811_TouchData StaticTouchData;
#endif // COMPILE_TOUCH_FUNCTION


static uint32_t wins1 = 0;
static uint32_t wins2 = 0;

void ApplicationInit(void)
{
	initialise_monitor_handles(); // Allows printf functionality
    LTCD__Init();
    LTCD_Layer_Init(0);
    LCD_Clear(0,LCD_COLOR_WHITE);

    #if COMPILE_TOUCH_FUNCTIONS == 1
	InitializeLCDTouch();


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
static void showWinScreen(uint8_t player) {
  LCD_Clear(0, LCD_COLOR_WHITE);
  LCD_SetTextColor(LCD_COLOR_BLACK);
  LCD_SetFont(&Font16x24);

  // Build the message
  char msg[20];
  int len = sprintf(msg, "Player %d wins!", player);

  // center it
  int msgWidth  = len * Font16x24.Width;
  int msgHeight = Font16x24.Height;
  int x = (LCD_PIXEL_WIDTH  - msgWidth)  / 2;
  int y = (LCD_PIXEL_HEIGHT - msgHeight) / 2;

  // draw each character
  for (int i = 0; i < len; i++) {
    LCD_DisplayChar(x + i*Font16x24.Width, y, msg[i]);
  }
}

static bool checkWin(uint8_t player) {
  // horizontal, vertical, and two diagonals
  const int dx[4] = { 1, 0, 1,  1 };
  const int dy[4] = { 0, 1, 1, -1 };

  for (int r = 0; r < ROWS; r++) {
    for (int c = 0; c < COLS; c++) {
      if (board[r][c] != player) continue;
      // try each direction
      for (int dir = 0; dir < 4; dir++) {
        int count = 1;
        for (int step = 1; step < 4; step++) {
          int nr = r + dy[dir]*step;
          int nc = c + dx[dir]*step;
          if (nr < 0 || nr >= ROWS || nc < 0 || nc >= COLS) break;
          if (board[nr][nc] != player) break;
          count++;
        }
        if (count == 4) return true;
      }
    }
  }
  return false;
}


// Shows running scores, round time, and a Restart button
static void finalScreen(uint8_t winner, uint32_t duration_s) {
  // Update totals
  if (winner == 1)        wins1++;
  else if (winner == 2)   wins2++;
  else /* winner==0 */    {/* ties++; */}

  // Clear to white
  LCD_Clear(0, LCD_COLOR_WHITE);

  // Centered “Game Over” message
  LCD_SetTextColor(LCD_COLOR_BLACK);
  LCD_SetFont(&Font16x24);
  const char *headline = (winner == 0) ? "Tie Game!" :
                         (winner == 1) ? "Red Wins!" : "Yellow Wins!";
  int hl_len = strlen(headline);
  int hl_x   = (LCD_PIXEL_WIDTH - hl_len*Font16x24.Width)/2;
  int hl_y   =  20;
  for (int i = 0; i < hl_len; i++)
    LCD_DisplayChar(hl_x + i*Font16x24.Width, hl_y, headline[i]);

  // Show scores
  char buf[32];
  // Player 1 / Red
  sprintf(buf, "Red: %lu", wins1);
  LCD_SetTextColor(LCD_COLOR_RED);
  int x = (LCD_PIXEL_WIDTH - strlen(buf)*Font16x24.Width)/2;
  int y = hl_y +  40;
  for (int i = 0; buf[i]; i++)
    LCD_DisplayChar(x + i*Font16x24.Width, y, buf[i]);

  // Player 2 / Yellow
  sprintf(buf, "Yellow: %lu", wins2);
  LCD_SetTextColor(LCD_COLOR_YELLOW);
  y += 30;
  x  = (LCD_PIXEL_WIDTH - strlen(buf)*Font16x24.Width)/2;
  for (int i = 0; buf[i]; i++)
    LCD_DisplayChar(x + i*Font16x24.Width, y, buf[i]);

  // Show round time
  sprintf(buf, "Time: %lus", duration_s);
  LCD_SetTextColor(LCD_COLOR_BLACK);
  y += 30;
  x  = (LCD_PIXEL_WIDTH - strlen(buf)*Font16x24.Width)/2;
  for (int i = 0; buf[i]; i++)
    LCD_DisplayChar(x + i*Font16x24.Width, y, buf[i]);

  // Draw a Restart button at bottom
  const int btnW = LCD_PIXEL_WIDTH - 2*BTN_MARGIN;
  const int btnH = BTN_HEIGHT;
  const int btnX = BTN_MARGIN;
  const int btnY = LCD_PIXEL_HEIGHT - BTN_HEIGHT - BTN_MARGIN;
  fillRect(btnX, btnY, btnW, btnH, LCD_COLOR_BLUE);
  const char *lbl = "Restart";
  x = btnX + (btnW - strlen(lbl)*Font16x24.Width)/2;
  y = btnY + (btnH - Font16x24.Height)/2;
  LCD_SetTextColor(LCD_COLOR_WHITE);
  for (int i = 0; lbl[i]; i++)
    LCD_DisplayChar(x + i*Font16x24.Width, y, lbl[i]);

  // Wait for touch on “Restart”
  STMPE811_TouchData td = { .orientation = STMPE811_Orientation_Portrait_2 };
  while (1) {
    if (STMPE811_ReadTouch(&td) == STMPE811_State_Pressed) {
      uint16_t tx = td.x;
      uint16_t ty = LCD_PIXEL_HEIGHT - td.y;
      if (tx >= btnX && tx < btnX + btnW &&
          ty >= btnY && ty < btnY + btnH) {
        break;
      }
    }
    HAL_Delay(50);
  }
}
static bool checkTie(void) {
  for (int r = 0; r < ROWS; r++)
    for (int c = 0; c < COLS; c++)
      if (board[r][c] == 0)
        return false;
  return true;
}


void playLoop(GameMode mode) {
    // 1) Start the timer
    uint32_t start_ms = HAL_GetTick();

    // 2) Clear and init game state
    memset(board, 0, sizeof(board));
    curCol = COLS/2;
    if (mode == MODE_1P) {
        // randomly choose who starts: 1 = human, 2 = AI
        uint32_t rnd;
        HAL_RNG_GenerateRandomNumber(&hrng, &rnd);
        currentPlayer = (rnd & 1) ? 2 : 1;
    } else {
        currentPlayer = 1;
    }

    // 3) Initial draw
    LCD_Clear(0, LCD_COLOR_WHITE);
    drawBoard();
    drawHoverCoin();


    // 5) Prepare touch state
    STMPE811_TouchData td = { .orientation = STMPE811_Orientation_Portrait_2 };

    // 6) Main game loop
    while (1) {
        // --- AI turn (1-Player mode only) ---
        if (mode == MODE_1P && currentPlayer == 2) {
            uint32_t rnd;
            uint8_t aiCol;
            // pick a random non-full column
            do {
                HAL_RNG_GenerateRandomNumber(&hrng, &rnd);
                aiCol = rnd % COLS;
            } while (!placeCoin(aiCol));

            // win?
            if (checkWin(2)) {
                finalScreen(2, (HAL_GetTick() - start_ms)/1000);
                return;
            }
            // tie?
            if (checkTie()) {
                finalScreen(0, (HAL_GetTick() - start_ms)/1000);
                return;
            }
            // hand back to human
            currentPlayer = 1;
            curCol = COLS/2;
            drawHoverCoin();
            continue;
        }

        // --- user turn (or 2-Player mode) ---

        // a) Drop with  button
        if (HAL_GPIO_ReadPin(B1_GPIO_Port, B1_Pin) == GPIO_PIN_SET) {
            if (placeCoin(curCol)) {
                // win?
                if (checkWin(currentPlayer)) {
                    finalScreen(currentPlayer, (HAL_GetTick() - start_ms)/1000);
                    return;
                }
                // tie?
                if (checkTie()) {
                    finalScreen(0, (HAL_GetTick() - start_ms)/1000);
                    return;
                }
                // next turn
                if (mode == MODE_1P)
                    currentPlayer = 2;               // AI next
                else
                    currentPlayer = (currentPlayer==1?2:1);  // swap players
                curCol = COLS/2;
                drawHoverCoin();
            }
            // simple debounce: wait for release
            while (HAL_GPIO_ReadPin(B1_GPIO_Port, B1_Pin) == GPIO_PIN_SET)
                HAL_Delay(10);
        }
        // b) Otherwise, slide hover-coin left/right via touch
        else if (STMPE811_ReadTouch(&td) == STMPE811_State_Pressed) {
            uint16_t x = td.x;
            // flip Y if you ever need it: uint16_t y = LCD_PIXEL_HEIGHT - td.y;
            if (x < LCD_PIXEL_WIDTH/2) {
                if (curCol > 0) curCol--;
            } else {
                if (curCol < COLS-1) curCol++;
            }
            drawHoverCoin();
        }

        HAL_Delay(50);
    }
}


