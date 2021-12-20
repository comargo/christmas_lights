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

  CH1_BUTTON = 12,
  CH2_BUTTON = 13,
  CH3_BUTTON = 14,
  CH4_BUTTON = 16,
  CH5_BUTTON = 17,
  CH6_BUTTON = 18,
  CH7_BUTTON = 20,
  CH8_BUTTON = 21,
  CH9_BUTTON = 22,
  CHX_XX_BUTTON = 24,
  CH0_BUTTON = 25,
  GOTO_BUTTON = 26,

  STEP_BUTTON = 28,
  PAUSE_BUTTON = 29,
  STOP_BUTTON = 30,
  TITLE_BUTTON = 31,


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
  PREV_BUTTON = 73,
  NEXT_BUTTON = 69,
};

uint8_t GetCommandFromBtn(struct command *cmd, struct CM_HAL_BTN *button)
{
  // Hold for 10sec
  if (CM_HAL_BTN_isHold(button) && HAL_GetTick() - button->btn_timer > 3000) {
    // Holden flag will be reset here
    if (CM_HAL_BTN_isHolded(button)) {
      cmd->type = CMD_Power;
    }
    return 1;
  }

  int click_count = CM_HAL_BTN_getClicks(button);
  if (click_count == 1) {
    cmd->type = CMD_ChangeMode;
    cmd->direction = 1;
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

  case CH1_BUTTON:
  case CH2_BUTTON:
  case CH3_BUTTON:
  case CH4_BUTTON:
  case CH5_BUTTON:
  case CH6_BUTTON:
  case CH7_BUTTON:
  case CH8_BUTTON:
  case CH9_BUTTON: {
    int btn = decode_result.command - CH1_BUTTON;
    int ch = btn/4 + btn%4;
    cmd->type = CMD_Channel;
    cmd->channel = ch+1;
    return 1;
  }
  case CH0_BUTTON:
    cmd->type = CMD_Channel;
    cmd->channel = 0;
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
