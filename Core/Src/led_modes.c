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
	int state = nPos % 20;

	// i%2 = 0
	// state 0,2,4,6 - red
	// state 1,3,5,7 - black
	// state 8,10,12,14 - black
	// state 9,11,13,15 - blue
	if (state % 10 > 8) {
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

static RGB gCurrentPalette[16] = {0};
static RGB gTargetPalette[16];
uint32_t get_millisecond_timer()
{
	return HAL_GetTick();
}
static int colorwave(RGB *buffer, uint16_t numLeds, int nPos)
{
	nblendPaletteTowardPalette16(gCurrentPalette, gTargetPalette, 24);

	for(int i=0; i<numLeds; ++i) {
		buffer[i] = ColorFromPalette16(gCurrentPalette, i, 255, LINEARBLEND);
	}
	nPos++;
	return nPos;
}

int FillMode(enum LED_MODES mode, RGB *buffer, uint16_t numLeds, int *nPos)
{
	switch (mode) {
	case MODE_Off:
		memset(buffer, 0, numLeds * sizeof(RGB));
		return NO_UPDATE;
	case MODE_Start:
		*nPos = building_light(buffer, numLeds, (*nPos));
		if (*nPos == 0xFFFF)
			return NO_UPDATE;
		return 0;
	case MODE_Rainbow:
		fill_rainbow(buffer, numLeds, (*nPos), 1);
//    blur1d(buffer, numLeds, 5);
		(*nPos)++;
		return DEFAULT_DELAY;
	case MODE_Police:
		*nPos = police_lights(buffer, numLeds, (*nPos));
		return 100;
	case MODE_White:
		memset(buffer, 0xFF, numLeds*sizeof(RGB));
		return NO_UPDATE;
	case MODE_Tricolor:
		fill_solid(buffer, numLeds, RGB_White);
		fill_solid(buffer, numLeds/3, (RGB){{213,43,30}});
		fill_solid(buffer+numLeds/3, numLeds/3, (RGB){{0,57,166}});
		return NO_UPDATE;
	case MODE_Palette1:
	case MODE_Palette2:
	case MODE_Palette3:
		if(*nPos == 0) {
			Palette16FromGradientPalette(gTargetPalette, gGradientPalettes[mode-MODE_Palette1]);
		}
		*nPos = colorwave(buffer, numLeds, (*nPos));
		return DEFAULT_DELAY;
	default:
		return NO_UPDATE;
	}
}

void add_glitter(RGB* buffer, uint16_t numLeds, int chance)
{
  if((rand()>>24) < chance) {
    buffer[(rand()>>16)*numLeds/(RAND_MAX>>16)] = RGB_White;
  }
}
