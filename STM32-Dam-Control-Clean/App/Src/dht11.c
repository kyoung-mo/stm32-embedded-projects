#include "dht11.h"
#include "stm32f4xx_hal_gpio.h"
#include "tim.h"
#include <stdio.h>

// 마이크로초 지연 함수 (htim2 사용)
void delay_us(uint16_t us) {
    __HAL_TIM_SET_COUNTER(&htim2, 0);
    while (__HAL_TIM_GET_COUNTER(&htim2) < us);
}

void DHT11_Init(void) {
    HAL_TIM_Base_Start(&htim2);
    
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = DHT11_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP; // 기본 상태 HIGH 유지
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(DHT11_PORT, &GPIO_InitStruct);
    
    HAL_GPIO_WritePin(DHT11_PORT, DHT11_PIN, GPIO_PIN_SET);
    HAL_Delay(1000); // 센서 안정화
}

uint8_t DHT11_Read(DHT11_Data_t *data) {
    uint8_t dht_data[5] = {0};
    uint32_t timeout = 0;
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // 1. MCU 시작 신호 전송
    GPIO_InitStruct.Pin = DHT11_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(DHT11_PORT, &GPIO_InitStruct);

    HAL_GPIO_WritePin(DHT11_PORT, DHT11_PIN, GPIO_PIN_RESET);
    HAL_Delay(18); // 최소 18ms 유지
    HAL_GPIO_WritePin(DHT11_PORT, DHT11_PIN, GPIO_PIN_SET);
    delay_us(30);  // 20~40us 대기

    // 2. MCU 수신 모드로 전환
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(DHT11_PORT, &GPIO_InitStruct);

    // 3. 센서 응답 확인 (LOW -> HIGH)
    timeout = 0;
    while(HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN) == GPIO_PIN_SET) {
        if(++timeout > 10000) return 0; // 응답 없음
    }
    timeout = 0;
    while(HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN) == GPIO_PIN_RESET) {
        if(++timeout > 10000) return 0;
    }
    timeout = 0;
    while(HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN) == GPIO_PIN_SET) {
        if(++timeout > 10000) return 0;
    }

    // 4. 40비트 데이터 읽기
    for(int j = 0; j < 5; j++) {
        for(int i = 7; i >= 0; i--) {
            // 비트 시작 (LOW 대기)
            while(HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN) == GPIO_PIN_RESET);
            
            // 비트 길이 측정 (HIGH 유지 시간)
            delay_us(40); // 40us 대기 후 샘플링
            
            if(HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN) == GPIO_PIN_SET) {
                dht_data[j] |= (1 << i);
                // 나머지 HIGH 시간 대기 (타임아웃 처리)
                timeout = 0;
                while(HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN) == GPIO_PIN_SET) {
                    if(++timeout > 10000) return 0;
                }
            }
        }
    }

    // 5. 체크섬 확인
    if(dht_data[4] == (uint8_t)(dht_data[0] + dht_data[1] + dht_data[2] + dht_data[3])) {
        data->humidity = dht_data[0];
        data->temperature = dht_data[2];
        return 1;
    }


    return 0;
}