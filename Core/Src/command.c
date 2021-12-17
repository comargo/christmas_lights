/*
 * ir_command.c
 *
 *  Created on: 15 дек. 2021 г.
 *      Author: Кирилл
 */

#include "command.h"
#include <stm32_hal_btn.h>
#include <stm32_hal_irremote.h>

enum IR_BUTTONS
{
  POWER_BUTTON = 3,
  MUTE_BUTTON = 7,
  DOWN_BUTTON = 85,
  LEFT_BUTTON = 88,
  PLAY_BUTTON = 89,
  RIGHT_BUTTON = 90,
  UP_BUTTON = 93,

	VOLUP_BUTTON = 80,
	VOLDOWN_BUTTON = 81,
	FWD_BUTTON = 68,
	REV_BUTTON = 72,
	VIDEO_BUTTON = 82,
	PREV_BUTTON = 69,
	NEXT_BUTTON = 73,
};

uint8_t GetCommandFromBtn(struct command *cmd, struct CM_HAL_BTN *button)
{
	// Hold for 10sec
	if(CM_HAL_BTN_isHold(button) && HAL_GetTick()-button->btn_timer > 10000) {
		cmd->type = CMD_Power;
		return 1;
	}

	int click_count = CM_HAL_BTN_getClicks(button);
	if(click_count == 1) {
		cmd->type = CMD_ChangeMode;
		cmd->direction = 0;
		return 1;
	}
	return 0;
}

uint8_t GetCommandFromIR(struct command *cmd, struct CM_HAL_IRREMOTE *irremote)
{
  struct decode_results_t decode_result =
    { 0 };
  if (!CM_HAL_IRREMOTE_Decode(irremote, &decode_result))
    return 0;

//  if(decode_result.value == IR_REPEAT) {
//  	return 1; // Do not touch the command. It is the same
//  }

  switch (decode_result.command) {
  case POWER_BUTTON: // POWER BUTTON
  	cmd->type = CMD_Power;
  	return 1;

  case UP_BUTTON:
  	cmd->type = CMD_ChangeMode;
  	cmd->direction = 1;
  	return 1;

  case DOWN_BUTTON:
  	cmd->type = CMD_ChangeMode;
  	cmd->direction = -1;
  	return 1;

  case PLAY_BUTTON:
  	cmd->type = CMD_ChangeMode;
  	cmd->direction = 0;
  	return 1;

  case VOLUP_BUTTON:
  	cmd->type = CMD_Brightness;
  	cmd->direction = 1;
  	return 1;

  case VOLDOWN_BUTTON:
  	cmd->type = CMD_Brightness;
  	cmd->direction = -1;
  	return 1;

  case FWD_BUTTON:
  	cmd->type = CMD_Speed;
  	cmd->direction = 1;
  	return 1;

  case REV_BUTTON:
  	cmd->type = CMD_Speed;
  	cmd->direction = -1;
  	return 1;


  case VIDEO_BUTTON:
  	cmd->type = CMD_ToggleGlitter;
  	return 1;

  case NEXT_BUTTON:
  	cmd->type = CMD_GlitterChance;
  	cmd->direction = 1;
  	return 1;

  case PREV_BUTTON:
  	cmd->type = CMD_GlitterChance;
  	cmd->direction = -1;
  	return 1;

  default:
  	return 0;
  }
}
