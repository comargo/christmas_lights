/*
 * ir_command.c
 *
 *  Created on: 15 дек. 2021 г.
 *      Author: Кирилл
 */


#include "main.h"
#include "ir_commands.h"

enum IR_BUTTONS {
  POWER_BUTTON = 3,
  MUTE_BUTTON = 7,
  DOWN_BUTTON = 85,
  LEFT_BUTTON = 88,
  PLAY_BUTTON = 89,
  RIGHT_BUTTON = 90,
  UP_BUTTON = 93,
};

enum LED_MODES GetModeFromIR(enum LED_MODES curMode)
{
  struct decode_results_t decode_result = {0};
  if(!CM_HAL_IRREMOTE_Decode(&irremote, &decode_result))
    return -1;

  if(curMode == MODE_Off && decode_result.command != POWER_BUTTON)
    return -1;

  switch(decode_result.command)
  {
  case POWER_BUTTON: // POWER BUTTON
    if(curMode != MODE_Off)
      return MODE_Off;
    return MODE_Rainbow;

  default:
    return -1;
  }
}