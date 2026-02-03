# STM32 Embedded Projects

STM32F411RE (Nucleo-F411RE) 기반 임베디드 시스템 프로젝트 모음

## 🎯 프로젝트 개요

이 레포지토리는 STM32 마이크로컨트롤러를 사용한 다양한 임베디드 시스템 프로젝트를 포함합니다.  
각 프로젝트는 특정 주변장치(Peripheral)나 통신 프로토콜을 학습하고 실습하기 위해 구현되었습니다.

## 📁 프로젝트 목록

### 1. ADC (Analog-Digital Converter)
- **설명**: 아날로그 신호를 디지털로 변환
- **주요 기능**: ADC 센서 값 읽기
- **파일**: `ADC/Core/Src/main.c`

### 2. DMAtest (Direct Memory Access)
- **설명**: DMA를 사용한 고속 데이터 전송
- **주요 기능**: CPU 개입 없이 메모리 간 데이터 이동
- **파일**: `DMAtest/Core/Src/main.c`

### 3. IButton
- **설명**: 버튼 입력 처리 (Interrupt 방식)
- **주요 기능**: GPIO Interrupt를 이용한 버튼 이벤트 처리
- **파일**: `IButton/Core/Src/main.c`

### 4. InterruptUART
- **설명**: Interrupt 기반 UART 통신
- **주요 기능**: 비동기 시리얼 통신
- **파일**: `InterruptUART/Core/Src/main.c`

### 5. LCD_I2C
- **설명**: I2C 통신을 이용한 LCD 제어
- **주요 기능**: 16x2 LCD 디스플레이 문자 출력
- **파일**: `LCD_I2C/Core/Src/main.c`, `LCD_I2C/App/`

### 6. LED
- **설명**: 기본 LED 제어
- **주요 기능**: GPIO를 이용한 LED 깜빡임
- **파일**: `LED/Core/Src/main.c`

### 7. UART / UART_COMMUNICATION
- **설명**: 기본 UART 시리얼 통신
- **주요 기능**: PC와 시리얼 통신, 데이터 송수신
- **파일**: `UART/Core/Src/main.c`

### 8. STM32-Dam-Control-Clean ⭐
- **설명**: **댐 제어 시스템 (최종 프로젝트)**
- **주요 기능**:
  - 수위 센서 모니터링
  - 서보 모터 제어 (수문 개폐)
  - LCD 디스플레이
  - 조이스틱 입력
  - RGB LED 상태 표시
  - 비밀번호 인증
- **파일**: `STM32-Dam-Control-Clean/App/`, `STM32-Dam-Control-Clean/Core/Src/main.c`

### 9. dam_control_system
- **설명**: 댐 제어 시스템 (개발 버전)
- **파일**: `dam_control_system/App/`

## 🛠️ 개발 환경

- **MCU**: STM32F411RE (Nucleo-F411RE)
- **IDE**: STM32CubeIDE
- **HAL**: STM32 HAL Driver
- **주변장치**:
  - DHT11 (온습도 센서)
  - Water Level Sensor (수위 센서)
  - Servo Motor (MG996R)
  - RGB LED
  - I2C LCD (16x2)
  - Joystick
  - Buzzer

## 🔧 빌드 및 실행

### STM32CubeIDE 사용
```
1. File → Import → Existing Projects into Workspace
2. 프로젝트 폴더 선택
3. Build Project (Ctrl+B)
4. Run (F11) 또는 Debug
```

### 프로젝트 구조
```
ProjectName/
├── Core/
│   ├── Inc/          # 헤더 파일
│   └── Src/
│       └── main.c    # 메인 소스 코드
├── App/              # 애플리케이션 로직 (있는 경우)
│   ├── ap/
│   └── hw/
└── *.ioc             # STM32CubeMX 설정 파일
```

## 📋 주요 학습 내용

### 주변장치 제어
- ✅ GPIO (Digital I/O)
- ✅ ADC (Analog Input)
- ✅ PWM (Servo Motor)
- ✅ UART (Serial Communication)
- ✅ I2C (LCD Communication)
- ✅ DMA (Direct Memory Access)

### 인터럽트 처리
- ✅ External Interrupt (Button)
- ✅ UART Interrupt
- ✅ Timer Interrupt

### 센서 통합
- ✅ DHT11 (온습도)
- ✅ Water Level Sensor
- ✅ Joystick (Analog)

### 시스템 통합
- ✅ 상태 머신 (State Machine)
- ✅ 비밀번호 인증
- ✅ 멀티태스킹 로직

## 🎓 프로젝트 하이라이트: Dam Control System

### 시스템 구성도
```
[DHT11] ─┐
[Water] ─┼─> [STM32F411RE] ─┬─> [Servo Motor]
[Joy]   ─┤                   ├─> [RGB LED]
[Btn]   ─┘                   └─> [LCD Display]
```

### 주요 기능
1. **센서 모니터링**: 수위, 온도, 습도 실시간 측정
2. **자동 제어**: 수위에 따른 자동 수문 개폐
3. **수동 제어**: 조이스틱을 이용한 수동 조작
4. **보안**: 4자리 비밀번호 인증 시스템
5. **상태 표시**: RGB LED + LCD 화면

## 🔗 관련 링크

- [STM32F411RE Datasheet](https://www.st.com/en/microcontrollers-microprocessors/stm32f411re.html)
- [STM32 HAL Documentation](https://www.st.com/en/embedded-software/stm32cube-mcu-mpu-packages.html)
- [Nucleo-F411RE User Manual](https://www.st.com/resource/en/user_manual/um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics.pdf)

## 📝 라이선스

MIT License

## 👨‍💻 Author

**구영모 (Kyoung-Mo Ku)**
- GitHub: [@kyoung-mo](https://github.com/kyoung-mo)

---

**📌 Note**: 
- 각 프로젝트의 `.ioc` 파일은 STM32CubeMX에서 열어 핀 설정을 확인할 수 있습니다.
- `Core/Src/main.c`에 주요 로직이 구현되어 있습니다.
- `App/` 폴더가 있는 프로젝트는 모듈화된 구조를 사용합니다.
