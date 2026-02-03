#ifndef AP_H_
#define AP_H_

#include "main.h"
#include "adc.h"
#include "dma.h"
#include "usart.h"
#include "gpio.h"
#include "ap_def.h"
#include "i2c.h"


void apInit(void);
void apMain(void);

#endif