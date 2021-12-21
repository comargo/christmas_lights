/*
 * morozko.c
 *
 *  Created on: 19 дек. 2021 г.
 *      Author: Кирилл
 */

#include "morozko.h"
#include "countof.h"
#include "led_modes.h"

#include "script.h"

const struct script_mode morozko_script[] = {
		{ .mode = MODE_Off, .glitter = -1 },
		{ .mode = MODE_Fire123, .start_palette = 3 },
		{ .mode = MODE_Fire123, .start_pos = 0x100 },
		{ .mode = MODE_Fire123, .start_pos = 0x200, .glitter = 1 },
		{ .mode = MODE_Black, .glitter = -1 },
		{ .mode = MODE_Palette2, .glitter = 1 },
		{ .mode = MODE_Black, .glitter = -1 },
		{ .mode = MODE_Palette1, .glitter = 1 },
		{ .mode = MODE_Palette2 },
		{ .mode = MODE_Palette3 },
		{ .mode = MODE_Palette1 } };
const int morozko_script_len = _countof(morozko_script);
