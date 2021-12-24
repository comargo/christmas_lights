/*
 * script.c
 *
 *  Created on: Dec 21, 2021
 *      Author: cyril
 */


#include "script.h"
#include "led_modes.h"

static void Script_SetMode(int8_t next_state, struct xmas *xmas, const struct script_mode *script, int script_size)
{
	XMAS_SetMode(xmas, script[next_state].mode);
  if(script[next_state].start_palette != 0) {
    LedMode_SetTargetPalette(script[next_state].start_palette);
  }
  xmas->current_mode.position = script[next_state].start_pos;
  if(script[next_state].glitter != 0) {
  	if(script[next_state].glitter > 0)
  		xmas->glitter |= GLITTER_ENABLE_BIT;
  	else
  		xmas->glitter &= ~GLITTER_ENABLE_BIT;
  }
  if(script[next_state].immediate) {
  	xmas->transition_complete = 1;
  }
  xmas->script_state = next_state;
}

void ProcessScript(struct xmas *xmas, struct command *command, const struct script_mode *script, int script_size)
{
  switch (command->type) {
  case CMD_Channel:
    Script_SetMode(0, xmas, script, script_size);
    break;
  case CMD_ChangeMode: {
  	int8_t next_state = xmas->script_state + command->direction;
    if (next_state < 0)
    	next_state = script_size-1;
    if (next_state >= script_size)
      next_state = 0;
    Script_SetMode(next_state, xmas, script, script_size);
    break;
  }
  default:
    break;
  }
}
