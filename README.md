# stm32-study

STM32F411RE (Nucleo-F411RE) 기반 주변장치 학습 및 미니 프로젝트 모음입니다.

## 📁 목록

### 주변장치 실습
- **LED**: GPIO를 이용한 기본 LED 제어
- **IButton**: GPIO Interrupt 기반 버튼 이벤트 처리
- **ADC**: 아날로그 센서 값 읽기
- **DMAtest**: DMA를 활용한 고속 데이터 전송
- **UART / InterruptUART**: PC와 시리얼 통신 (폴링 / 인터럽트)
- **LCD_I2C**: I2C 통신을 이용한 16x2 LCD 제어

### 미니 프로젝트
- **STM32-Dam-Control-Clean** ⭐: 댐 제어 시스템
  - 수위 센서 모니터링 / 서보 모터 수문 제어
  - 조이스틱 수동 조작 / RGB LED 상태 표시
  - I2C LCD 디스플레이 / 비밀번호 인증

## 🛠️ 개발 환경

- **MCU**: STM32F411RE (Nucleo-F411RE)
- **IDE**: STM32CubeIDE / HAL Driver

## 📋 학습 내용

- ✅ GPIO, ADC, PWM, UART, I2C, DMA
- ✅ External / UART / Timer Interrupt
- ✅ 상태 머신 (State Machine)
- ✅ 센서 통합 (DHT11, Water Level, Joystick)

## 📁 프로젝트 구조

```
ProjectName/
├── Core/
│   ├── Inc/          # 헤더 파일
│   └── Src/
│       └── main.c
├── App/              # 모듈화된 애플리케이션 로직 (해당 프로젝트만)
└── *.ioc             # STM32CubeMX 설정 파일
```
