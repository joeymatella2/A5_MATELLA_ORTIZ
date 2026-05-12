#ifndef DACPT1_H
#define DACPT1_H

#include "stm32l4xx_hal.h"
#include <stdint.h>

void     DAC_init(void);
uint16_t DAC_volt_conv(float voltage);
void     DAC_write(uint16_t data);

#endif /* DACPT1_H */
