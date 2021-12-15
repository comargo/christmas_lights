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
  case MODE_Start: {
    if(*nPos == 0) {
      memset(buffer, 0, numLeds*3);
    }
    int hiPos = 200 - (((*nPos)&0xFF)>>8);
    int loPos = (*nPos)&0xFF;
    for(int i=199; i>=hiPos; --i) {
      set_pixel(buffer+3*i, RGB(255,0,0));
    }
    if(hiPos == 0)
      return;
    if(loPos) {
      set_pixel(buffer+3*(loPos-1), RGB(0,0,0));
    }
    set_pixel(buffer+3*(loPos), RGB(255,0,0));
    loPos++;
    if(loPos == hiPos) {
      hiPos--;
      loPos=0;
    }
    hiPos = 200-hiPos;
    *nPos = (hiPos<<8)|loPos;
    return;
    blur1d(buffer, numLeds, 3);
  }
  case MODE_Rainbow:
    fill_rainbow(buffer, numLeds, *nPos, 1);
    blur1d(buffer, numLeds, 5);
    (*nPos)++;
    return;
  case MODE_White:
    memset(buffer, 0xFF, numLeds*3);
    return;
  case MODE_INVALID:
    return;
  }
}
