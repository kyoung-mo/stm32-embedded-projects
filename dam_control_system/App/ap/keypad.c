#include "keypad.h"
#include "stm32f4xx_hal.h"

// =======================================
// STM32F4 핀 매핑 (CubeMX 기준)
// =======================================
GPIO_TypeDef* ROW_Ports[] = {GPIOA, GPIOA, GPIOA, GPIOA}; // PA8, PA9, PA10, PA11
uint16_t ROW_Pins[] = {GPIO_PIN_8, GPIO_PIN_9, GPIO_PIN_10, GPIO_PIN_11};

GPIO_TypeDef* COL_Ports[] = {GPIOB, GPIOB, GPIOB, GPIOB}; // PB5, PB6, PB12, PB13
uint16_t COL_Pins[] = {GPIO_PIN_5, GPIO_PIN_6, GPIO_PIN_12, GPIO_PIN_13};

char keys[4][4] = {
    {'1','2','3','A'},
    {'4','5','6','B'},
    {'7','8','9','C'},
    {'*','0','#','D'}
};

// =======================================
// 키패드 초기화 (필수는 아님, CubeMX에서 설정해도 됨)
// =======================================
void Keypad_Init(void) {
    // ROW 핀을 모두 HIGH로 초기화
    for(int i=0;i<4;i++) {
        HAL_GPIO_WritePin(ROW_Ports[i], ROW_Pins[i], GPIO_PIN_SET);
    }
}

// =======================================
// 키 읽기 함수
// =======================================
char Keypad_Read(void) {
    char key = 0;

    for(int row=0; row<4; row++) {
        // 모든 ROW를 HIGH로 초기화
        for(int i=0;i<4;i++)
            HAL_GPIO_WritePin(ROW_Ports[i], ROW_Pins[i], GPIO_PIN_SET);

        // 현재 ROW만 LOW로 내림
        HAL_GPIO_WritePin(ROW_Ports[row], ROW_Pins[row], GPIO_PIN_RESET);
        HAL_Delay(1); // 안정화 짧게 대기

        // 각 컬럼 검사
        for(int col=0; col<4; col++) {
            if(HAL_GPIO_ReadPin(COL_Ports[col], COL_Pins[col]) == GPIO_PIN_RESET) {
                // 디바운싱
                HAL_Delay(50);
                while(HAL_GPIO_ReadPin(COL_Ports[col], COL_Pins[col]) == GPIO_PIN_RESET);

                key = keys[row][col];
                return key;
            }
        }
    }

    return 0; // 눌린 키 없으면 0 리턴
}
