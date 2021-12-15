/*
 * led_modes.c
 *
 *  Created on: 15 дек. 2021 г.
 *      Author: Кирилл
 */

#include "led_modes.h"
#include <string.h>
#include <colorutils.h>


void FillMode(enum LED_MODES mode, uint8_t *buffer, uint16_t numLeds, int* nPos)
{
  switch(mode) {
  case MODE_Off:
    memset(buffer, 0, numLeds*3);
    return;
  case MODE_Rainbow:
    fill_rainbow(buffer, numLeds, *nPos, 1);
    nPos++;
    return;
  case MODE_INVALID:
    return;
  }
}
