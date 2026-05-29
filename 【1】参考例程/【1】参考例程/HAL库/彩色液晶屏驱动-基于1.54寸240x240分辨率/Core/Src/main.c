/***
	*************************************************************************************************
	*	@file  	main.c
	*	@version V1.0
	*	@brief   Snake Game - ST7789 240x240 LCD
	************************************************************************************************
	*  @description
	*	Platform: STM32F407ZGT6 + 1.54" 240x240 LCD (ST7789)
	*	Control: Press KEY to cycle direction (Right->Down->Left->Up->Right...)
	*	LED: Blinks on game over
	************************************************************************************************
***/

#include "main.h"
#include "led.h"
#include "usart.h"
#include "key.h"
#include "lcd_spi_154.h"
#include <stdlib.h>
#include <stdio.h>

/********************************************** Snake Game Config *******************************************/

#define CELL_SIZE     12
#define GRID_COLS     20
#define GRID_ROWS     18
#define GRID_Y        24
#define MAX_SNAKE     400

#define SNAKE_HEAD    0x00FF00
#define SNAKE_BODY    0x00CC00
#define FOOD_COLOR    0xFF0000
#define BG_COLOR      0x1A1A2E
#define BORDER_COLOR  0x333355
#define SCORE_COLOR   0xFFFF00
#define STATUS_BG     0x0F0F1F

#define DIR_RIGHT      0
#define DIR_DOWN       1
#define DIR_LEFT       2
#define DIR_UP         3

int snake_x[MAX_SNAKE];
int snake_y[MAX_SNAKE];
int snake_len;
int snake_dir;
int food_x, food_y;
int score;
int game_over;

/********************************************** Function Declarations *******************************************/

void SystemClock_Config(void);
void Snake_Init(void);
void Snake_Restart(void);
void Snake_PlaceFood(void);
void Snake_Update(void);
void Snake_Draw(void);
void Snake_DrawStatus(void);
void Snake_DrawGameOver(void);
uint8_t Snake_ReadKey(void);

/********************************************** Main *******************************************/

int main(void)
{
	HAL_Init();
	SystemClock_Config();
	LED_Init();
	USART1_Init();

	SPI_LCD_Init();
	KEY_Init();

	Snake_Init();

	while (1)
	{
		if (Snake_ReadKey())
		{
			if (game_over)
			{
				Snake_Restart();
			}
			else
			{
				snake_dir = (snake_dir + 1) % 4;
			}
		}

		if (!game_over)
		{
			Snake_Update();
			Snake_Draw();
			Snake_DrawStatus();
			HAL_Delay(180);
		}
		else
		{
			HAL_GPIO_TogglePin(LED1_PORT, LED1_PIN);
			HAL_Delay(300);
		}
	}
}

/********************************************** Snake Init *******************************************/

void Snake_Init(void)
{
	srand(12345);
	Snake_Restart();
}

void Snake_Restart(void)
{
	snake_len = 3;
	snake_dir = DIR_RIGHT;
	score = 0;
	game_over = 0;

	int start_x = GRID_COLS / 2;
	int start_y = GRID_ROWS / 2;
	for (int i = 0; i < snake_len; i++)
	{
		snake_x[i] = start_x - i;
		snake_y[i] = start_y;
	}

	LCD_SetBackColor(BG_COLOR);
	LCD_Clear();

	LCD_SetColor(STATUS_BG);
	LCD_FillRect(0, 0, 240, GRID_Y - 2);

	LCD_SetColor(BORDER_COLOR);
	LCD_DrawLine_H(0, GRID_Y - 1, 240);

	Snake_PlaceFood();

	HAL_GPIO_WritePin(LED1_PORT, LED1_PIN, GPIO_PIN_SET);
}

/********************************************** Place Food *******************************************/

void Snake_PlaceFood(void)
{
	int valid;
	do {
		valid = 1;
		food_x = rand() % GRID_COLS;
		food_y = rand() % GRID_ROWS;

		for (int i = 0; i < snake_len; i++)
		{
			if (snake_x[i] == food_x && snake_y[i] == food_y)
			{
				valid = 0;
				break;
			}
		}
	} while (!valid);
}

/********************************************** Game Logic Update *******************************************/

void Snake_Update(void)
{
	int new_x = snake_x[0];
	int new_y = snake_y[0];

	switch (snake_dir)
	{
		case DIR_RIGHT: new_x++; break;
		case DIR_DOWN:  new_y++; break;
		case DIR_LEFT:  new_x--; break;
		case DIR_UP:    new_y--; break;
	}

	if (new_x < 0 || new_x >= GRID_COLS || new_y < 0 || new_y >= GRID_ROWS)
	{
		game_over = 1;
		Snake_DrawGameOver();
		return;
	}

	for (int i = 0; i < snake_len; i++)
	{
		if (snake_x[i] == new_x && snake_y[i] == new_y)
		{
			game_over = 1;
			Snake_DrawGameOver();
			return;
		}
	}

	for (int i = snake_len; i > 0; i--)
	{
		snake_x[i] = snake_x[i - 1];
		snake_y[i] = snake_y[i - 1];
	}
	snake_x[0] = new_x;
	snake_y[0] = new_y;

	if (new_x == food_x && new_y == food_y)
	{
		snake_len++;
		score += 10;
		if (snake_len >= MAX_SNAKE) snake_len = MAX_SNAKE;
		Snake_PlaceFood();
	}
}

/********************************************** Draw Game Frame *******************************************/

void Snake_Draw(void)
{
	uint16_t x, y;
	static int old_tail_x = -1, old_tail_y = -1;
	static int prev_len = 0;

	if (snake_len <= prev_len && old_tail_x >= 0 && old_tail_y >= 0)
	{
		x = old_tail_x * CELL_SIZE;
		y = old_tail_y * CELL_SIZE + GRID_Y;
		LCD_SetColor(BG_COLOR);
		LCD_FillRect(x, y, CELL_SIZE, CELL_SIZE);
		LCD_SetColor(BORDER_COLOR);
		LCD_DrawRect(x, y, CELL_SIZE, CELL_SIZE);
	}

	prev_len = snake_len;
	old_tail_x = snake_x[snake_len - 1];
	old_tail_y = snake_y[snake_len - 1];

	for (int i = snake_len - 1; i > 0; i--)
	{
		x = snake_x[i] * CELL_SIZE;
		y = snake_y[i] * CELL_SIZE + GRID_Y;
		LCD_SetColor(SNAKE_BODY);
		LCD_FillRect(x + 1, y + 1, CELL_SIZE - 2, CELL_SIZE - 2);
	}

	x = snake_x[0] * CELL_SIZE;
	y = snake_y[0] * CELL_SIZE + GRID_Y;
	LCD_SetColor(SNAKE_HEAD);
	LCD_FillRect(x + 1, y + 1, CELL_SIZE - 2, CELL_SIZE - 2);

	int eye_x1 = x + 2, eye_y1 = y + 2;
	int eye_x2 = x + 2, eye_y2 = y + 8;
	switch (snake_dir)
	{
		case DIR_RIGHT: eye_x1 = x + 8; eye_y1 = y + 2; eye_x2 = x + 8; eye_y2 = y + 8; break;
		case DIR_DOWN:  eye_x1 = x + 2; eye_y1 = y + 8; eye_x2 = x + 8; eye_y2 = y + 8; break;
		case DIR_LEFT:  eye_x1 = x + 2; eye_y1 = y + 2; eye_x2 = x + 2; eye_y2 = y + 8; break;
		case DIR_UP:    eye_x1 = x + 2; eye_y1 = y + 2; eye_x2 = x + 8; eye_y2 = y + 2; break;
	}
	LCD_SetColor(LCD_WHITE);
	LCD_FillRect(eye_x1, eye_y1, 2, 2);
	LCD_FillRect(eye_x2, eye_y2, 2, 2);

	x = food_x * CELL_SIZE;
	y = food_y * CELL_SIZE + GRID_Y;
	LCD_SetColor(FOOD_COLOR);
	LCD_FillRect(x + 1, y + 1, CELL_SIZE - 2, CELL_SIZE - 2);
	LCD_SetColor(0xFF6666);
	LCD_FillRect(x + 4, y + 4, 4, 4);
}

/********************************************** Draw Status Bar *******************************************/

void Snake_DrawStatus(void)
{
	char buf[16];

	LCD_SetColor(STATUS_BG);
	LCD_FillRect(0, 0, 240, GRID_Y - 2);

	LCD_SetAsciiFont(&ASCII_Font16);
	LCD_SetColor(0x00FFAA);
	LCD_DisplayString(4, 4, "SNAKE");

	LCD_SetColor(SCORE_COLOR);
	sprintf(buf, "Score:%d", score);
	LCD_DisplayString(100, 4, buf);

	sprintf(buf, "Len:%d", snake_len);
	LCD_DisplayString(175, 4, buf);

	LCD_SetColor(BORDER_COLOR);
	LCD_DrawLine_H(0, GRID_Y - 1, 240);
}

/********************************************** Game Over Screen *******************************************/

void Snake_DrawGameOver(void)
{
	LCD_SetColor(0x000010);
	for (int row = 0; row < GRID_ROWS; row++)
	{
		LCD_FillRect(0, row * CELL_SIZE + GRID_Y, 240, CELL_SIZE / 2);
	}

	LCD_SetBackColor(0x000010);
	LCD_SetAsciiFont(&ASCII_Font24);
	LCD_SetColor(LCD_RED);
	LCD_DisplayString(55, GRID_Y + 40, "GAME OVER");

	char buf[24];
	LCD_SetAsciiFont(&ASCII_Font20);
	LCD_SetColor(LCD_YELLOW);
	sprintf(buf, "Score: %d", score);
	LCD_DisplayString(55, GRID_Y + 80, buf);

	LCD_SetAsciiFont(&ASCII_Font16);
	LCD_SetColor(0xCCCCCC);
	LCD_DisplayString(30, GRID_Y + 120, "Press KEY to restart");

	LCD_SetBackColor(BG_COLOR);

	HAL_GPIO_WritePin(LED1_PORT, LED1_PIN, GPIO_PIN_RESET);
}

/********************************************** Non-blocking Key Read *******************************************/

uint8_t Snake_ReadKey(void)
{
	if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_15) == GPIO_PIN_RESET)
	{
		HAL_Delay(15);
		if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_15) == GPIO_PIN_RESET)
		{
			while (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_15) == GPIO_PIN_RESET);
			return 1;
		}
	}
	return 0;
}

/********************************************** System Clock Config *******************************************/

void SystemClock_Config(void)
{
	RCC_OscInitTypeDef RCC_OscInitStruct = {0};
	RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

	__HAL_RCC_PWR_CLK_ENABLE();
	__HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
	RCC_OscInitStruct.HSEState = RCC_HSE_ON;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
	RCC_OscInitStruct.PLL.PLLM = 8;
	RCC_OscInitStruct.PLL.PLLN = 336;
	RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
	RCC_OscInitStruct.PLL.PLLQ = 4;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
	{
		Error_Handler();
	}

	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
		| RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
	{
		Error_Handler();
	}
}

void Error_Handler(void)
{
	__disable_irq();
	while (1) {}
}

/************************ (C) COPYRIGHT ************************END OF FILE****/
