//
// Created by Dave Cherry on 14/08/2026.
//

#ifndef TESTLTDC_FRAMEBUFFERSETUP_H
#define TESTLTDC_FRAMEBUFFERSETUP_H

#include "stm32f4xx_hal.h"

void SDRAM_Initialization_Sequence(SDRAM_HandleTypeDef* hsdram);
void FrameBuffer_EnableLTDC(LTDC_HandleTypeDef* hltdc);
void LCD_ILI9341_Init(SPI_HandleTypeDef* hspi);

#endif //TESTLTDC_FRAMEBUFFERSETUP_H
