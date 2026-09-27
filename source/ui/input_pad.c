/*
 * Copyright (C) 2016 FIX94
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */
#include <gccore.h>
#include "ui/input.h"

// GameCube controller in port 1

static u32 map_buttons(u32 pad)
{
	u32 out = 0;
	if(pad & PAD_BUTTON_A) out |= INPUT_A;
	if(pad & PAD_BUTTON_B) out |= INPUT_B;
	if(pad & PAD_BUTTON_X) out |= INPUT_X;
	if(pad & PAD_BUTTON_Y) out |= INPUT_Y;
	if(pad & PAD_TRIGGER_Z) out |= INPUT_Z;
	if(pad & PAD_BUTTON_START) out |= INPUT_START;
	if(pad & PAD_BUTTON_UP) out |= INPUT_UP;
	if(pad & PAD_BUTTON_DOWN) out |= INPUT_DOWN;
	if(pad & PAD_BUTTON_LEFT) out |= INPUT_LEFT;
	if(pad & PAD_BUTTON_RIGHT) out |= INPUT_RIGHT;
	if(pad & ~(PAD_BUTTON_A|PAD_BUTTON_B|PAD_BUTTON_X|PAD_BUTTON_Y|PAD_TRIGGER_Z|
		PAD_BUTTON_START|PAD_BUTTON_UP|PAD_BUTTON_DOWN|PAD_BUTTON_LEFT|PAD_BUTTON_RIGHT))
		out |= INPUT_OTHER;
	return out;
}

void input_init(void)
{
	PAD_Init();
}

void input_scan(void)
{
	PAD_ScanPads();
}

u32 input_held(void)
{
	return map_buttons(PAD_ButtonsHeld(0));
}

u32 input_down(void)
{
	return map_buttons(PAD_ButtonsDown(0));
}
