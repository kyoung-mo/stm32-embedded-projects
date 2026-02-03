#include "ap.h"
#include <stdint.h>
#include <stdio.h>  // sprintf 사용을 위해
#include <string.h> // strlen 사용을 위해

volatile uint16_t adc_values[3];
char msg[128];

void apInit(void) {
  // ADC를 DMA 모드로 시작 (버퍼 주소와 크기 전달)
  if (HAL_ADC_Start_DMA(&hadc1, (uint32_t *)adc_values, 3) != HAL_OK) {
    Error_Handler();
  }
}

void apMain(void) {
  char tx_buffer[50]; // 출력 버퍼
  while (1) {
    // X축 값 출력
    int len_x = sprintf(tx_buffer, ">x:%u\r\n", adc_values[0]);
    HAL_UART_Transmit(&huart2, (uint8_t *)tx_buffer, len_x, 100);

    // Y축 값 출력
    int len_y = sprintf(tx_buffer, ">y:%u\r\n", adc_values[1]);
    HAL_UART_Transmit(&huart2, (uint8_t *)tx_buffer, len_y, 100);

    // 온도 계산 (STM32F411 Datasheet 기준)
    // 공식: Temp = {(Vsense - V25) / Avg_Slope} + 25
    // V25 (25도일 때 전압) = 0.76V (760mV)
    // Avg_Slope (기울기) = 2.5mV/도
    
    // (1) Raw 값을 전압(mV)으로 변환 (3.3V 기준)
    //float Vsense = (float)adc_values[2] * 3300.0 / 4095.0;
    
    // (2) 섭씨 온도로 변환
    //volatile float temperature = ((Vsense - 760.0) / 2.5) + 25.0;
    //volatile int len_t = sprintf(tx_buffer, ">t:%f\r\n", temperature);
    int len_t = sprintf(tx_buffer, ">t:%u\r\n", adc_values[2]);
    HAL_UART_Transmit(&huart2, (uint8_t *)tx_buffer, len_t, 100);

    
    HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), 10);

    HAL_Delay(100); // 0.5초 대기 (너무 빠른 출력을 방지)
  }
}