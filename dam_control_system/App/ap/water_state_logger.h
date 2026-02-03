#ifndef WATER_STATE_LOGGER_H_
#define WATER_STATE_LOGGER_H_

#include "rtc.h"
#include <stdint.h>

typedef enum { WATER_ST_OK = 0, WATER_ST_LOW, WATER_ST_HIGH } WaterState_t;

typedef struct {
  WaterState_t state;      // LOW / HIGH
  uint8_t level_percent;   // 이벤트 순간 수위
  RTC_TimeTypeDef t_start; // 시작 시간
  RTC_DateTypeDef d_start; // 시작 날짜
  RTC_TimeTypeDef t_end;   // 종료 시간 (OK 복귀)
  RTC_DateTypeDef d_end;   // 종료 날짜
  uint8_t ended;           // 종료 여부 (0: 진행중, 1: 종료됨)
} WaterEvent_t;

#define WATERLOG_MAX 10

typedef struct {
  WaterEvent_t ev[WATERLOG_MAX];
  uint8_t head;
  uint8_t count;
  uint8_t view;
  WaterState_t live_state;
} WaterLog_t;

void WaterLog_Init(WaterLog_t *log);

void WaterLog_Update(WaterLog_t *log, uint8_t level_percent, uint8_t th_low,
                     uint8_t th_high, const RTC_TimeTypeDef *now_t,
                     const RTC_DateTypeDef *now_d);

uint8_t WaterLog_Count(const WaterLog_t *log);

const WaterEvent_t *WaterLog_GetByViewIndex(const WaterLog_t *log,
                                            uint8_t view_index);

#endif