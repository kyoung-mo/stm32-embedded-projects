#include "ap.h"
#include "adc.h"
#include "dht11.h"
#include "i2c-lcd.h"
#include "keypad.h"
#include "rtc.h"
#include "stm32f4xx_hal_rtc.h"
#include "tim.h"
#include "usart.h"
#include "water_state_logger.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ========== 전역 변수 ==========
volatile uint16_t adc_values[4];
volatile SystemMode_t current_mode = MODE_PASSWORD_INPUT;
uint8_t is_logged_in = 0; // 0: 로그인 전, 1: 로그인 완료

// 비밀번호
char input_pw[5] = {0};
char target_pw[] = "1234";
uint8_t pw_idx = 0;

// 비밀번호 잠금
#define PW_MAX_FAIL 5
#define PW_LOCK_TIME 60000 // 60초 (1분)

uint8_t pw_fail_count = 0;
uint8_t pw_locked = 0;
uint32_t pw_lock_start_time = 0;

// 백그라운드
uint32_t last_dht11_time = 0;
uint32_t last_rtc_time = 0;
DHT11_Data_t global_dht11_data = {0};
RTC_TimeTypeDef global_time = {0};
RTC_DateTypeDef global_date = {0};
uint8_t dht11_valid = 0;

// 조이스틱
uint8_t joy_button_prev = 1;
uint16_t joy_center_x = 3130;
uint16_t joy_center_y = 3065;

// 메뉴 및 커서
uint8_t menu_selected = 0;
uint8_t cursor_pos = 0;
uint8_t scroll_offset = 0;

// Threshold 입력용
uint8_t threshold_input_mode = 0;
char threshold_input[3] = {0};
uint8_t threshold_input_idx = 0;

// 댐 제어 설정
uint8_t dam_auto_mode = 0;
uint8_t threshold_high = 40;
uint8_t threshold_low = 10;

// ⭐ 로그 시스템
WaterLog_t water_log;

// ⭐ HAL_Delay 대체용 타이머
uint32_t buzzer_off_time = 0;
uint32_t led_off_time = 0;
uint32_t message_clear_time = 0;
uint8_t auto_return_mode = 0; // 자동 복귀할 모드 (0=없음)

typedef enum {
  JOY_NONE = 0,
  JOY_UP,
  JOY_DOWN,
  JOY_LEFT,
  JOY_RIGHT
} JoyDirection_t;

// ========== 조이스틱 함수 ==========
uint8_t Is_Joy_Button_Clicked(void) {
  uint8_t current = HAL_GPIO_ReadPin(JOW_SW_GPIO_Port, JOW_SW_Pin);
  if (joy_button_prev == 1 && current == 0) {
    // ⭐ HAL_Delay 제거
    joy_button_prev = current;
    printf("[JOY] Button Clicked!\r\n");
    return 1;
  }
  joy_button_prev = current;
  return 0;
}

JoyDirection_t Get_Joy_Direction(void) {
  uint16_t vrx = adc_values[0];
  uint16_t vry = adc_values[2];

#define THRESHOLD_UP 2000
#define THRESHOLD_DOWN 3150
#define THRESHOLD_LEFT 2000
#define THRESHOLD_RIGHT 4000

  if (vry < THRESHOLD_UP)
    return JOY_UP;
  if (vry > THRESHOLD_DOWN)
    return JOY_DOWN;
  if (vrx < THRESHOLD_LEFT)
    return JOY_LEFT;
  if (vrx > THRESHOLD_RIGHT)
    return JOY_RIGHT;

  return JOY_NONE;
}

// ========== 서보모터 제어 ==========
void Servo_Set_Angle(uint8_t servo_num, uint8_t angle) {
  uint16_t pulse = 1000 + (angle * 1000 / 90);

  if (servo_num == 1) {
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, pulse);
    printf("[SERVO1] Angle: %d deg (Pulse: %u)\r\n", angle, pulse);
  } else if (servo_num == 2) {
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, pulse);
    printf("[SERVO2] Angle: %d deg (Pulse: %u)\r\n", angle, pulse);
  }
}

// ========== 자동 제어 로직 ==========
void Dam_Auto_Control(uint8_t water_level) {
  if (!dam_auto_mode)
    return;

  if (water_level < threshold_low) {
    Servo_Set_Angle(1, 90);
    Servo_Set_Angle(2, 0);
    printf("[AUTO] LOW: Servo1=90, Servo2=0\r\n");
  } else if (water_level > threshold_high) {
    Servo_Set_Angle(1, 0);
    Servo_Set_Angle(2, 90);
    printf("[AUTO] HIGH: Servo1=0, Servo2=90\r\n");
  } else {
    Servo_Set_Angle(1, 0);
    Servo_Set_Angle(2, 0);
    printf("[AUTO] NORMAL: Both closed\r\n");
  }
}

// ========== LCD 커서 표시 ==========
void LCD_Print_With_Cursor(uint8_t line, uint8_t is_selected,
                           const char *text) {
  LCD_SetCursor(line, 0);
  if (is_selected) {
    LCD_Print("[V] ");
  } else {
    LCD_Print("[ ] ");
  }
  LCD_Print((char *)text);
}

// ========== 초기화 ==========
void apInit(void) {
  printf("\r\n===========================================\r\n");
  printf("  STM32 Dam Management System v8\r\n");
  printf("  - Log System + Non-blocking\r\n");
  printf("===========================================\r\n");

  if (HAL_ADC_Start_DMA(&hadc1, (uint32_t *)adc_values, 4) != HAL_OK) {
    Error_Handler();
  }

  // ⭐ 로그 초기화
  WaterLog_Init(&water_log);

  // ⭐ 로그인 상태 초기화
  is_logged_in = 0;

  // ⭐ RGB LED 테스트 (간단히)
  printf("RGB LED Test...\r\n");
  HAL_GPIO_WritePin(RGB_R_GPIO_Port, RGB_R_Pin, GPIO_PIN_SET);
  for (volatile int i = 0; i < 100000; i++)
    ;
  HAL_GPIO_WritePin(RGB_R_GPIO_Port, RGB_R_Pin, GPIO_PIN_RESET);

  HAL_GPIO_WritePin(RGB_G_GPIO_Port, RGB_G_Pin, GPIO_PIN_SET);
  for (volatile int i = 0; i < 100000; i++)
    ;
  HAL_GPIO_WritePin(RGB_G_GPIO_Port, RGB_G_Pin, GPIO_PIN_RESET);

  HAL_GPIO_WritePin(RGB_B_GPIO_Port, RGB_B_Pin, GPIO_PIN_SET);
  for (volatile int i = 0; i < 100000; i++)
    ;
  HAL_GPIO_WritePin(RGB_B_GPIO_Port, RGB_B_Pin, GPIO_PIN_RESET);

  printf("RGB LED Test Done!\r\n");

  // ⭐ RGB LED 모두 끄기 (로그인 전)
  HAL_GPIO_WritePin(RGB_R_GPIO_Port, RGB_R_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(RGB_G_GPIO_Port, RGB_G_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(RGB_B_GPIO_Port, RGB_B_Pin, GPIO_PIN_RESET);

  HAL_TIM_Base_Start(&htim2);
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);

  Servo_Set_Angle(1, 0);
  Servo_Set_Angle(2, 0);

  LCD_Init();
  DHT11_Init();
  Keypad_Init();

  global_date.Year = 26;
  global_date.Month = 1;
  global_date.Date = 28;
  global_time.Hours = 10;
  global_time.Minutes = 0;
  global_time.Seconds = 0;
  HAL_RTC_SetTime(&hrtc, &global_time, RTC_FORMAT_BIN);
  HAL_RTC_SetDate(&hrtc, &global_date, RTC_FORMAT_BIN);

  // ⭐ 비밀번호 잠금 초기화
  pw_fail_count = 0;
  pw_locked = 0;
  pw_lock_start_time = 0;

  printf("System Ready!\r\n");

  LCD_Clear();
  LCD_SetCursor(0, 0);
  LCD_Print("Dam System Ready");
  LCD_SetCursor(1, 0);
  LCD_Print("Enter Password");

  message_clear_time = HAL_GetTick() + 2000;
  auto_return_mode = 1;
}

// ========== 백그라운드 ==========
void Update_Background_Tasks(void) {
  uint32_t now = HAL_GetTick();

  // DHT11 읽기
  if (now - last_dht11_time >= 2000) {
    last_dht11_time = now;
    dht11_valid = DHT11_Read(&global_dht11_data);
  }

  // RTC 업데이트
  if (now - last_rtc_time >= 1000) {
    last_rtc_time = now;
    HAL_RTC_GetTime(&hrtc, &global_time, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&hrtc, &global_date, RTC_FORMAT_BIN);
  }

  // ⭐ RGB LED 상시 동작 (로그인 후에만!)
  if (is_logged_in) {
    uint8_t water_level = (adc_values[1] * 100) / 4095;

    if (water_level < threshold_low) {
      // LOW: 빨강
      HAL_GPIO_WritePin(RGB_R_GPIO_Port, RGB_R_Pin, GPIO_PIN_SET);
      HAL_GPIO_WritePin(RGB_G_GPIO_Port, RGB_G_Pin, GPIO_PIN_RESET);
      HAL_GPIO_WritePin(RGB_B_GPIO_Port, RGB_B_Pin, GPIO_PIN_RESET);
    } else if (water_level > threshold_high) {
      // HIGH: 파랑
      HAL_GPIO_WritePin(RGB_R_GPIO_Port, RGB_R_Pin, GPIO_PIN_RESET);
      HAL_GPIO_WritePin(RGB_G_GPIO_Port, RGB_G_Pin, GPIO_PIN_RESET);
      HAL_GPIO_WritePin(RGB_B_GPIO_Port, RGB_B_Pin, GPIO_PIN_SET);
    } else {
      // OK: 녹색
      HAL_GPIO_WritePin(RGB_R_GPIO_Port, RGB_R_Pin, GPIO_PIN_RESET);
      HAL_GPIO_WritePin(RGB_G_GPIO_Port, RGB_G_Pin, GPIO_PIN_SET);
      HAL_GPIO_WritePin(RGB_B_GPIO_Port, RGB_B_Pin, GPIO_PIN_RESET);
    }
  } else {
    // ⭐ 로그인 전: RGB LED 모두 끄기
    HAL_GPIO_WritePin(RGB_R_GPIO_Port, RGB_R_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(RGB_G_GPIO_Port, RGB_G_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(RGB_B_GPIO_Port, RGB_B_Pin, GPIO_PIN_RESET);
  }

  // ⭐ 로그 업데이트 (로그인 후에만!)
  if (is_logged_in) {
    static uint32_t last_log_time = 0;
    if (now - last_log_time >= 1000) {
      last_log_time = now;
      uint8_t water_level = (adc_values[1] * 100) / 4095;
      WaterLog_Update(&water_log, water_level, threshold_low, threshold_high,
                      &global_time, &global_date);
    }
  }

  // 자동 모드 실행
  if (dam_auto_mode) {
    uint8_t water_level = (adc_values[1] * 100) / 4095;
    Dam_Auto_Control(water_level);
  }

  // ⭐ 부저 자동 OFF
  if (buzzer_off_time > 0 && now >= buzzer_off_time) {
    HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, GPIO_PIN_RESET);
    buzzer_off_time = 0;
  }

  // ⭐ LED 자동 OFF
  if (led_off_time > 0 && now >= led_off_time) {
    HAL_GPIO_WritePin(LED_RED_GPIO_Port, LED_RED_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin, GPIO_PIN_RESET);
    led_off_time = 0;
  }

  // ⭐ 메시지 자동 Clear
  if (message_clear_time > 0 && now >= message_clear_time) {
    message_clear_time = 0;
    if (auto_return_mode) {
      LCD_Clear();
      auto_return_mode = 0;
    }
  }
}

// ========== 메인 루프 ==========
void apMain(void) {
  char buffer[32];
  LCD_Clear();

  while (1) {
    Update_Background_Tasks();

    char key = Keypad_Read();
    if (key != 0) {
      printf("[KEY] %c\r\n", key);
    }

    switch (current_mode) {

    case MODE_PASSWORD_INPUT: {
      // ⭐ 잠금 상태 체크
      if (pw_locked) {
        uint32_t now = HAL_GetTick();
        if (now - pw_lock_start_time >= PW_LOCK_TIME) {
          // 잠금 해제
          pw_locked = 0;
          pw_fail_count = 0;

          LCD_SetCursor(0, 0);
          LCD_Print("Lock Released   ");
          LCD_SetCursor(1, 0);
          LCD_Print("Enter Password  ");

          message_clear_time = HAL_GetTick() + 1500;
          auto_return_mode = 1;

        } else {
          // 잠금 중
          uint32_t remain = (PW_LOCK_TIME - (now - pw_lock_start_time)) / 1000;

          LCD_SetCursor(0, 0);
          LCD_Print("LOCKED!         ");
          LCD_SetCursor(1, 0);
          sprintf(buffer, "Wait %2lus      ", remain);
          LCD_Print(buffer);
        }
        break;
      }

      // ⭐ 정상 비밀번호 입력 화면
      LCD_SetCursor(0, 0);
      LCD_Print("Enter Password: ");

      LCD_SetCursor(1, 0);
      for (int i = 0; i < 4; i++) {
        LCD_Print((i < pw_idx) ? "*" : "_");
      }
      LCD_Print("            ");

      if (key >= '0' && key <= '9' && pw_idx < 4) {
        input_pw[pw_idx++] = key;

      } else if (key == '*') {
        pw_idx = 0;
        memset(input_pw, 0, sizeof(input_pw));

      } else if ((key == '#' || Is_Joy_Button_Clicked()) && pw_idx == 4) {
        input_pw[4] = '\0';

        if (strcmp(input_pw, target_pw) == 0) {
          // ⭐ 성공
          pw_fail_count = 0;
          is_logged_in = 1; // ⭐ 로그인 상태 ON

          HAL_GPIO_WritePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin, GPIO_PIN_SET);
          led_off_time = HAL_GetTick() + 1000;

          LCD_SetCursor(1, 0);
          LCD_Print("CORRECT!        ");

          printf("[LOGIN] Success! RGB LED Enabled.\r\n"); // ⭐ 디버그

          current_mode = MODE_MENU_SELECT;
          menu_selected = 0;
          pw_idx = 0;
          memset(input_pw, 0, sizeof(input_pw));

          message_clear_time = HAL_GetTick() + 1000;
          auto_return_mode = 1;

        } else {
          // ⭐ 실패
          pw_fail_count++;
          printf("[PW] FAIL! Count=%d\r\n", pw_fail_count);

          HAL_GPIO_WritePin(LED_RED_GPIO_Port, LED_RED_Pin, GPIO_PIN_SET);
          HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, GPIO_PIN_SET);

          buzzer_off_time = HAL_GetTick() + 500;
          led_off_time = HAL_GetTick() + 1000;

          LCD_SetCursor(1, 0);

          if (pw_fail_count >= PW_MAX_FAIL) {
            pw_locked = 1;
            pw_lock_start_time = HAL_GetTick();

            LCD_Print("Too Many Fails! ");
            printf("[PW] SYSTEM LOCKED\r\n");

            message_clear_time = HAL_GetTick() + 1500;
            auto_return_mode = 1;

          } else {
            sprintf(buffer, "WRONG! (%d/%d)  ", pw_fail_count, PW_MAX_FAIL);
            LCD_Print(buffer);

            message_clear_time = HAL_GetTick() + 1000;
            auto_return_mode = 0;
          }

          pw_idx = 0;
          memset(input_pw, 0, sizeof(input_pw));
        }
      }

      // ⭐ 메시지 자동 복귀
      if (auto_return_mode && message_clear_time > 0 &&
          HAL_GetTick() >= message_clear_time) {
        LCD_SetCursor(1, 0);
        LCD_Print("____            ");
        message_clear_time = 0;
        auto_return_mode = 0;
      }
      break;
    }

    case MODE_MENU_SELECT: {
      const char *menu_items[] = {
          "1.Water Status ",  "2.Dam Control   ", "3.Threshold Set ",
          "4.Environment   ", "5.Clock         ", "6.Change PW     ",
          "7.Event Log     " // ⭐ 추가
      };

      static JoyDirection_t last_dir = JOY_NONE;
      static uint8_t menu_locked = 0;

      JoyDirection_t dir = Get_Joy_Direction();

      if (dir == JOY_NONE) {
        last_dir = JOY_NONE;
        menu_locked = 0;
      }

      if (dir != JOY_NONE && !menu_locked) {
        if (dir == JOY_UP && last_dir != JOY_UP) {
          menu_selected = (menu_selected + 1) % 7; // ⭐ 6 → 7
          LCD_Clear();
          menu_locked = 1;
          // ⭐ HAL_Delay 제거
        } else if (dir == JOY_DOWN && last_dir != JOY_DOWN) {
          menu_selected =
              (menu_selected == 0) ? 6 : menu_selected - 1; // ⭐ 5 → 6
          LCD_Clear();
          menu_locked = 1;
        }
        last_dir = dir;
      }

      LCD_SetCursor(0, 0);
      LCD_Print((char *)menu_items[menu_selected]);
      LCD_SetCursor(1, 0);
      LCD_Print("Click to Enter  ");

      if (Is_Joy_Button_Clicked()) {
        cursor_pos = 0;
        scroll_offset = 0;
        LCD_Clear();

        switch (menu_selected) {
        case 0:
          current_mode = MODE_WATER_STATUS;
          break;
        case 1:
          current_mode = MODE_DAM_CONTROL;
          break;
        case 2:
          current_mode = MODE_THRESHOLD_SET;
          break;
        case 3:
          current_mode = MODE_ENVIRONMENT;
          break;
        case 4:
          current_mode = MODE_CLOCK;
          break;
        case 5:
          current_mode = MODE_PW_CHANGE;
          pw_idx = 0;
          memset(input_pw, 0, sizeof(input_pw));
          break;
        case 6: // ⭐ 로그 화면
          current_mode = MODE_LOG;
          cursor_pos = 0;
          break;
        }
      }
      break;
    }

      // ============ 1. 수위 상태 ============
      // ============ 1. 수위 상태 ============
    case MODE_WATER_STATUS: {
      uint8_t water_level = (adc_values[1] * 100) / 4095;

      LCD_SetCursor(0, 0);
      sprintf(buffer, "Water:%3d%% %02d:%02d", water_level, global_time.Hours,
              global_time.Minutes);
      LCD_Print(buffer);

      LCD_SetCursor(1, 0);
      if (water_level < threshold_low) {
        LCD_Print("LOW   (Back)    ");
      } else if (water_level > threshold_high) {
        LCD_Print("HIGH  (Back)    ");
      } else {
        LCD_Print("OK    (Back)    ");
      }

      if (Is_Joy_Button_Clicked()) {
        current_mode = MODE_MENU_SELECT;
        LCD_Clear();
      }
      break;
    }

    // ============ 2. 댐 제어 모드 선택 ============
    case MODE_DAM_CONTROL: {
      const char *items[] = {
          "Passive Mode ", // ⭐ 수정: 13칸 (커서 [V] 제외)
          "Active Mode  ", // ⭐ 수정: 12칸
          "Back         "  // ⭐ 수정: 4칸 + 8공백
      };
      const uint8_t ITEM_COUNT = 3;

      static JoyDirection_t last_dir_dam = JOY_NONE;
      JoyDirection_t dir = Get_Joy_Direction();

      if (dir == JOY_NONE) {
        last_dir_dam = JOY_NONE;
      }

      // ⭐ X축으로 커서 이동 (LEFT/RIGHT)
      if (dir == JOY_RIGHT && last_dir_dam != JOY_RIGHT &&
          cursor_pos < ITEM_COUNT - 1) {
        cursor_pos++;
        if (cursor_pos >= scroll_offset + 2) {
          scroll_offset = cursor_pos - 1;
        }
        LCD_Clear();
        last_dir_dam = JOY_RIGHT;
        HAL_Delay(200);
      } else if (dir == JOY_LEFT && last_dir_dam != JOY_LEFT &&
                 cursor_pos > 0) {
        cursor_pos--;
        if (cursor_pos < scroll_offset) {
          scroll_offset = cursor_pos;
        }
        LCD_Clear();
        last_dir_dam = JOY_LEFT;
        HAL_Delay(200);
      }

      // 2줄 표시
      for (uint8_t i = 0; i < 2 && (scroll_offset + i) < ITEM_COUNT; i++) {
        uint8_t item_idx = scroll_offset + i;
        LCD_Print_With_Cursor(i, (item_idx == cursor_pos), items[item_idx]);
      }

      if (Is_Joy_Button_Clicked()) {
        if (cursor_pos == 0) {
          current_mode = MODE_DAM_MANUAL;
          cursor_pos = 0;
          scroll_offset = 0;
        } else if (cursor_pos == 1) {
          current_mode = MODE_DAM_AUTO;
          cursor_pos = 0;
          scroll_offset = 0;
        } else {
          current_mode = MODE_MENU_SELECT;
        }
        LCD_Clear();
      }
      break;
    }

    // ============ 2-1. 수동 제어 ============
    case MODE_DAM_MANUAL: {
      const char *items[] = {
          "Servo1 Ctrl ", // ⭐ 수정: 12칸
          "Servo2 Ctrl ", // ⭐ 수정: 12칸
          "Back        "  // ⭐ 수정: 4칸 + 8공백
      };
      const uint8_t ITEM_COUNT = 3;

      static JoyDirection_t last_dir_manual = JOY_NONE;
      JoyDirection_t dir = Get_Joy_Direction();

      if (dir == JOY_NONE)
        last_dir_manual = JOY_NONE;

      // ⭐ X축 이동 (LEFT/RIGHT)
      if (dir == JOY_RIGHT && last_dir_manual != JOY_RIGHT &&
          cursor_pos < ITEM_COUNT - 1) {
        cursor_pos++;
        if (cursor_pos >= scroll_offset + 2) {
          scroll_offset = cursor_pos - 1;
        }
        LCD_Clear();
        last_dir_manual = JOY_RIGHT;
        HAL_Delay(200);
      } else if (dir == JOY_LEFT && last_dir_manual != JOY_LEFT &&
                 cursor_pos > 0) {
        cursor_pos--;
        if (cursor_pos < scroll_offset) {
          scroll_offset = cursor_pos;
        }
        LCD_Clear();
        last_dir_manual = JOY_LEFT;
        HAL_Delay(200);
      }

      for (uint8_t i = 0; i < 2 && (scroll_offset + i) < ITEM_COUNT; i++) {
        uint8_t item_idx = scroll_offset + i;
        LCD_Print_With_Cursor(i, (item_idx == cursor_pos), items[item_idx]);
      }

      if (Is_Joy_Button_Clicked()) {
        if (cursor_pos == 0) {
          static uint8_t servo1_state = 0;
          servo1_state = !servo1_state;
          Servo_Set_Angle(1, servo1_state ? 90 : 0);
        } else if (cursor_pos == 1) {
          static uint8_t servo2_state = 0;
          servo2_state = !servo2_state;
          Servo_Set_Angle(2, servo2_state ? 90 : 0);
        } else {
          current_mode = MODE_DAM_CONTROL;
          cursor_pos = 0;
          scroll_offset = 0;
          LCD_Clear();
        }
      }
      break;
    }

    // ============ 2-2. 자동 제어 ============
    case MODE_DAM_AUTO: {
      char status_str[16];
      sprintf(status_str, "Now: %s     ",
              dam_auto_mode ? "ON " : "OFF"); // ⭐ 수정: 16칸

      const char *items[] = {dam_auto_mode ? "Turn OFF    "
                                           : "Turn ON     ", // ⭐ 수정: 12칸
                             status_str, "Back        "};
      const uint8_t ITEM_COUNT = 3;

      static JoyDirection_t last_dir_auto = JOY_NONE;
      JoyDirection_t dir = Get_Joy_Direction();

      if (dir == JOY_NONE)
        last_dir_auto = JOY_NONE;

      // ⭐ X축 이동
      if (dir == JOY_RIGHT && last_dir_auto != JOY_RIGHT &&
          cursor_pos < ITEM_COUNT - 1) {
        cursor_pos++;
        if (cursor_pos >= scroll_offset + 2) {
          scroll_offset = cursor_pos - 1;
        }
        LCD_Clear();
        last_dir_auto = JOY_RIGHT;
        HAL_Delay(200);
      } else if (dir == JOY_LEFT && last_dir_auto != JOY_LEFT &&
                 cursor_pos > 0) {
        cursor_pos--;
        if (cursor_pos < scroll_offset) {
          scroll_offset = cursor_pos;
        }
        LCD_Clear();
        last_dir_auto = JOY_LEFT;
        HAL_Delay(200);
      }

      for (uint8_t i = 0; i < 2 && (scroll_offset + i) < ITEM_COUNT; i++) {
        uint8_t item_idx = scroll_offset + i;
        LCD_Print_With_Cursor(i, (item_idx == cursor_pos), items[item_idx]);
      }

      if (Is_Joy_Button_Clicked()) {
        if (cursor_pos == 0) {
          dam_auto_mode = !dam_auto_mode;
          printf("[AUTO] Mode: %s\r\n", dam_auto_mode ? "ON" : "OFF");
          LCD_Clear();
        } else if (cursor_pos == 2) {
          current_mode = MODE_DAM_CONTROL;
          cursor_pos = 0;
          scroll_offset = 0;
          LCD_Clear();
        }
      }
      break;
    }

    // ============ 3. 기준치 변경 ============
    case MODE_THRESHOLD_SET: {
      char item0[16], item1[16];
      sprintf(item0, "High:%2d%%   ", threshold_high);
      sprintf(item1, "Low: %2d%%   ", threshold_low);

      const char *items[] = {item0, item1, "Back        "};
      const uint8_t ITEM_COUNT = 3;

      static JoyDirection_t last_dir_th = JOY_NONE;
      JoyDirection_t dir = Get_Joy_Direction();

      if (dir == JOY_NONE)
        last_dir_th = JOY_NONE;

      if (dir == JOY_RIGHT && last_dir_th != JOY_RIGHT &&
          cursor_pos < ITEM_COUNT - 1) {
        cursor_pos++;
        if (cursor_pos >= scroll_offset + 2) {
          scroll_offset = cursor_pos - 1;
        }
        LCD_Clear();
        last_dir_th = JOY_RIGHT;
        HAL_Delay(200);
      } else if (dir == JOY_LEFT && last_dir_th != JOY_LEFT && cursor_pos > 0) {
        cursor_pos--;
        if (cursor_pos < scroll_offset) {
          scroll_offset = cursor_pos;
        }
        LCD_Clear();
        last_dir_th = JOY_LEFT;
        HAL_Delay(200);
      }

      for (uint8_t i = 0; i < 2 && (scroll_offset + i) < ITEM_COUNT; i++) {
        uint8_t item_idx = scroll_offset + i;
        LCD_Print_With_Cursor(i, (item_idx == cursor_pos), items[item_idx]);
      }

      if (Is_Joy_Button_Clicked()) {
        if (cursor_pos == 0) {
          // High 기준치 변경 ⭐
          threshold_input_mode = 0; // High 모드
          threshold_input_idx = 0;
          memset(threshold_input, 0, sizeof(threshold_input));
          current_mode = MODE_THRESHOLD_INPUT;
          LCD_Clear();
          printf("[THRESHOLD] Entering High input mode\r\n");
        } else if (cursor_pos == 1) {
          // Low 기준치 변경 ⭐
          threshold_input_mode = 1; // Low 모드
          threshold_input_idx = 0;
          memset(threshold_input, 0, sizeof(threshold_input));
          current_mode = MODE_THRESHOLD_INPUT;
          LCD_Clear();
          printf("[THRESHOLD] Entering Low input mode\r\n");
        } else {
          // Back
          current_mode = MODE_MENU_SELECT;
          cursor_pos = 0;
          scroll_offset = 0;
          LCD_Clear();
        }
      }
      break;
    }

    // ============ 3-1. 기준치 입력 ============
    case MODE_THRESHOLD_INPUT: {
      // 화면 표시
      LCD_SetCursor(0, 0);
      if (threshold_input_mode == 0) {
        sprintf(buffer, "Set High (0-50) ");
      } else {
        sprintf(buffer, "Set Low  (0-50) ");
      }
      LCD_Print(buffer);

      // ⭐ 현재 입력 상태를 buffer에 한번에 구성
      LCD_SetCursor(1, 0);
      char display_line[17]; // 16칸 + NULL

      // 입력된 숫자들
      if (threshold_input_idx == 0) {
        sprintf(display_line, "__ (#:OK *:Back)"); // 16칸
      } else if (threshold_input_idx == 1) {
        sprintf(display_line, "%c_ (#:OK *:Back)", threshold_input[0]); // 16칸
      } else {
        sprintf(display_line, "%c%c (#:OK *:Back)", threshold_input[0],
                threshold_input[1]); // 16칸
      }

      LCD_Print(display_line);

      // 숫자 키 입력 (0~9)
      if (key >= '0' && key <= '9') {
        if (threshold_input_idx < 2) {
          threshold_input[threshold_input_idx] = key;
          threshold_input_idx++;
          printf("[THRESHOLD] Input: %c (idx=%d)\r\n", key,
                 threshold_input_idx);
        }
      }
      // '*' 누르면 취소하고 돌아가기
      else if (key == '*') {
        threshold_input_idx = 0;
        memset(threshold_input, 0, sizeof(threshold_input));
        current_mode = MODE_THRESHOLD_SET;
        cursor_pos = threshold_input_mode;
        printf("[THRESHOLD] Cancelled\r\n");
        LCD_Clear();
      }
      // '#' 누르면 값 저장
      else if (key == '#' && threshold_input_idx > 0) {
        threshold_input[threshold_input_idx] = '\0';
        int new_value = atoi(threshold_input);

        printf("[THRESHOLD] Input value: %d\r\n", new_value);

        // 유효성 검사
        if (new_value < 0 || new_value > 50) {
          // 범위 초과
          LCD_Clear();
          LCD_SetCursor(0, 0);
          LCD_Print("Error: 0-50 Only");
          LCD_SetCursor(1, 0);
          LCD_Print("Try Again       ");
          HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, GPIO_PIN_SET);
          HAL_Delay(300);
          HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, GPIO_PIN_RESET);
          HAL_Delay(1200);

          threshold_input_idx = 0;
          memset(threshold_input, 0, sizeof(threshold_input));
          LCD_Clear();

        } else if (threshold_input_mode == 0) {
          // High 값 설정
          if (new_value <= threshold_low) {
            LCD_Clear();
            LCD_SetCursor(0, 0);
            sprintf(buffer, "High > Low(%d%%) ", threshold_low);
            LCD_Print(buffer);
            LCD_SetCursor(1, 0);
            LCD_Print("Try Again       ");
            HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, GPIO_PIN_SET);
            HAL_Delay(300);
            HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, GPIO_PIN_RESET);
            HAL_Delay(1200);

            threshold_input_idx = 0;
            memset(threshold_input, 0, sizeof(threshold_input));
            LCD_Clear();

          } else {
            // 성공
            threshold_high = new_value;
            printf("[THRESHOLD] High set to: %d\r\n", threshold_high);

            LCD_Clear();
            LCD_SetCursor(0, 0);
            sprintf(buffer, "High Set: %d%%  ", threshold_high);
            LCD_Print(buffer);
            LCD_SetCursor(1, 0);
            LCD_Print("Success!        ");
            HAL_Delay(1500);

            current_mode = MODE_THRESHOLD_SET;
            threshold_input_idx = 0;
            memset(threshold_input, 0, sizeof(threshold_input));
            cursor_pos = 0;
            scroll_offset = 0;
            LCD_Clear();
          }

        } else {
          // Low 값 설정
          if (new_value >= threshold_high) {
            LCD_Clear();
            LCD_SetCursor(0, 0);
            sprintf(buffer, "Low < High(%d%%)", threshold_high);
            LCD_Print(buffer);
            LCD_SetCursor(1, 0);
            LCD_Print("Try Again       ");
            HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, GPIO_PIN_SET);
            HAL_Delay(300);
            HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, GPIO_PIN_RESET);
            HAL_Delay(1200);

            threshold_input_idx = 0;
            memset(threshold_input, 0, sizeof(threshold_input));
            LCD_Clear();

          } else {
            // 성공
            threshold_low = new_value;
            printf("[THRESHOLD] Low set to: %d\r\n", threshold_low);

            LCD_Clear();
            LCD_SetCursor(0, 0);
            sprintf(buffer, "Low Set: %d%%   ", threshold_low);
            LCD_Print(buffer);
            LCD_SetCursor(1, 0);
            LCD_Print("Success!        ");
            HAL_Delay(1500);

            current_mode = MODE_THRESHOLD_SET;
            threshold_input_idx = 0;
            memset(threshold_input, 0, sizeof(threshold_input));
            cursor_pos = 1;
            scroll_offset = 0;
            LCD_Clear();
          }
        }
      }
      break;
    }

      // ============ 4. 환경 정보 ============
    case MODE_ENVIRONMENT: {
      LCD_SetCursor(0, 0);
      if (dht11_valid) {
        sprintf(buffer, "T:%2dC H:%2d%%    ", // ⭐ 수정: 16칸
                global_dht11_data.temperature, global_dht11_data.humidity);
        LCD_Print(buffer); // "T:24C H:55%    " = 16칸
      } else {
        LCD_Print("Sensor Error!   "); // 16칸 ✅
      }

      // 시스템 내부 온도
      uint16_t temp_adc = adc_values[3];
      float vsense = (temp_adc * 3.3f) / 4095.0f;
      float sys_temp = ((vsense - 0.76f) / 0.0025f) + 25.0f;

      LCD_SetCursor(1, 0);
      sprintf(buffer, "Sys:%.1fC (Back)", sys_temp); // 16칸 ✅
      LCD_Print(buffer);

      // 온도 경고 (30도 이상)
      if (dht11_valid && global_dht11_data.temperature >= 30) {
        HAL_GPIO_WritePin(LED_RED_GPIO_Port, LED_RED_Pin, GPIO_PIN_SET);
      } else {
        HAL_GPIO_WritePin(LED_RED_GPIO_Port, LED_RED_Pin, GPIO_PIN_RESET);
      }

      if (Is_Joy_Button_Clicked()) {
        current_mode = MODE_MENU_SELECT;
        HAL_GPIO_WritePin(LED_RED_GPIO_Port, LED_RED_Pin, GPIO_PIN_RESET);
        LCD_Clear();
      }
      break;
    }

    // ============ 5. 시계 ============
    case MODE_CLOCK:
      LCD_SetCursor(0, 0);
      sprintf(buffer, "20%02d-%02d-%02d    ", // ⭐ 수정: 16칸
              global_date.Year, global_date.Month, global_date.Date);
      LCD_Print(buffer); // "2026-01-27    " = 16칸

      LCD_SetCursor(1, 0);
      sprintf(buffer, "%02d:%02d:%02d (Back)", // 16칸 ✅
              global_time.Hours, global_time.Minutes, global_time.Seconds);
      LCD_Print(buffer); // "14:30:25 (Back)" = 16칸

      if (Is_Joy_Button_Clicked()) {
        current_mode = MODE_MENU_SELECT;
        LCD_Clear();
      }
      break;
    // ============ 6. 비밀번호 변경 ============
    case MODE_PW_CHANGE: {
      LCD_SetCursor(0, 0);
      LCD_Print("New Password:   ");

      LCD_SetCursor(1, 0);
      for (int i = 0; i < 4; i++) {
        if (i < pw_idx) {
          LCD_Print("*");
        } else {
          LCD_Print("_");
        }
      }
      LCD_Print(" (#:OK)     ");

      if (key >= '0' && key <= '9') {
        if (pw_idx < 4) {
          input_pw[pw_idx] = key;
          pw_idx++;
          printf("[PW-CHG] Input: %c (idx=%d)\r\n", key, pw_idx);
        }
      } else if (key == '*') {
        pw_idx = 0;
        memset(input_pw, 0, sizeof(input_pw));
        current_mode = MODE_MENU_SELECT;
        printf("[PW-CHG] Cancelled\r\n");
        LCD_Clear();
      } else if (key == '#' && pw_idx == 4) {
        input_pw[4] = '\0';

        strcpy(target_pw, input_pw);
        printf("[PW-CHG] New: %s\r\n", target_pw);

        LCD_Clear();
        LCD_SetCursor(0, 0);
        LCD_Print("PW CHANGED!     ");
        LCD_SetCursor(1, 0);
        LCD_Print("Re-login Please ");

        HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, GPIO_PIN_SET);
        buzzer_off_time = HAL_GetTick() + 300;

        message_clear_time = HAL_GetTick() + 1500;

        // ⭐ 로그아웃 처리
        is_logged_in = 0;
        current_mode = MODE_PASSWORD_INPUT;
        pw_idx = 0;
        memset(input_pw, 0, sizeof(input_pw));

        printf("[LOGOUT] RGB LED Disabled.\r\n"); // ⭐ 디버그
      }
      break;
    }

    // ============ 7. 로그 화면 ============
    case MODE_LOG: {
      uint8_t log_count = WaterLog_Count(&water_log);

      uint8_t total_lines = 1 + (log_count * 2);

      static JoyDirection_t last_dir_log = JOY_NONE;
      JoyDirection_t dir = Get_Joy_Direction();

      if (dir == JOY_NONE) {
        last_dir_log = JOY_NONE;
      }

      // X축으로 이동 (2줄씩 건너뛰기)
      if (dir == JOY_RIGHT && last_dir_log != JOY_RIGHT) {
        if (cursor_pos == 0 && log_count > 0) {
          cursor_pos = 1;
          scroll_offset = 0;
        } else if (cursor_pos > 0) {
          cursor_pos += 2;
          if (cursor_pos >= total_lines) {
            cursor_pos = total_lines - 2;
          }
          if (cursor_pos >= scroll_offset + 2) {
            scroll_offset = cursor_pos;
          }
        }
        LCD_Clear();
        last_dir_log = JOY_RIGHT;

      } else if (dir == JOY_LEFT && last_dir_log != JOY_LEFT) {
        if (cursor_pos > 1) {
          cursor_pos -= 2;
          if (cursor_pos < 1)
            cursor_pos = 1;
          if (cursor_pos < scroll_offset) {
            scroll_offset = cursor_pos - 1;
            if (scroll_offset < 0)
              scroll_offset = 0;
          }
        } else {
          cursor_pos = 0;
          scroll_offset = 0;
        }
        LCD_Clear();
        last_dir_log = JOY_LEFT;
      }

      // ⭐ 2줄 표시
      if (cursor_pos == 0) {
        // ⭐ Back 화면
        LCD_SetCursor(0, 0);
        LCD_Print("[V] Back        "); // 체크박스 유지

        LCD_SetCursor(1, 0);
        if (log_count > 0) {
          sprintf(buffer, "Logs:%2d/10     ", log_count); // ⭐ 체크박스 제거
          LCD_Print(buffer);
        } else {
          LCD_Print("No Logs         "); // ⭐ 체크박스 제거
        }

      } else {
        // ⭐ 로그 표시 (체크박스 완전 제거)
        uint8_t log_idx = (cursor_pos - 1) / 2;
        const WaterEvent_t *ev = WaterLog_GetByViewIndex(&water_log, log_idx);

        if (ev) {
          char log_line[17];
          const char *state_str = (ev->state == WATER_ST_LOW) ? "L" : "H";

          // ⭐ 첫 줄: 번호 + 상태 + 시작 시간 (체크박스 없음)
          LCD_SetCursor(0, 0);
          sprintf(log_line, "%d/%d %s %02d:%02d:%02d   ", log_idx + 1,
                  log_count, state_str, ev->t_start.Hours, ev->t_start.Minutes,
                  ev->t_start.Seconds);
          LCD_Print(log_line);

          // ⭐ 둘째 줄: 종료 시간 (체크박스 없음)
          LCD_SetCursor(1, 0);
          if (ev->ended) {
            sprintf(log_line, "    ~ %02d:%02d:%02d      ", ev->t_end.Hours,
                    ev->t_end.Minutes, ev->t_end.Seconds);
          } else {
            sprintf(log_line, "(ongoing)       ");
          }
          LCD_Print(log_line);
        }
      }

      // Back 선택 시 돌아가기
      if (Is_Joy_Button_Clicked() && cursor_pos == 0) {
        current_mode = MODE_MENU_SELECT;
        cursor_pos = 0;
        scroll_offset = 0;
        LCD_Clear();
      }
      break;
    }
    default:
      break;
    }

    // ⭐ 10ms 대신 짧게
    for (volatile int i = 0; i < 1000; i++)
      ;
  }
}