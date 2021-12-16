/*
 * ir_commands.h
 *
 *  Created on: 15 дек. 2021 г.
 *      Author: Кирилл
 */

#ifndef INC_COMMAND_H_
#define INC_COMMAND_H_

#include <stdbool.h>
#include "led_modes.h"

enum CommandType {
	CMD_NoCommand,
	CMD_Power, // TOGGLE
	CMD_SetMode, // mode
	CMD_ChangeMode, // direction
	CMD_Brightness, // direction
	CMD_Speed, // direction
	CMD_ToggleGlitter, // TOGGLE
	CMD_GlitterChance, // direction
};

struct command {
	enum CommandType type;
	union {
		enum LED_MODES mode; // CMD_SetMode
		int8_t direction; // CMD_ChangeMode, CMD_Brightness
	};
};

bool GetCommandFromBtn(struct command *cmd);
bool GetCommandFromIR(struct command *cmd);

#endif /* INC_COMMAND_H_ */
