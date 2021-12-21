/*
 * christmas_lights.h
 *
 *  Created on: Dec 17, 2021
 *      Author: cyril
 */

#ifndef INC_CHRISTMAS_LIGHTS_H_
#define INC_CHRISTMAS_LIGHTS_H_

#include <stm32_hal_ws281x.h>
#include <stm32_hal_irremote.h>
#include <stm32_hal_btn.h>

#include "led_modes.h"

#define XMAS_LENGTH 200

struct xmas_state
{
  enum LED_Mode mode;
  uint32_t position;
  uint32_t next_tick;
  RGB buffer[XMAS_LENGTH];
};

struct xmas
{
  struct xmas_state prev_mode;
  struct xmas_state current_mode;

  uint32_t transition_start_tick;

  RGB out_buffer[XMAS_LENGTH];

  uint16_t glitter;
  union
  {
    struct
    {
      uint8_t brightness;
      uint8_t speed;
    };
    uint16_t brightness_speed;
  };

  uint8_t power;
  uint8_t transition_complete;

  uint8_t script;
  uint8_t script_state;
};

#define GLITTER_ENABLE_BIT UINT16_C(0x100)
#define GLITTER_MASK UINT16_C(0xFF)

extern struct CM_HAL_WS281x ws281x;
extern struct CM_HAL_IRREMOTE irremote;
extern struct CM_HAL_BTN button;

struct xmas* XMAS_Init();
void XMAS_Loop(struct xmas *xmas);
void XMAS_SetMode(struct xmas *xmas, enum LED_Mode new_mode);

#endif /* INC_CHRISTMAS_LIGHTS_H_ */
