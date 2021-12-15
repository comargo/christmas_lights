/*
 * led_modes.c
 *
 *  Created on: 15 дек. 2021 г.
 *      Author: Кирилл
 */

#include "led_modes.h"
#include <string.h>
#include <colorutils.h>

static int building_light(uint8_t *buffer, uint16_t numLeds, int nPos)
{
	if (nPos == 0xFFFF) {
		for (int i = 0; i < numLeds; ++i) {
			set_pixel(buffer + 3 * i, RGB(255, 0, 0));
		}
		return nPos;
	}
	int loPos = nPos & 0xFF;
	int hiPos = (nPos >> 8) & 0xFF;

	memset(buffer, 0, numLeds * 3);

	for (int i = 0; i < hiPos; ++i) {
		set_pixel(buffer + 3 * (numLeds - 1 - i), RGB(255, 0, 0));
	}
	set_pixel(buffer + 3 * loPos, RGB(255, 0, 0));
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

static int police_lights(uint8_t *buffer, uint16_t numLeds, int nPos)
{
	int state = nPos % 20;
	const uint32_t red = RGB(63, 0, 0);
	const uint32_t blue = RGB(0, 0, 63);
	const uint32_t black = 0;

	// i%2 = 0
	// state 0,2,4,6 - red
	// state 1,3,5,7 - black
	// state 8,10,12,14 - black
	// state 9,11,13,15 - blue
	if (state % 10 > 8) {
		memset(buffer, 0, numLeds * 3);
	}
	else {
		for (int i = 0; i < numLeds; ++i) {
			if ((state / 10) == ((i / 4) % 2)) {
				set_pixel(buffer + 3 * i, (state % 2) ? red : black);
			}
			else {
				set_pixel(buffer + 3 * i, (state % 2) ? blue : black);
			}
		}
	}

	state++;
	return state;
}

int FillMode(enum LED_MODES mode, uint8_t *buffer, uint16_t numLeds, int *nPos)
{
	switch (mode) {
	case MODE_Off:
		memset(buffer, 0, numLeds * 3);
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
	default:
		return NO_UPDATE;
	}
}
