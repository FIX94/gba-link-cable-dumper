/*
 * Copyright (C) 2016 FIX94
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */
#ifndef __UI_H__
#define __UI_H__

#include <gccore.h>
#include "link/gba_protocol.h"

// everything the app shows on screen goes through here, so the text console
// in ui_console.c can be swapped for a graphical front end

void ui_init(void);
// waits for the next frame
void ui_frame(void);

// clears the screen and draws the title
void ui_clear(void);
// status line, printf style
void ui_status(const char *fmt, ...) __attribute__((format(printf,1,2)));
// shows an error for a moment and returns
void ui_warn(const char *msg);
// shows an error and exits the app
void ui_fatal(const char *msg) __attribute__((noreturn));
// exits the app because the user asked to
void ui_exit(void) __attribute__((noreturn));

typedef struct
{
	const char *label;
	const char *value;	// shown after the label, may be NULL
} ui_menu_item;

// full screen list with the cursor item highlighted,
// long lists scroll to keep the cursor visible
void ui_draw_menu(const char *title, const ui_menu_item *items, int count, int cursor, const char *help);

// where dumps are going, shown on the waiting and main screens
void ui_show_storage(const char *device_name, const char *dump_dir);
void ui_show_waiting_help(void);
void ui_show_main_menu(void);
void ui_show_cart_info(const gba_cart_info *cart);
void ui_show_cart_menu(const gba_cart_info *cart);
void ui_rom_progress(u32 bytes_done, u32 bytes_total);

#endif
