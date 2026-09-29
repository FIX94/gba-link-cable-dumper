/*
 * Copyright (C) 2016 FIX94
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */
#include <gccore.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include "version.h"
#include "ui/ui.h"

// text console front end

void ui_init(void)
{
	void *xfb = NULL;
	GXRModeObj *rmode = NULL;
	VIDEO_Init();
	rmode = VIDEO_GetPreferredMode(NULL);
	xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
	VIDEO_Configure(rmode);
	VIDEO_SetNextFramebuffer(xfb);
	VIDEO_SetBlack(FALSE);
	VIDEO_Flush();
	VIDEO_WaitVSync();
	if(rmode->viTVMode&VI_NON_INTERLACE) VIDEO_WaitVSync();
	int x = 24, y = 32, w, h;
	w = rmode->fbWidth - (32);
	h = rmode->xfbHeight - (48);
	CON_InitEx(rmode, x, y, w, h);
	VIDEO_ClearFrameBuffer(rmode, xfb, COLOR_BLACK);
}

void ui_frame(void)
{
	VIDEO_WaitVSync();
}

void ui_clear(void)
{
	printf("\x1b[2J");
	printf("\x1b[37m");
	printf("GBA Link Cable Dumper " APP_VERSION " by FIX94\n");
	printf("Save Support based on SendSave by Chishm\n");
	printf("GBA BIOS Dumper by Dark Fader\n \n");
}

void ui_status(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);
}

void ui_warn(const char *msg)
{
	puts(msg);
	VIDEO_WaitVSync();
	VIDEO_WaitVSync();
	sleep(2);
}

void ui_fatal(const char *msg)
{
	puts(msg);
	VIDEO_WaitVSync();
	VIDEO_WaitVSync();
	sleep(5);
	exit(0);
}

void ui_exit(void)
{
	printf("Start pressed, exit\n");
	VIDEO_WaitVSync();
	VIDEO_WaitVSync();
	exit(0);
}

// rows available for menu items below the title
#define MENU_ROWS 14

void ui_draw_menu(const char *title, const ui_menu_item *items, int count, int cursor, const char *help)
{
	ui_clear();
	printf("%s\n \n", title);
	int first = 0;
	if(count > MENU_ROWS)
	{
		first = cursor - MENU_ROWS/2;
		if(first < 0) first = 0;
		if(first > count - MENU_ROWS) first = count - MENU_ROWS;
	}
	printf(first > 0 ? "   ...\n" : " \n");
	int i;
	for(i = first; i < count && i < first + MENU_ROWS; i++)
	{
		//highlight the cursor line in yellow
		printf(i == cursor ? "\x1b[33m > " : "   ");
		if(items[i].value)
			printf("%s: %s", items[i].label, items[i].value);
		else
			printf("%s", items[i].label);
		printf(i == cursor ? "\x1b[37m\n" : "\n");
	}
	printf(i < count ? "   ...\n" : " \n");
	if(help)
		printf(" \n%s\n", help);
}

void ui_show_storage(const char *device_name, const char *dump_dir)
{
	printf("Saving to %s (%s)\n \n", dump_dir, device_name);
}

void ui_show_waiting_help(void)
{
	printf("Press X for settings, Start to exit.\n \n");
}

void ui_show_main_menu(void)
{
	printf("Press A once you have a GBA Game inserted.\n");
	printf("Press Y to backup the GBA BIOS.\n");
	printf("Press X for settings.\n");
	printf("You can also use the buttons shown on the GBA.\n \n");
}

void ui_show_cart_info(const gba_cart_info *cart)
{
	printf("Game Name: %.12s\n",GBA_CART_TITLE(cart));
	printf("Game ID: %.4s\n",GBA_CART_GAME_CODE(cart));
	printf("Company ID: %.2s\n",GBA_CART_MAKER_CODE(cart));
	printf("ROM Size: %02.02f MB\n",((float)(cart->rom_size/1024))/1024.f);
	if(cart->save_size > 0)
		printf("Save Size: %02.02f KB\n \n",((float)(cart->save_size))/1024.f);
	else
		printf("No Save File\n \n");
}

void ui_show_cart_menu(const gba_cart_info *cart)
{
	printf("Press A to dump this game, it will take about %i minutes.\n",cart->rom_size/1024/1024*3/2);
	printf("Press B if you want to cancel dumping this game.\n");
	if(cart->save_size > 0)
	{
		printf("Press Y to backup this save file.\n");
		printf("Press X to restore this save file.\n");
		printf("Press Z to clear the save file on the GBA Cartridge.\n\n");
	}
	else
		printf("\n");
}

void ui_rom_progress(u32 bytes_done, u32 bytes_total)
{
	printf("\r%02.02f MB done",(float)(bytes_done/1024)/1024.f);
}
