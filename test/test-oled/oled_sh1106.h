#ifndef OLED_SH1106_H
#define OLED_SH1106_H

#include "CH58x_common.h"

// Define GPIO Pins (Using GPIOB for this example)
#define OLED_SCL_PIN    GPIO_Pin_13
#define OLED_SDA_PIN    GPIO_Pin_12

// I2C Address of SH1106 (Typically 0x3C, shifted left by 1 for write = 0x78)
#define OLED_I2C_ADDR   0x78

// Prototypes
void OLED_Init(void);
void OLED_Clear(void);
void OLED_SetPos(uint8_t x, uint8_t y);
void OLED_WriteCmd(uint8_t cmd);
void OLED_WriteData(uint8_t data);

#endif
