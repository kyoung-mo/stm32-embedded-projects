#include "ap.h"
#include "rtc.h"
#include "stm32f4xx_hal_rtc.h"
#include "usart.h"
#include "adc.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "i2c-lcd.h"

volatile uint16_t adc_values[1];
volatile DisplayMode_t current_mode = MODE_CLOCK;  // 현재 모드
volatile uint8_t mode_changed = 0;  // 모드 변경 플래그
char msg[128];

void apInit(void) {
  // ADC를 DMA 모드로 시작 (버퍼 주소와 크기 전달)
  if (HAL_ADC_Start_DMA(&hadc1, (uint32_t *)adc_values, 3) != HAL_OK) {
    Error_Handler();
  }
  LCD_Init(); // LCD 초기화
  printf("\r\n=== LCD I2C Test ===\r\n");

  LCD_PutCur(0, 0);                // 0번째 줄, 0번째 칸으로 이동
  LCD_SendString("Hello, mommers!"); // 이름 출력 예시

  LCD_PutCur(1, 0); // 1번째 줄, 0번째 칸으로 이동
  LCD_SendString("Library Mode :D");

  printf("Hello, mommers!\r\n");
  printf("Mode Change Ver :D\r\n");
  printf("Initial message displayed\r\n");
  
  HAL_Delay(2000); // 2초 대기


  LCD_PutCur(0, 0);
  LCD_SendString("System Ready");
  LCD_PutCur(1, 0);
  LCD_SendString("Press B1");
  
  HAL_Delay(2000); // 2초 대기
}

void apMain(void) {
  RTC_TimeTypeDef sTime = {0};
  RTC_DateTypeDef sDate = {0};
  volatile float temperature = 0.0f;
  char buffer[32];

  sDate.Year = (uint8_t)2026;
  sDate.Month = 1;
  sDate.Date = 23;
  sDate.WeekDay = 5;
  
  sTime.Hours = 14;
  sTime.Minutes = 33;
  sTime.Seconds = 0;



  
  if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BCD) != HAL_OK)
  {
    Error_Handler();
  }


  LCD_Clear();
    while(1)
    {
    if (mode_changed) {
            mode_changed = 0;
            LCD_Clear();
            HAL_Delay(100);
        }

    if (current_mode == MODE_CLOCK) {
            // ========== 시계 모드 ==========
            HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
            HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
            
            // 날짜 표시
            sprintf(buffer, "%04d-%02d-%02d", 
                    2026 + sDate.Year, sDate.Month, sDate.Date);
            LCD_SetCursor(0, 0);
            LCD_Print(buffer);
            
            // 시간 표시
            sprintf(buffer, "%02d:%02d:%02d", 
                    sTime.Hours, sTime.Minutes, sTime.Seconds);
            LCD_SetCursor(1, 0);
            LCD_Print(buffer);
            
            printf("Clock Mode: %s %s\r\n", buffer, buffer);
        }
    else if (current_mode == MODE_TEMPERATURE) {
            uint16_t temp_adc = adc_values[0];
            // ========== 온도 모드 ==========
            // 내부 온도 센서 읽기 (ADC 채널에 따라 다름)
            // STM32F411의 내부 온도 센서 공식:
            // Temperature (°C) = {(VSENSE - V25) / Avg_Slope} + 25
            // V25 = 0.76V, Avg_Slope = 2.5mV/°C
            
            float vsense = (temp_adc * 3.3f) / 4096.0f;  // ADC 값을 전압으로 변환
            temperature = ((vsense - 0.76f) / 0.0025f) + 25.0f;
            
            int temp_int = (int)temperature;           // 정수 부분
            int temp_frac = (int)((temperature - temp_int) * 10);  // 소수점 첫째자리

            // LCD 출력 - 첫 번째 줄
            sprintf(buffer, "ADC:%4u", temp_adc);
            LCD_SetCursor(0, 0);
            LCD_Print(buffer);
            
            // LCD 출력 - 두 번째 줄 (정수로 표시)
            sprintf(buffer, "%d.%dC", temp_int, temp_frac);  // 예: "25.3C"
            LCD_SetCursor(1, 0);
            LCD_Print(buffer);
            
            // 시리얼 출력
            printf("ADC:%4u | Temp:%d.%dC\r\n", temp_adc, temp_int, temp_frac);
        }
    HAL_Delay(1000);
    }
}

// 버튼 눌림 콜백 함수
void Button_Pressed_Callback(void) {
    // LED 토글로 버튼 동작 확인 (LED가 있다면)
    HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);  // 보드의 LED 핀
    
    // 모드 전환
    if (current_mode == MODE_CLOCK) {
        current_mode = MODE_TEMPERATURE;
        printf(">>> Mode changed to: TEMPERATURE <<<\r\n");
    } else {
        current_mode = MODE_CLOCK;
        printf(">>> Mode changed to: CLOCK <<<\r\n");
    }
    
    mode_changed = 1;
}