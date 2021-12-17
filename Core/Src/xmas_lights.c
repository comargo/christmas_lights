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

#define GLITTER_ENABLE_BIT UINT16_C(0x100)
#define GLITTER_MASK UINT16_C(0xFF)

struct xmas g_xmas = { 0 };

struct CM_HAL_WS281x ws281x;
struct CM_HAL_IRREMOTE irremote;
struct CM_HAL_BTN button = CM_HAL_BTN_StaticInit(BTN_GPIO_Port, BTN_Pin, GPIO_PULLUP, 0);

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

static inline void SetMode(struct xmas *xmas, enum LED_Mode new_mode)
{
	xmas->prev_mode = xmas->current_mode;

	xmas->current_mode.mode = new_mode;
	xmas->current_mode.position = 0;
	xmas->current_mode.next_tick = 0;

	xmas->transition_start_tick = HAL_GetTick();
}

static inline void load_params(struct xmas *play)
{
	uint8_t hasBackup = HAL_RTCEx_BKUPRead(&hrtc, BKP_HasBackup);
	HAL_RTCEx_BKUPWrite(&hrtc, BKP_HasBackup, 1);

	play->current_mode.mode = hasBackup ? HAL_RTCEx_BKUPRead(&hrtc, BKP_LastMode) : MODE_Start;
	play->current_mode.glitter = hasBackup ? HAL_RTCEx_BKUPRead(&hrtc, BKP_Glitter) : 0;
	play->brightness_speed = hasBackup ? HAL_RTCEx_BKUPRead(&hrtc, BKP_BrightnessSpeed) : (0x7F7F);

	if (!hasBackup) {
		HAL_RTCEx_BKUPWrite(&hrtc, BKP_LastMode, play->current_mode.mode);
		HAL_RTCEx_BKUPWrite(&hrtc, BKP_Glitter, play->current_mode.glitter);
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
	// wait for 1.5*debounce_time to detect press on start-up
	HAL_Delay(button.debounce * 3 / 2);

	CM_HAL_IRREMOTE_Init(&irremote, TIM3);
	CM_HAL_IRREMOTE_Start_IT(&irremote);

	CM_HAL_WS281X_Init(&ws281x, XMAS_GPIO_Port, TIM2);
	struct CM_HAL_WS281X_Channel xmas_chan1 = {
			.GPIO_Pin = XMAS_Pin,
			.frameBuffer = (uint8_t*) g_xmas.out_buffer,
			.frameBufferSize = sizeof(g_xmas.out_buffer),
			.colorMode = WS281x_RGB };
	CM_HAL_WS281X_AddChannel(&ws281x, &xmas_chan1);

	send_lights();

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

void XMAS_Loop(struct xmas *xmas)
{
	struct command cmd = { .type = CMD_NoCommand };
	if (irremote.rcvstate == IRREMOTE_DONE) {
		GetCommandFromIR(&cmd, &irremote);
		CM_HAL_IRREMOTE_Start_IT(&irremote);
	}
	else if (CM_HAL_BTN_hasClicks(&button)) {
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
			if (powerState) {
				// In case of turning on
				enum LED_Mode new_mode = mode.curMode;
				mode.curMode = MODE_Off;
				SetMode(xmas, new_mode);
			}
			else {
				SetMode(xmas, MODE_Off);
			}
			break;
		case CMD_SetMode:
			SetMode(xmas, cmd.mode);
			break;
		case CMD_ToggleGlitter:
			xmas->current_mode.glitter ^= GLITTER_ENABLE_BIT;
			HAL_RTCEx_BKUPWrite(&hrtc, BKP_Glitter, xmas->current_mode.glitter);
			break;
		case CMD_GlitterChance: {
			uint8_t glitter = xmas->current_mode.glitter & GLITTER_MASK;
			if (cmd.direction > 0) {
				glitter = qadd8(glitter, 1);
			}
			else if (cmd.direction < 0) {
				glitter = qsub8(glitter, 1);
			}
			xmas->current_mode.glitter = (xmas->current_mode.glitter & GLITTER_ENABLE_BIT) | glitter;
			HAL_RTCEx_BKUPWrite(&hrtc, BKP_Glitter, xmas->current_mode.glitter);
			break;
		}
		case CMD_ChangeMode: {
			enum LED_Mode new_mode = mode.curMode + cmd.direction;
			if (new_mode == MODE_Off) {
				new_mode = MODE_Last - 1;
			}
			else if (new_mode == MODE_Last) {
				new_mode = MODE_Off + 1;
			}
			SetMode(&mode, new_mode);
			break;
		}
		case CMD_Brightness:
			if (cmd.direction > 0) {
				xmas->brightness = qadd8(xmas->brightness, 1);
			}
			else if (cmd.direction < 0) {
				xmas->brightness = qsub8(xmas->brightness, 1);
			}
			HAL_RTCEx_BKUPWrite(&hrtc, BKP_BrightnessSpeed, xmas->brightness_speed);
			break;
		case CMD_Speed:
			if (cmd.direction > 0) {
				xmas->speed = qadd8(xmas->speed, 1);
			}
			else if (cmd.direction < 0) {
				xmas->speed = qsub8(xmas->speed, 1);
			}
			HAL_RTCEx_BKUPWrite(&hrtc, BKP_BrightnessSpeed, xmas->brightness_speed);
			break;
		case CMD_NoCommand:
			break;
		}
	}

	if (xmas->current_mode.mode == MODE_Off && xmas->transition_complete)
		return; // Power is off. Nothing else to do

	uint8_t hasUpdates = 0;
	uint32_t tick = HAL_GetTick();

	// Calculate current state
	if(tick >= xmas->current_mode.next_tick) {
		xmas->current_mode.next_tick = GetNextTick(FillMode(xmas->current_mode), xmas->speed);
	}


	if(xmas->transition_complete)
	{
		// Copy entire state buffer to out buffer
		xmas->out_buffer = xmas->current_mode.buffer;
	}
	else {
		// Calculate previous state
		if(tick >= xmas->prev_mode.next_tick) {
			xmas->prev_mode.next_tick = GetNextTick(FillMode(xmas->prev_mode), xmas->speed);
		}
	}

	if (mode.oldMode == mode.curMode) {
		if (tick >= mode.curModeNextTick) {
			mode.curModeNextTick = GetNextTick(
					FillMode(mode.curMode, xmas_buffer, XMAS_LENGTH, &mode.curModePos), speed);
			hasUpdates = true;
			if (mode.curMode != MODE_Off && mode.curModeGlitter) {
				add_glitter(xmas_buffer, XMAS_LENGTH, 30);
			}
		}
	}
	else {
		if (mode.oldModeNextTick != HAL_MAX_DELAY && tick >= mode.oldModeNextTick) {
			mode.oldModeNextTick = GetNextTick(
					FillMode(mode.oldMode, xmas_buffer_from, XMAS_LENGTH, &mode.oldModePos), speed);
			if (mode.oldMode != MODE_Off && mode.oldModeGlitter) {
				add_glitter(xmas_buffer_from, XMAS_LENGTH, 30);
			}
		}
		if (mode.curModeNextTick != HAL_MAX_DELAY && tick >= mode.curModeNextTick) {
			mode.curModeNextTick = GetNextTick(
					FillMode(mode.curMode, xmas_buffer_to, XMAS_LENGTH, &mode.curModePos), speed);
			if (mode.curMode != MODE_Off && mode.curModeGlitter) {
				add_glitter(xmas_buffer_to, XMAS_LENGTH, mode.curModeGlitter & GLITTER_MASK);
			}
		}
		fract16 transition = (tick - mode.transitionStartTick) * 256 / 1000;
		if (transition > 0xFF)
			transition = 0xFF;
		blend_leds(xmas_buffer_from, xmas_buffer_to, xmas_buffer, XMAS_LENGTH,
				ease8InOutCubic(transition));
		if (transition == 0xFF) {
			if (!powerState) {
				//Special case of power off state:

				// Save previous mode in _curMode_
				mode.curMode = mode.oldMode;
				// Set old mode to power off
				mode.oldMode = MODE_Off;
				// It will be later checked to skip mode calculations
			}
			else {
				mode.oldMode = mode.curMode;
				if (mode.curMode != MODE_Off) {
					HAL_RTCEx_BKUPWrite(&hrtc, BKP_LastMode, mode.curMode);
				}
			}
		}
		hasUpdates = true;
	}

	if (hasUpdates) {
		nscale8(xmas_buffer, XMAS_LENGTH, brightness);
		CM_HAL_WS281X_SendBuffer(&ws281x);
		while (ws281x.state != WS281x_Ready) {
			__NOP();
		}
	}
	return;
}
