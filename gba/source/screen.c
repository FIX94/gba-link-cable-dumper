/*
 * Copyright (C) 2016 FIX94
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */
#include <gba.h>
#include <stdio.h>
#include <string.h>
#include "screen.h"
#include "version.h"

// consoleDemoInit puts the map in screenblock 4 and uses palette 15,
// a map entry is (palette << 12) | character
#define CON_MAP			((vu16*)(VRAM + 4*0x800))
#define MAP_ENTRY(c)	((15 << 12) | (c))
#define PAL15			((vu16*)(BG_PALETTE + 15*16))

// the font never prints control characters 1 and 2, reuse them for the bar
#define TILE_FILLED		1
#define TILE_EMPTY		2
#define TILE_ADDR(t)	((vu32*)(VRAM + (t)*32))

#define COLOR_GREY		RGB5(10,10,10)
#define COLOR_GREEN		RGB5(8,28,8)

#define SCREEN_COLS		30
#define ROW_INFO		5
#define ROW_STATUS		12
#define ROW_BAR			14
#define ROW_PCT			15
#define BAR_COL			2
#define BAR_CELLS		26
#define PCT_COL			13

static inline void put_char(int col, int row, char c)
{
	CON_MAP[row*32 + col] = MAP_ENTRY(c);
}

static void clear_row(int row)
{
	int col;
	for(col = 0; col < SCREEN_COLS; col++)
		put_char(col, row, ' ');
}

// writes text straight into the map, cut off at the right edge
static void put_string(int col, int row, const char *str)
{
	for(; *str && col < SCREEN_COLS; str++, col++)
		put_char(col, row, *str);
}

static void put_line(int row, const char *str)
{
	clear_row(row);
	put_string(2, row, str);
}

// a filled 4bpp tile with a blank top and bottom row so the bar looks slimmer
static void make_bar_tile(int tile, u32 color_index)
{
	u32 fill = color_index * 0x11111111;
	vu32 *dst = TILE_ADDR(tile);
	int y;
	for(y = 0; y < 8; y++)
		dst[y] = (y == 0 || y == 7) ? 0 : fill;
}

void screen_init(void)
{
	consoleDemoInit();
	PAL15[2] = COLOR_GREY;
	PAL15[3] = COLOR_GREEN;
	make_bar_tile(TILE_FILLED, 3);
	make_bar_tile(TILE_EMPTY, 2);
	put_line(1, "GBA Link Cable Dumper " APP_VERSION);
	put_line(2, "--------------------------");
	put_line(18, "Use the GameCube controller");
	screen_status("Ready, look at the TV");
}

static const char *save_type_name(u32 savesize)
{
	switch(savesize)
	{
		case 0x200:		return "EEPROM 512B";
		case 0x2000:	return "EEPROM 8KB";
		case 0x8000:	return "SRAM 32KB";
		case 0x10000:	return "Flash 64KB";
		case 0x20000:	return "Flash 128KB";
		default:		return "None";
	}
}

// copies n header chars, replacing anything unprintable
static void header_text(char *out, const u8 *src, int n)
{
	int i;
	for(i = 0; i < n; i++)
		out[i] = (src[i] >= 0x20 && src[i] < 0x7F) ? src[i] : ' ';
	out[n] = '\0';
}

void screen_cart(const u8 *header, s32 gamesize, u32 savesize)
{
	int row;
	for(row = ROW_INFO; row < ROW_INFO+5; row++)
		clear_row(row);
	if(gamesize < 0)
	{
		put_line(ROW_INFO, "No cartridge found");
		put_line(ROW_INFO+2, "Check the cartridge");
		return;
	}
	char title[13], code[5], maker[3];
	header_text(title, header+0xA0, 12);
	header_text(code, header+0xAC, 4);
	header_text(maker, header+0xB0, 2);
	char line[SCREEN_COLS+1];
	siprintf(line, "Title: %s", title);
	put_line(ROW_INFO, line);
	siprintf(line, "Code:  %s%s  (v%d)", code, maker, header[0xBC]);
	put_line(ROW_INFO+1, line);
	siprintf(line, "ROM:   %d MB", (int)(gamesize >> 20));
	put_line(ROW_INFO+2, line);
	siprintf(line, "Save:  %s", save_type_name(savesize));
	put_line(ROW_INFO+3, line);
}

void screen_status(const char *msg)
{
	clear_row(ROW_BAR);
	clear_row(ROW_PCT);
	screen_status_line(msg);
}

void screen_status_line(const char *msg)
{
	put_line(ROW_STATUS, msg);
}

static void draw_pct(const progress_t *p)
{
	put_char(PCT_COL,   ROW_PCT, p->pct[0] ? '0'+p->pct[0] : ' ');
	put_char(PCT_COL+1, ROW_PCT, (p->pct[0] || p->pct[1]) ? '0'+p->pct[1] : ' ');
	put_char(PCT_COL+2, ROW_PCT, '0'+p->pct[2]);
	put_char(PCT_COL+3, ROW_PCT, '%');
}

void progress_start(progress_t *p, u32 total)
{
	int i;
	p->total = total;
	p->cells = 0;
	p->pct[0] = p->pct[1] = p->pct[2] = 0;
	//the only divisions, done before the transfer starts
	//rounded up so the bar never shows more than has really been sent
	p->cell_step = (total + BAR_CELLS - 1) / BAR_CELLS;
	p->pct_step = (total + 99) / 100;
	if(p->cell_step == 0) p->cell_step = 1;
	if(p->pct_step == 0) p->pct_step = 1;
	p->cell_next = p->cell_step;
	p->pct_next = p->pct_step;
	for(i = 0; i < BAR_CELLS; i++)
		put_char(BAR_COL+i, ROW_BAR, TILE_EMPTY);
	draw_pct(p);
}

void progress_update_slow(progress_t *p, u32 done)
{
	while(done >= p->cell_next && p->cells < BAR_CELLS)
	{
		put_char(BAR_COL + p->cells, ROW_BAR, TILE_FILLED);
		p->cells++;
		p->cell_next += p->cell_step;
	}
	bool changed = false;
	//stops at 99, only progress_finish shows 100
	while(done >= p->pct_next && !(p->pct[1] == 9 && p->pct[2] == 9))
	{
		//count up in decimal digits to avoid dividing
		if(++p->pct[2] == 10)
		{
			p->pct[2] = 0;
			p->pct[1]++;
		}
		p->pct_next += p->pct_step;
		changed = true;
	}
	if(changed)
		draw_pct(p);
	//nothing left to draw, keep the inline check from calling in here again
	if(p->cells >= BAR_CELLS)
		p->cell_next = 0xFFFFFFFF;
	if(p->pct[1] == 9 && p->pct[2] == 9)
		p->pct_next = 0xFFFFFFFF;
}

void progress_finish(progress_t *p)
{
	while(p->cells < BAR_CELLS)
	{
		put_char(BAR_COL + p->cells, ROW_BAR, TILE_FILLED);
		p->cells++;
	}
	p->pct[0] = 1; p->pct[1] = 0; p->pct[2] = 0;
	p->cell_next = p->pct_next = 0xFFFFFFFF;
	draw_pct(p);
}
