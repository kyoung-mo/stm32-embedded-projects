#ifndef AP_H_
#define AP_H_

#include "adc.h"
#include "dht11.h"
#include "i2c-lcd.h"
#include "keypad.h"
#include "main.h"
#include "rtc.h"
#include "tim.h"
#include "usart.h"
#include "water_state_logger.h"

// 시스템 모드 정의
typedef enum {
  MODE_PASSWORD_INPUT,
  MODE_MENU_SELECT,
  MODE_WATER_STATUS,
  MODE_DAM_CONTROL,
  MODE_DAM_MANUAL,
  MODE_DAM_AUTO,
  MODE_THRESHOLD_SET,
  MODE_THRESHOLD_INPUT,
  MODE_ENVIRONMENT,
  MODE_CLOCK,
  MODE_PW_CHANGE,
  MODE_LOG              // ⭐ 7. 로그 기록
} SystemMode_t;

void apInit(void);
void apMain(void);

#endif // AP_H_