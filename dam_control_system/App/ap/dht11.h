#ifndef DHT11_H
#define DHT11_H

#include "main.h"
#include <stdint.h>

// DHT11 핀 정의 (PA1 → PB1로 변경)
#define DHT11_PORT GPIOB  // ⭐ GPIOA → GPIOB
#define DHT11_PIN GPIO_PIN_1

// DHT11 데이터 구조체
typedef struct {
    uint8_t humidity;       // 습도 (%)
    uint8_t temperature;    // 온도 (°C)
    uint8_t checksum_ok;    // 체크섬 검증 결과
} DHT11_Data_t;

// 함수 프로토타입
void DHT11_Init(void);
uint8_t DHT11_Read(DHT11_Data_t *data);

#endif // DHT11_H