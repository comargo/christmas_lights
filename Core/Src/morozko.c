/*
 * morozko.c
 *
 *  Created on: 19 дек. 2021 г.
 *      Author: Кирилл
 */


#include "morozko.h"
#include "countof.h"
#include "led_modes.h"

enum MZ_State {
  MZ_Off,
  MZ_Fire1,
  MZ_Fire2,
  MZ_Fire3, // Cold lght
  MZ_Game_Round1_Off,
  MZ_Game_Round1_On, // Medium light
  MZ_Game_Round2_Off,
  MZ_Game_Round2_On, // Warm light
  MZ_Cooling1, // Medium light
  MZ_Cooling2, // Cold light
  MZ_Warming, // Warm light
};

static void MZ_SetMode(enum MZ_State state, struct xmas *xmas)
{
  switch(state) {
  case MZ_Off:
  case MZ_Game_Round1_Off:
  case MZ_Game_Round2_Off:
    XMAS_SetMode(xmas, MODE_Off);
    break;
  case MZ_Fire1:
  case MZ_Fire2:
  case MZ_Fire3:
    LedMode_SetTargetPalette(2);
    XMAS_SetMode(xmas, MODE_Fire123);
    xmas->current_mode.position = (state-MZ_Fire1)*0x100;
    break;

  case MZ_Game_Round1_On: // Medium light
  case MZ_Cooling1: // Medium light
    XMAS_SetMode(xmas, MODE_Palette2);
    break;

  case MZ_Game_Round2_On: // Warm light
  case MZ_Warming: // Warm light
    XMAS_SetMode(xmas, MODE_Palette1);
    break;

  case MZ_Cooling2:
    XMAS_SetMode(xmas, MODE_Palette3);
    break;
  }
  xmas->script_state = state;
}

void MorozkoScript(struct xmas *xmas, struct command *command)
{
  switch (command->type) {
  case CMD_Channel:
    MZ_SetMode(MZ_Off, xmas);
    break;
  case CMD_ChangeMode: {
    enum MZ_State mz_state = xmas->script_state + command->direction;
    if (mz_state < MZ_Off)
      mz_state = MZ_Warming;
    if (mz_state > MZ_Warming)
      mz_state = MZ_Off;
    MZ_SetMode(mz_state, xmas);
    break;
  }
  default:
    break;
  }
}
