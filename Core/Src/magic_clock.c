/*
 * magic_clock.c
 *
 *  Created on: 21 дек. 2021 г.
 *      Author: Кирилл
 */


#include "magic_clock.h"

enum MC_State
{
  MС_Off,
  MС_Fire1,
  MС_Fire2,
  MС_Fire3,
  MC_Police,
  MC_Off2,
  MC_WhiteSplash,
  MC_Off3,
  MC_On,
};

static void MC_SetMode(enum MZ_State state, struct xmas *xmas)
{
  switch(state) {
  case MC_Off:
  case MC_Off2:
  case MC_Off3:
    XMAS_SetMode(xmas, MODE_Off);
    xmas->glitter = 0;
    break;
  case MC_Fire1:
  case MC_Fire2:
  case MC_Fire3:
    LedMode_SetTargetPalette(0);
    XMAS_SetMode(xmas, MODE_Fire123);
    xmas->current_mode.position = (state-MZ_Fire1)*0x100;
    break;
  case MC_Police:
    XMAS_SetMode(xmas, MODE_Police);
    break;
  case MC_WhiteSplash:
    XMAS_SetMode(xmas, MODE_White);
    xmas->brightness = 255;
    xmas->transition_complete = 1;
    break;
  case MC_On:
    XMAS_SetMode(xmas, MODE_Palette1);
    break;

  }

  xmas->script_state = state;
}

void MagicClockScript(struct xmas *xmas, struct command *command)
{
  switch (command->type) {
  case CMD_Channel:
    MZ_SetMode(MZ_Off, xmas);
    break;
  case CMD_ChangeMode: {
    enum MC_State mc_state = xmas->script_state + command->direction;
    if (mc_state < MC_Off)
      mc_state = MC_On;
    if (mc_state > MC_On)
      mc_state = MC_Off;
    MZ_SetMode(mc_state, xmas);
    break;
  }
  default:
    break;
  }
}
