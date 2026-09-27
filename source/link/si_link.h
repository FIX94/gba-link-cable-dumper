/*
 * Copyright (C) 2016 FIX94
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */
#ifndef __SI_LINK_H__
#define __SI_LINK_H__

#include <gccore.h>

// low level GBA link cable access over the SI bus (controller port 2)

void si_link_init(void);

// device detection, non-blocking so the caller can keep polling input
void si_link_probe_start(void);
// returns 0 while still probing, otherwise the SI device type
u32 si_link_probe_poll(void);

void si_link_reset(void);
// returns the JOY status byte
u8 si_link_status(void);
u32 si_link_recv(void);
void si_link_send(u32 msg);

#endif
