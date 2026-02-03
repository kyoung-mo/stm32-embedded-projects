#ifndef INC_I2C_LCD_H_
#define INC_I2C_LCD_H_

#include "i2c.h"
#include "main.h"
#include "stm32f4xx_hal.h" // MCU 제품군에 따라 변경 (F1, F4, G4 등)
#include <time.h>

// LCD I2C 주소 (스캔 결과: 0x27)
#define LCD_I2C_ADDR (0x27 << 1)

// LCD 명령어
#define LCD_CLEAR 0x01
#define LCD_HOME 0x02
#define LCD_ENTRY_MODE 0x04
#define LCD_DISPLAY_CONTROL 0x08
#define LCD_CURSOR_SHIFT 0x10
#define LCD_FUNCTION_SET 0x20
#define LCD_SETCGRAM_ADDR 0x40
#define LCD_SETDDRAM_ADDR 0x80

// Entry Mode
#define LCD_ENTRY_LEFT 0x02
#define LCD_ENTRY_SHIFT_DEC 0x00

// Display Control
#define LCD_DISPLAY_ON 0x04
#define LCD_CURSOR_OFF 0x00
#define LCD_BLINK_OFF 0x00

// Function Set
#define LCD_4BIT_MODE 0x00
#define LCD_2LINE 0x08
#define LCD_5x8DOTS 0x00

// Backlight
#define LCD_BACKLIGHT 0x08
#define LCD_NO_BACKLIGHT 0x00

// Control bits
#define En 0x04
#define Rw 0x02
#define Rs 0x01

void LCD_Init(void);               // 초기화
void LCD_SendCmd(char cmd);        // 명령어 전송
void LCD_SendData(char data);      // 데이터(글자) 전송
void LCD_SendString(char *str);    // 문자열 전송
void LCD_PutCur(int row, int col); // 커서 위치 이동 (0~1행, 0~15열)
void LCD_Clear(void);              // 화면 지우기
void LCD_SetCursor(uint8_t row, uint8_t col);
void LCD_Print(const char *str);
void LCD_PrintNum(int num);
void LCD_Backlight(uint8_t state);

#endif /* INC_I2C_LCD_H_ */