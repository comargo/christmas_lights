/*
 * magic_clock.c
 *
 *  Created on: 21 дек. 2021 г.
 *      Author: Кирилл
 */


#include "magic_clock.h"
#include "countof.h"
#include "led_modes.h"
#include "script.h"

const struct script_mode magic_clock_script[] = {
		{.mode = MODE_Off, .glitter = -1},
		{.mode = MODE_Fire123, .start_palette=3},
		{.mode = MODE_Fire123, .start_palette=2, .start_pos = 0x100},
		{.mode = MODE_Fire123, .start_palette=1, .start_pos=0x200, .glitter=1},
		{.mode = MODE_Police, .glitter = -1},
		{.mode = MODE_Black, .glitter = -1 },
		{.mode = MODE_WhitePulse, .glitter = -1, .immediate=1 },
		{.mode = MODE_Palette1, .glitter = 1},
		{.mode = MODE_Black, .glitter = -1 },
		{.mode = MODE_Palette1, .glitter = 1},
};

const int magic_clock_len = _countof(magic_clock_script);
