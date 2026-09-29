/*
 * Copyright (C) 2016 FIX94
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */
#ifndef __SCREEN_H__
#define __SCREEN_H__

#include <gba_types.h>

// GBA screen: title, cart info, a status line and a progress bar
// built on the libgba text console (30x20 characters)

void screen_init(void);

// shows the cart header info, pass gamesize -1 for "no cartridge"
void screen_cart(const u8 *header, s32 gamesize, u32 savesize);
// one line of status text, clears the progress bar
void screen_status(const char *msg);
// replaces the status text but keeps the progress bar, e.g. for "Done!"
void screen_status_line(const char *msg);

// progress bar, safe to call inside transfer loops:
// progress_update only compares and writes a few map entries,
// no division or printf, so the link timing is not disturbed
typedef struct
{
	u32 total;
	u32 cell_next, cell_step;
	u32 pct_next, pct_step;
	u8 cells;
	u8 pct[3];	// hundreds, tens, ones
} progress_t;

void progress_start(progress_t *p, u32 total);
void progress_update_slow(progress_t *p, u32 done);
static inline void progress_update(progress_t *p, u32 done)
{
	if(done >= p->cell_next || done >= p->pct_next)
		progress_update_slow(p, done);
}
void progress_finish(progress_t *p);

#endif
