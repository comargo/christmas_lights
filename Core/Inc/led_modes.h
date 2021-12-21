/*
 * led_modes.h
 *
 *  Created on: 15 дек. 2021 г.
 *      Author: Кирилл
 */

#ifndef INC_LED_MODES_H_
#define INC_LED_MODES_H_

#include <stdint.h>
#include <color_types.h>

enum LED_Mode
{
  MODE_INVALID = -1,
  MODE_Off = 0,
  MODE_Start,
  MODE_Rainbow,
  MODE_White,
  MODE_Black,
  MODE_Police,
  MODE_Tricolor,
  MODE_Palette1,
  MODE_Palette2,
  MODE_Palette3,
  MODE_Fire123,

  MODE_Last
};

#define DEFAULT_DELAY -1
#define NO_UPDATE -2

// returns delay
struct xmas_state;

void LedMode_SetTragetPallete(int pallete);
int FillMode(struct xmas_state *xmas_state);

void add_glitter(RGB *buffer, uint16_t numLeds, int chance);

#endif /* INC_LED_MODES_H_ */
