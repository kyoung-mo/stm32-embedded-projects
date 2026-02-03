#include "i2c-lcd.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

extern I2C_HandleTypeDef hi2c1; // main.c에 정의된 핸들러 가져오기

#define SLAVE_ADDRESS_LCD 0x4E // 0x27 주소 << 1 = 0x4E (또는 0x3F << 1 = 0x7E)

static uint8_t lcd_backlight = LCD_BACKLIGHT;

void LCD_SendCmd(char cmd) {
  char data_u, data_l;
  uint8_t data_t[4];
  data_u = (cmd & 0xf0);
  data_l = ((cmd << 4) & 0xf0);
  data_t[0] = data_u | 0x0C; // en=1, rs=0
  data_t[1] = data_u | 0x08; // en=0, rs=0
  data_t[2] = data_l | 0x0C; // en=1, rs=0
  data_t[3] = data_l | 0x08; // en=0, rs=0
  HAL_I2C_Master_Transmit(&hi2c1, SLAVE_ADDRESS_LCD, data_t, 4, 100);
}

void LCD_SendData(char data) {
  char data_u, data_l;
  uint8_t data_t[4];
  data_u = (data & 0xf0);
  data_l = ((data << 4) & 0xf0);
  data_t[0] = data_u | 0x0D; // en=1, rs=1
  data_t[1] = data_u | 0x09; // en=0, rs=1
  data_t[2] = data_l | 0x0D; // en=1, rs=1
  data_t[3] = data_l | 0x09; // en=0, rs=1
  HAL_I2C_Master_Transmit(&hi2c1, SLAVE_ADDRESS_LCD, (uint8_t *)data_t, 4, 100);
}

static void LCD_SendInternal(uint8_t data, uint8_t flags) {
  uint8_t up = data & 0xF0;
  uint8_t lo = (data << 4) & 0xF0;

  uint8_t data_arr[4];
  data_arr[0] = up | flags | En | lcd_backlight;
  data_arr[1] = up | flags | lcd_backlight;
  data_arr[2] = lo | flags | En | lcd_backlight;
  data_arr[3] = lo | flags | lcd_backlight;

  HAL_I2C_Master_Transmit(&hi2c1, LCD_I2C_ADDR, data_arr, 4, 100);
}

static void LCD_SendCommand(uint8_t cmd) { LCD_SendInternal(cmd, 0); }

void LCD_Init(void) {
  // 1. 전원 인가 후 안정화 대기
  HAL_Delay(50);

  // 2. 초기화 매직 시퀀스 (0x33, 0x32)
  // 이 과정은 LCD가 8비트 모드인지 4비트 모드인지 헷갈려하는 상태를
  // 정리해줍니다.
  LCD_SendCmd(0x33);
  HAL_Delay(5);
  LCD_SendCmd(0x32);
  HAL_Delay(5);

  // 3. 이제 완벽한 4비트 모드로 설정됨
  LCD_SendCmd(0x28); // Function Set: 4-bit mode, 2 lines, 5x8 font
  HAL_Delay(1);

  LCD_SendCmd(0x08); // Display Off (화면 끄기)
  HAL_Delay(1);

  LCD_SendCmd(0x01); // Clear Display (화면 지우기 - 중요!)
  HAL_Delay(3);      // Clear 명령은 2ms 이상 시간이 필요함 (넉넉히 3ms)

  LCD_SendCmd(0x06); // Entry Mode Set (글자 쓰면 커서 오른쪽 이동)
  HAL_Delay(1);

  LCD_SendCmd(0x0C); // Display On, Cursor Off, Blink Off
}
void LCD_Init2(void) {
  LCD_SendCmd(0x33); // 초기화 시퀀스
  LCD_SendCmd(0x32);
  HAL_Delay(50);
  LCD_SendCmd(0x28); // 4비트 모드
  HAL_Delay(50);
  LCD_SendCmd(0x01); // 화면 클리어
  HAL_Delay(50);
  LCD_SendCmd(0x06); // 입력 모드 설정
  HAL_Delay(50);
  LCD_SendCmd(0x0C); // 디스플레이 ON, 커서 OFF
  HAL_Delay(50);
  LCD_SendCmd(0x02); // 홈으로 이동
  HAL_Delay(50);
}

void LCD_SendString(char *str) {
  while (*str)
    LCD_SendData(*str++);
}

void LCD_PutCur(int row, int col) {
  switch (row) {
  case 0:
    col |= 0x80;
    break;
  case 1:
    col |= 0xC0;
    break;
  }
  LCD_SendCmd(col);
}

void LCD_Clear(void) {
  LCD_SendCmd(0x01); // 전체 지우기
  HAL_Delay(2);
}

void LCD_SetCursor(uint8_t row, uint8_t col) {
  uint8_t row_offsets[] = {0x00, 0x40};
  if (row >= 2)
    row = 0;
  if (col >= 16)
    col = 0;
  LCD_SendCommand(LCD_SETDDRAM_ADDR | (col + row_offsets[row]));
}

void LCD_Print(const char *str) {
  while (*str) {
    LCD_SendData(*str++);
  }
}

void LCD_PrintNum(int num) {
  char buffer[50];
  sprintf(buffer, "%d", num);
  LCD_Print(buffer);
}

void LCD_Backlight(uint8_t state) {
  if (state) {
    lcd_backlight = LCD_BACKLIGHT;
  } else {
    lcd_backlight = LCD_NO_BACKLIGHT;
  }
  LCD_SendCommand(0);
}