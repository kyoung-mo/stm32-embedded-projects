#ifndef AP_H_
#define AP_H_

#include "main.h"
#include "adc.h"
#include "dma.h"
#include "usart.h"
#include "gpio.h"
#include "i2c.h"
#include "ap_def.h"
#include "i2c-lcd.h"

typedef enum {
    MODE_CLOCK = 0,    // 시계 모드
    MODE_TEMPERATURE   // 온도 모드
} DisplayMode_t;

void apInit(void);
void apMain(void);

void Button_Pressed_Callback(void);

#endif