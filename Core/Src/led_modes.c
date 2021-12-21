/*
 * led_modes.c
 *
 *  Created on: 15 дек. 2021 г.
 *      Author: Кирилл
 */

#include "led_modes.h"
#include <stm32f1xx_hal.h>
#include <string.h>
#include <stdlib.h>
#include <colorutils.h>
#include <html_colors.h>
#include <gradient_palettes.h>
#include "xmas_lights.h"
#include "countof.h"

static int building_light(RGB *buffer, uint16_t numLeds, int nPos)
{
  if (nPos == 0xFFFF) {
    for (int i = 0; i < numLeds; ++i) {
      buffer[i] = RGB_Red;
    }
    return nPos;
  }
  int loPos = nPos & 0xFF;
  int hiPos = (nPos >> 8) & 0xFF;

  memset(buffer, 0, numLeds * sizeof(RGB));

  for (int i = 0; i < hiPos; ++i) {
    buffer[(numLeds - 1 - i)] = RGB_Red;
  }
  buffer[loPos] = RGB_Red;
  blur1d(buffer, numLeds, 5);
  loPos++;
  if (loPos >= numLeds - hiPos) {
    loPos = 0;
    hiPos++;
  }
  if (hiPos >= numLeds) {
    return 0xFFFF;
  }

  return (hiPos << 8) | loPos;
}

static int police_lights(RGB *buffer, uint16_t numLeds, int nPos)
{
  int state = nPos % 30;

  // i%2 = 0
  // state 0,2,4,6 - red
  // state 1,3,5,7 - black
  // state 8,10,12,14 - black
  // state 9,11,13,15 - blue
  if (state % 15 > 8) {
    memset(buffer, 0, numLeds * sizeof(RGB));
  }
  else {
    for (int i = 0; i < numLeds; ++i) {
      if ((state / 10) == ((i / 4) % 2)) {
        buffer[i] = (state % 2) ? RGB_Red : RGB_Black;
      }
      else {
        buffer[i] = (state % 2) ? RGB_Blue : RGB_Black;
      }
    }
  }

  state++;
  return state;
}

static RGB gCurrentPalette[16] =
  { 0 };
static RGB gTargetPalette[16];

void LedMode_SetTargetPallete(int palette)
{
  Palette16FromGradientPalette(gTargetPalette,
      gGradientPalettes[palette]);
}



uint32_t get_millisecond_timer()
{
  return HAL_GetTick();
}
static int colorwave(RGB *buffer, uint16_t numLeds, int nPos)
{
  static int wave1 = 0;
  static int wave2 = 0;
  static int wave3 = 0;
  static uint8_t mul1 = 5;
  static uint8_t mul2 = 8;
  static uint8_t mul3 = 7;

  nblendPaletteTowardPalette16(gCurrentPalette, gTargetPalette, 24);

  wave1 += beatsin8(10, -4, 4, 0, 0);
  wave2 += beatsin8(15, -2, 2, 0, 0);
  wave3 += beatsin8(12, -3, 3, 0, 0);

  for (int i = 0; i < numLeds; ++i) {
    uint8_t tmp = sin8(mul1 * i + wave1) + sin8(mul2 * i + wave2) + sin8(mul3 * i + wave3);
    buffer[i] = ColorFromPalette16(gCurrentPalette, tmp, 255, LINEARBLEND);
  }
  blur1d(buffer, numLeds, 15);
  nPos++;
  return nPos;
}

static int fire_up_down(RGB *buffer, uint16_t numLeds, int nPos, int ledsToFireMax)
{
  static const int going_up_max = 256*2/5;
  static const int stay_max = 256*3/5;

  int scaled_pos;
  if(nPos < going_up_max) {
    // Going Up
    scaled_pos = nPos*5/2;
  }
  else if(nPos < stay_max) {
    // Stay
    scaled_pos = 255;
  }
  else {
    // Going down
    scaled_pos = 255 - ((nPos - stay_max)*5/2);
  }

  int fired_leds = numLeds*scaled_pos/255;
  int tail_leds = ledsToFireMax+numLeds/10;
  if(tail_leds > numLeds)
    tail_leds = numLeds;

  colorwave(buffer, fired_leds, 0);
  if(fired_leds > ledsToFireMax) {
    if(fired_leds > tail_leds)
      fired_leds = tail_leds;
    int max_delta = fired_leds-ledsToFireMax;

    for(int i=ledsToFireMax; i<fired_leds; ++i) {
      int scaled_i = (fired_leds-i)*255/max_delta;
      uint8_t eased_i = ease8InOutApprox(scaled_i);
      nscale8(&buffer[i], 1, eased_i);
    }
  }

  memset(&buffer[fired_leds], 0, (numLeds-fired_leds)*sizeof(RGB));
//
//
//  for(int i=firedLeds; i<numLeds; ++i) {
//    int scale = 255-(i-firedLeds)*255/(numLeds-firedLeds);
//    nscale8(&buffer[i], 1, scale);
//  }
//
  ++nPos;
  return nPos;
}

static int fire_up(RGB *buffer, uint16_t numLeds, int nPos)
{
  int fired_leds = scale8(numLeds, nPos);
  colorwave(buffer, fired_leds, 0);
  memset(&buffer[fired_leds], 0, (numLeds-fired_leds)*sizeof(RGB));
  if(nPos == 254)
    numLeds++;

  ++nPos;
  if(nPos > 255)
    nPos = 255;
  return nPos;
}

static int fire123(RGB *buffer, uint16_t numLeds, int nPos, int fireState)
{
  int fired_leds;
  switch(fireState)
  {
  case 0:
    fired_leds = numLeds/2;
    nPos = fire_up_down(buffer, numLeds, nPos, fired_leds);
    break;
  case 1:
    fired_leds = numLeds*2/3;
    nPos = fire_up_down(buffer, numLeds, nPos, fired_leds);
    break;
  default:
    nPos = fire_up(buffer, numLeds, nPos);
  }
  return nPos;
}

int FillMode(struct xmas_state *xmas_state)
{
  switch (xmas_state->mode) {
  case MODE_Off:
    memset(xmas_state->buffer, 0, sizeof(xmas_state->buffer));
    return NO_UPDATE;
  case MODE_Start:
    xmas_state->position = building_light(xmas_state->buffer,
        _countof(xmas_state->buffer), xmas_state->position);
    if (xmas_state->position == 0xFFFF)
      return NO_UPDATE;
    return 0;
  case MODE_Rainbow:
    fill_rainbow(xmas_state->buffer, _countof(xmas_state->buffer),
        xmas_state->position, 1);
    xmas_state->position++;
    return DEFAULT_DELAY;
  case MODE_Police:
    xmas_state->position = police_lights(xmas_state->buffer,
        _countof(xmas_state->buffer), xmas_state->position);
    return 50;
  case MODE_White:
    memset(xmas_state->buffer, 0xff, sizeof(xmas_state->buffer));
    return NO_UPDATE;
  case MODE_Black:
    memset(xmas_state->buffer, 0, sizeof(xmas_state->buffer));
    return NO_UPDATE;
  case MODE_Tricolor:
    fill_solid(xmas_state->buffer, _countof(xmas_state->buffer), RGB_White);
    fill_solid(xmas_state->buffer, _countof(xmas_state->buffer) / 2, RGB_Red);
    fill_solid(xmas_state->buffer + _countof(xmas_state->buffer) / 2,
        _countof(xmas_state->buffer) / 4, RGB_Blue);
    return NO_UPDATE;
  case MODE_Palette1:
  case MODE_Palette2:
  case MODE_Palette3:
    if (xmas_state->position == 0) {
      LedMode_SetTargetPallete(xmas_state->mode - MODE_Palette1);
    }
    xmas_state->position = colorwave(xmas_state->buffer,
        _countof(xmas_state->buffer), xmas_state->position);
    return DEFAULT_DELAY;
  case MODE_Fire123: {
    if (xmas_state->position == 0) {
      Palette16FromGradientPalette(gTargetPalette,
          gGradientPalettes[2]);
      memcpy(gCurrentPalette, gTargetPalette, sizeof(gTargetPalette));
    }
    uint8_t fire123_state = (xmas_state->position & 0xFF00)>>8;
    xmas_state->position = fire123(xmas_state->buffer, _countof(xmas_state->buffer), xmas_state->position&0xFF, fire123_state);
    xmas_state->position |= fire123_state*0x100;
    if((xmas_state->position & 0xFF) == 0xFF && fire123_state != 2) {
      memset(xmas_state->buffer, 0, sizeof(xmas_state->buffer));
      return NO_UPDATE;
    }
    return DEFAULT_DELAY;
  }
  default:
    return NO_UPDATE;
  }
}

void add_glitter(RGB *buffer, uint16_t numLeds, int chance)
{
  int den = RAND_MAX / 256;
  int r = rand();
  if ((r / den) < chance) {
    buffer[(rand() >> 16) * numLeds / (RAND_MAX >> 16)] = RGB_White;
  }
}
