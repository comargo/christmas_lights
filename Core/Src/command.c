/*
 * ir_command.c
 *
 *  Created on: 15 дек. 2021 г.
 *      Author: Кирилл
 */

#include "main.h"
#include "command.h"
#include "stm32_hal_btn.h"

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

bool GetCommandFromBtn(struct command *cmd)
{
	// Hold for 10sec
	if(CM_HAL_BTN_isHold(&button) && HAL_GetTick()-button.btn_timer > 10000) {
		cmd->type = CMD_Power;
		return true;
	}

	int click_count = CM_HAL_BTN_getClicks(&button);
	if(click_count == 1) {
		cmd->type = CMD_ChangeMode;
		cmd->direction = 0;
		return true;
	}
	return false;
}

bool GetCommandFromIR(struct command *cmd)
{
  struct decode_results_t decode_result =
    { 0 };
  if (!CM_HAL_IRREMOTE_Decode(&irremote, &decode_result))
    return false;

//  if(decode_result.value == IR_REPEAT) {
//  	return true; // Do not touch the command. It is the same
//  }

  switch (decode_result.command) {
  case POWER_BUTTON: // POWER BUTTON
  	cmd->type = CMD_Power;
  	return true;

  case UP_BUTTON:
  	cmd->type = CMD_ChangeMode;
  	cmd->direction = 1;
  	return true;

  case DOWN_BUTTON:
  	cmd->type = CMD_ChangeMode;
  	cmd->direction = -1;
  	return true;

  case PLAY_BUTTON:
  	cmd->type = CMD_ChangeMode;
  	cmd->direction = 0;
  	return true;

  case VOLUP_BUTTON:
  	cmd->type = CMD_Brightness;
  	cmd->direction = 1;
  	return true;

  case VOLDOWN_BUTTON:
  	cmd->type = CMD_Brightness;
  	cmd->direction = -1;
  	return true;

  case FWD_BUTTON:
  	cmd->type = CMD_Speed;
  	cmd->direction = 1;
  	return true;

  case REV_BUTTON:
  	cmd->type = CMD_Speed;
  	cmd->direction = -1;
  	return true;


  case VIDEO_BUTTON:
  	cmd->type = CMD_ToggleGlitter;
  	return true;

  case NEXT_BUTTON:
  	cmd->type = CMD_GlitterChance;
  	cmd->direction = 1;
  	return true;

  case PREV_BUTTON:
  	cmd->type = CMD_GlitterChance;
  	cmd->direction = -1;
  	return true;

  default:
  	return false;
  }
}
