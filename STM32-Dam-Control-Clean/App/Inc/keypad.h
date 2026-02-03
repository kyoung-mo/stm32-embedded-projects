#ifndef INC_KEYPAD_H_
#define INC_KEYPAD_H_

#include "main.h"

// 키패드 초기화 함수 선언 추가 ⭐
void Keypad_Init(void);

// 키패드에서 눌린 문자를 반환하는 함수 (없으면 0 반환)
char Keypad_Read(void);

#endif /* INC_KEYPAD_H_ */