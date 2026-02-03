#include "water_state_logger.h"
#include <string.h>

static WaterState_t calc_state(uint8_t level, uint8_t th_low, uint8_t th_high) {
  if (level < th_low)
    return WATER_ST_LOW;
  if (level > th_high)
    return WATER_ST_HIGH;
  return WATER_ST_OK;
}

static void push_event(WaterLog_t *log, WaterState_t st, uint8_t level,
                       const RTC_TimeTypeDef *t, const RTC_DateTypeDef *d) {
  WaterEvent_t *dst = &log->ev[log->head];
  dst->state = st;
  dst->level_percent = level;
  dst->t_start = *t; // 시작 시간
  dst->d_start = *d; // 시작 날짜
  dst->ended = 0;    // 아직 종료 안 됨

  log->head = (log->head + 1) % WATERLOG_MAX;
  if (log->count < WATERLOG_MAX)
    log->count++;
}

// ⭐ 가장 최근 이벤트에 종료 시간 기록
static void end_current_event(WaterLog_t *log, const RTC_TimeTypeDef *t,
                              const RTC_DateTypeDef *d) {
  if (log->count == 0)
    return; // 로그 없음

  // 가장 최근 이벤트 찾기
  int last_idx = (log->head - 1);
  if (last_idx < 0)
    last_idx += WATERLOG_MAX;

  WaterEvent_t *last = &log->ev[last_idx];

  // 이미 종료된 이벤트면 무시
  if (last->ended)
    return;

  // 종료 시간 기록
  last->t_end = *t;
  last->d_end = *d;
  last->ended = 1;
}

void WaterLog_Init(WaterLog_t *log) {
  memset(log, 0, sizeof(*log));
  log->live_state = WATER_ST_OK;
}

void WaterLog_Update(WaterLog_t *log, uint8_t level_percent, uint8_t th_low,
                     uint8_t th_high, const RTC_TimeTypeDef *now_t,
                     const RTC_DateTypeDef *now_d) {
  WaterState_t new_state = calc_state(level_percent, th_low, th_high);

  // 상태 변화가 없으면 아무 것도 안 함
  if (new_state == log->live_state) {
    return;
  }

  // OK로 복귀하는 경우 → 이전 이벤트 종료
  if (new_state == WATER_ST_OK) {
    end_current_event(log, now_t, now_d);
    log->live_state = WATER_ST_OK;
    return;
  }

  // LOW/HIGH로 진입하는 경우 → 새 이벤트 시작
  log->live_state = new_state;
  push_event(log, new_state, level_percent, now_t, now_d);
}

uint8_t WaterLog_Count(const WaterLog_t *log) { return log->count; }

const WaterEvent_t *WaterLog_GetByViewIndex(const WaterLog_t *log,
                                            uint8_t view_index) {
  if (log->count == 0)
    return NULL;
  if (view_index >= log->count)
    return NULL;

  int oldest = (int)log->head - (int)log->count;
  while (oldest < 0)
    oldest += WATERLOG_MAX;

  int idx = oldest + view_index;
  idx %= WATERLOG_MAX;

  return &log->ev[idx];
}