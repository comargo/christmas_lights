/*
 * christmas_lights.c
 *
 *  Created on: Dec 17, 2021
 *      Author: cyril
 */

#include "xmas_lights.h"
#include <string.h>
#include <stdio.h>

#include <colorutils.h>
#include "command.h"
#include "main.h"
#include "rtc.h"

#include "morozko.h"
#include "magic_clock.h"

struct xmas g_xmas =
  { .transition_complete = 1 };

struct CM_HAL_IRREMOTE irremote;
struct CM_HAL_BTN button =
    CM_HAL_BTN_StaticInit(BTN_GPIO_Port, BTN_Pin, GPIO_PULLUP, 0);
struct CM_HAL_WS281x ws281x;
struct CM_HAL_WS281X_Channel xmas_chan1 =
  { .GPIO_Pin = XMAS_Pin,
      .frameBuffer = (uint8_t*) g_xmas.out_buffer,
      .frameBufferSize = sizeof(g_xmas.out_buffer),
      .colorMode = WS281x_RGB };

static inline uint8_t qchange8(uint8_t val, int amount)
{
  int t = val + amount;
  if (t < 0)
    t = 0;
  if (t > 255)
    t = 255;
  return t;
}

static inline uint32_t GetNextTick(int delay, uint8_t speed)
{
  if (delay == NO_UPDATE) {
    return HAL_MAX_DELAY;
  }
  if (delay == DEFAULT_DELAY) {
    delay = 1000 / 60;
  }
  return HAL_GetTick() + delay;
}

void XMAS_SetMode(struct xmas *xmas, enum LED_Mode new_mode)
{
  xmas->prev_mode = xmas->current_mode;

  xmas->current_mode.mode = new_mode;
  xmas->current_mode.position = 0;
  xmas->current_mode.next_tick = 0;

  xmas->transition_start_tick = HAL_GetTick();
  xmas->transition_complete = 0;
}

static inline void load_params(struct xmas *play)
{
  uint8_t hasBackup = HAL_RTCEx_BKUPRead(&hrtc, BKP_HasBackup);
  HAL_RTCEx_BKUPWrite(&hrtc, BKP_HasBackup, 1);

  play->current_mode.mode =
      hasBackup ? HAL_RTCEx_BKUPRead(&hrtc, BKP_LastMode) : MODE_Start;
  play->glitter = hasBackup ? HAL_RTCEx_BKUPRead(&hrtc, BKP_Glitter) : 0;
  play->brightness_speed =
      hasBackup ? HAL_RTCEx_BKUPRead(&hrtc, BKP_BrightnessSpeed) : (0x7F7F);

  if (!hasBackup) {
    HAL_RTCEx_BKUPWrite(&hrtc, BKP_LastMode, play->current_mode.mode);
    HAL_RTCEx_BKUPWrite(&hrtc, BKP_Glitter, play->glitter);
    HAL_RTCEx_BKUPWrite(&hrtc, BKP_BrightnessSpeed, play->brightness_speed);
  }
}

void send_lights()
{
  CM_HAL_WS281X_SendBuffer(&ws281x);
  while (ws281x.state != WS281x_Ready) {
    __NOP();
  }
}

struct xmas* XMAS_Init()
{
  CM_HAL_BTN_Init(&button);
  uint32_t startTick = HAL_GetTick();

  CM_HAL_IRREMOTE_Init(&irremote, TIM3);
  CM_HAL_IRREMOTE_Start_IT(&irremote);

  CM_HAL_WS281X_Init(&ws281x, XMAS_GPIO_Port, TIM2);
  CM_HAL_WS281X_AddChannel(&ws281x, &xmas_chan1);

  send_lights();

  // wait for 1.5*debounce_time to detect press on start-up
  while (HAL_GetTick() - startTick < (button.debounce * 3 / 2))
    __NOP();
  if (CM_HAL_BTN_isPress(&button)) {
    // button is pressed now. Wait for 1s to detect reset
    // wait for 1s detect Reset
    while (HAL_GetTick() - startTick < 1000)
      __NOP();
    if (!CM_HAL_BTN_isRelease(&button)) {
      // button has not been release.
      // Clear backup registers
      __HAL_RCC_BACKUPRESET_FORCE();
      __HAL_RCC_BACKUPRESET_RELEASE();
    }
  }

  load_params(&g_xmas);
  return &g_xmas;
}

void NoScriptMode(struct xmas *xmas, struct command *cmd)
{
  switch (cmd->type) {
  case CMD_Power:
    xmas->prev_mode.mode = MODE_Off;
    xmas->prev_mode.next_tick = 0;
    break;
  case CMD_SetMode:
    XMAS_SetMode(xmas, cmd->mode);
    break;
  case CMD_ToggleGlitter:
    xmas->glitter ^= GLITTER_ENABLE_BIT;
    HAL_RTCEx_BKUPWrite(&hrtc, BKP_Glitter, xmas->glitter);
    break;
  case CMD_GlitterChance: {
    uint8_t glitter = qchange8(xmas->glitter & GLITTER_MASK, cmd->direction);
    xmas->glitter = (xmas->glitter & GLITTER_ENABLE_BIT) | glitter;
    HAL_RTCEx_BKUPWrite(&hrtc, BKP_Glitter, xmas->glitter);
    break;
  }
  case CMD_ChangeMode: {
    enum LED_Mode new_mode = xmas->current_mode.mode + cmd->direction;
    if (new_mode == MODE_Off) {
      new_mode = MODE_Last - 1;
    }
    else if (new_mode == MODE_Last) {
      new_mode = MODE_Off + 1;
    }
    XMAS_SetMode(xmas, new_mode);
    break;
  }
  case CMD_Brightness:
    xmas->brightness = qchange8(xmas->brightness, cmd->direction);
    HAL_RTCEx_BKUPWrite(&hrtc, BKP_BrightnessSpeed, xmas->brightness_speed);
    break;
  case CMD_Speed:
    xmas->speed = qchange8(xmas->speed, cmd->direction);
    HAL_RTCEx_BKUPWrite(&hrtc, BKP_BrightnessSpeed, xmas->brightness_speed);
    break;
  case CMD_Channel: // processed elsewhere
  case CMD_NoCommand:
    break;
  }
}

void XMAS_Loop(struct xmas *xmas)
{
  struct command cmd =
    { .type = CMD_NoCommand };
  if (irremote.rcvstate == IRREMOTE_DONE) {
    GetCommandFromIR(&cmd, &irremote);
    CM_HAL_IRREMOTE_Start_IT(&irremote);
  }
  else {
    GetCommandFromBtn(&cmd, &button);
    if (!xmas->power && cmd.type == CMD_ChangeMode) {
      // special case. Use hardware button single click to power on
      cmd.type = CMD_Power;
    }
  }

  if (xmas->power || cmd.type == CMD_Power) {
    switch (cmd.type) {
    case CMD_Power:
      xmas->power = !xmas->power;
      xmas->transition_complete = 0;
      xmas->transition_start_tick = HAL_GetTick();
      break;
    case CMD_Channel:
      memset(xmas->out_buffer, 0, sizeof(xmas->out_buffer));
      send_lights();
      xmas->script = cmd.channel;
// @formatter:off
      xmas->prev_mode = (struct xmas_state){ .mode = MODE_Off };
      xmas->current_mode = (struct xmas_state){ .mode = MODE_Off };
// @formatter:on
      break;
    default: // Handled elsewhere
      break;
    }
  }

  if (!xmas->power && xmas->transition_complete) {
    xmas->script = 0;
    return; // Power is off. Nothing else to do
  }

  switch(xmas->script)
  {
  case 0: // No script, main sequence
    NoScriptMode(xmas, &cmd);
    break;
  case 1: // 1st script Morozko
  	ProcessScript(xmas, &cmd, morozko_script, morozko_script_len);
    break;
  case 2: // 2ndt script Magic Clock
  	ProcessScript(xmas, &cmd, magic_clock_script, magic_clock_len);
    break;
  }

  uint8_t hasUpdates = 0;
  uint32_t tick = HAL_GetTick();
  // Calculate current state
  if (tick >= xmas->current_mode.next_tick) {
    xmas->current_mode.next_tick = GetNextTick(FillMode(&xmas->current_mode),
        xmas->speed);
    hasUpdates = 1;
  }
  if (xmas->transition_complete) {
    // Copy entire state buffer to out buffer
    memcpy(xmas->out_buffer, xmas->current_mode.buffer,
        sizeof(xmas->current_mode.buffer));
  }
  else {
    fract16 transition = (tick - xmas->transition_start_tick) * 256 / 1000;
    if (transition > 0xFF)
      transition = 0xFF;

    if (xmas->power) {
      // Do not make calculations if power is turning off
      // Calculate previous state
      if (tick >= xmas->prev_mode.next_tick) {
        xmas->prev_mode.next_tick = GetNextTick(FillMode(&xmas->prev_mode),
            xmas->speed);
      }
      blend_leds(xmas->prev_mode.buffer, xmas->current_mode.buffer,
          xmas->out_buffer, XMAS_LENGTH, ease8InOutCubic(transition));
    }
    else {
      memcpy(xmas->out_buffer, xmas->current_mode.buffer,
          sizeof(xmas->current_mode.buffer));
      fadeToBlackBy(xmas->out_buffer, XMAS_LENGTH, transition);
    }
    if (transition == 0xFF) {
      xmas->transition_complete = 1;
      if (xmas->power) {
        HAL_RTCEx_BKUPWrite(&hrtc, BKP_LastMode, xmas->current_mode.mode);
      }
    }
    hasUpdates = 1;
  }

  if (hasUpdates || (xmas->glitter & GLITTER_ENABLE_BIT)) {
    nscale8(xmas->out_buffer, XMAS_LENGTH, xmas->brightness);
    if (xmas->power && (xmas->glitter & GLITTER_ENABLE_BIT))
      add_glitter(xmas->out_buffer, XMAS_LENGTH, xmas->glitter & GLITTER_MASK);
    send_lights();
  }
  return;
}
