/*
 * Copyright (C) 2016 FIX94
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */
#ifndef __INPUT_H__
#define __INPUT_H__

#include <gccore.h>

// abstract buttons so the app does not depend on a specific controller
#define INPUT_A		(1<<0)
#define INPUT_B		(1<<1)
#define INPUT_X		(1<<2)
#define INPUT_Y		(1<<3)
#define INPUT_Z		(1<<4)
#define INPUT_START	(1<<5)
#define INPUT_UP	(1<<6)
#define INPUT_DOWN	(1<<7)
#define INPUT_LEFT	(1<<8)
#define INPUT_RIGHT	(1<<9)
#define INPUT_OTHER	(1<<31)	// any button without its own bit

void input_init(void);
// reads controller state, call once per frame
void input_scan(void);
// buttons currently held
u32 input_held(void);
// buttons pressed since the last scan
u32 input_down(void);

#endif
