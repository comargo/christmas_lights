/*
 * script.h
 *
 *  Created on: Dec 21, 2021
 *      Author: cyril
 */

#ifndef INC_SCRIPT_H_
#define INC_SCRIPT_H_

#include "led_modes.h"
#include "xmas_lights.h"
#include "command.h"

struct script_mode {
	enum LED_Mode mode;
	uint32_t start_pos;
	int8_t start_palette;
	int8_t glitter;
	uint8_t immediate;
};

void ProcessScript(struct xmas *xmas, struct command *command, const struct script_mode *script, int script_size);

#endif /* INC_SCRIPT_H_ */
