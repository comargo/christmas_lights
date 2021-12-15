/*
 * led_modes.h
 *
 *  Created on: 15 дек. 2021 г.
 *      Author: Кирилл
 */

#ifndef INC_LED_MODES_H_
#define INC_LED_MODES_H_

#include <stdint.h>

enum LED_MODES {
  MODE_INVALID = -1,
  MODE_Off = 0,
	MODE_Start = 1,
  MODE_Rainbow,
	MODE_Police,

	MODE_Last
};

#define DEFAULT_DELAY -1
#define NO_UPDATE -2

// returns delay
int FillMode(enum LED_MODES mode, uint8_t *buffer, uint16_t numLeds, int* nPos);

#endif /* INC_LED_MODES_H_ */
