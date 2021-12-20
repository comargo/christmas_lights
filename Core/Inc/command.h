/*
 * ir_commands.h
 *
 *  Created on: 15 дек. 2021 г.
 *      Author: Кирилл
 */

#ifndef INC_COMMAND_H_
#define INC_COMMAND_H_

#include "led_modes.h"

struct CM_HAL_BTN;
struct CM_HAL_IRREMOTE;

enum CommandType
{
  CMD_NoCommand, CMD_Power, // TOGGLE
  CMD_SetMode, // mode
  CMD_ChangeMode, // direction
  CMD_Brightness, // direction
  CMD_Speed, // direction
  CMD_ToggleGlitter, // TOGGLE
  CMD_GlitterChance, // direction

  CMD_Channel, // channel
};

struct command
{
  enum CommandType type;
  union
  {
    enum LED_Mode mode; // CMD_SetMode
    int8_t direction; // CMD_ChangeMode, CMD_Brightness
    uint8_t channel; // CMD_Channel
  };
};

uint8_t GetCommandFromBtn(struct command *cmd, struct CM_HAL_BTN *button);
uint8_t GetCommandFromIR(struct command *cmd, struct CM_HAL_IRREMOTE *irremote);

#endif /* INC_COMMAND_H_ */
