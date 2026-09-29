/*
 * Host tests for the GBA payload screen (gba/source/screen.c).
 * Builds the real source against tests/gba_stub, with VRAM and the palette
 * as plain arrays. Checks the progress bar maths and, when run with an
 * output folder, dumps each screen for render_gba_screen.py.
 */
#include <string.h>
#include <stdlib.h>
#include "gba.h"
#include "screen.h"

u8 fake_vram[0x18000];
u16 fake_pal[256];
static const char *font_path = NULL;
static const char *out_dir = NULL;
static int fails = 0;
#define CHECK(c) do { if(!(c)) { printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #c); fails++; } else printf("ok   %s\n", #c); } while(0)

// mirrors libgba's consoleDemoInit: 1bpp font -> 4bpp tiles in charblock 0,
// map in screenblock 4, palette 15 colour 1 white
void consoleDemoInit(void)
{
	u8 font[2048] = {0};
	FILE *f = font_path ? fopen(font_path, "rb") : NULL;
	if(f) { fread(font, 1, sizeof(font), f); fclose(f); }
	for(int c = 0; c < 256; c++) for(int y = 0; y < 8; y++) {
		u32 row = 0; u8 b = font[c*8+y];
		for(int x = 0; x < 8; x++) if(b & (0x80>>x)) row |= 1u << (x*4);
		memcpy(fake_vram + c*32 + y*4, &row, 4);
	}
	u16 *map = (u16*)(fake_vram + 4*0x800);
	for(int i = 0; i < 32*32; i++) map[i] = 0x20;
	fake_pal[0] = 0x51A7; fake_pal[15*16+1] = 0x7FFF;
}

static void dump(const char *name)
{
	if(!out_dir) return;
	char path[512]; snprintf(path, sizeof(path), "%s/%s", out_dir, name);
	FILE *f = fopen(path, "wb"); if(!f) return;
	fwrite(fake_vram, 1, 0x3000, f); fwrite(fake_pal, 2, 256, f); fclose(f);
}

static int pct_of(const progress_t *p) { return p->pct[0]*100 + p->pct[1]*10 + p->pct[2]; }

// steps like the payload's transfer loops and checks the display is never
// ahead of the real progress, never goes backwards, and only shows 100 when finished
static void check_size(u32 total)
{
	progress_t p;
	progress_start(&p, total);
	int last = 0, lastcells = 0, ok = 1;
	for(u32 i = 0; i < total; i += 4) {
		progress_update(&p, i+4);
		int pct = pct_of(&p);
		if(pct < last || pct > 99 || p.cells < lastcells || p.cells > 26) ok = 0;
		if((unsigned long long)pct * total > (unsigned long long)(i+4) * 100) ok = 0;
		if((unsigned long long)p.cells * total > (unsigned long long)(i+4) * 26) ok = 0;
		last = pct; lastcells = p.cells;
	}
	printf("     size %u: %d%% and %d cells before finish\n", total, last, p.cells);
	CHECK(ok);
	progress_finish(&p);
	CHECK(pct_of(&p) == 100 && p.cells == 26);
}

int main(int argc, char *argv[])
{
	if(argc > 1) font_path = argv[1];
	if(argc > 2) out_dir = argv[2];
	u8 header[0xC0] = {0};
	memcpy(header+0xA0, "POKEMON SAPP", 12); memcpy(header+0xAC, "AXPE", 4); memcpy(header+0xB0, "01", 2); header[0xBC] = 1;
	progress_t p;

	screen_init(); screen_controls("A: read cartridge", "SELECT: dump BIOS"); dump("1_ready.bin");
	screen_cart(header, 16<<20, 0x20000); screen_status("Choose on the GBA or TV");
	screen_controls("A: dump ROM    B: cancel", "R:backup L:restore SEL:clear"); dump("2_cart.bin");
	screen_status("Press SELECT again to clear"); dump("2b_confirm.bin");
	screen_controls(NULL, NULL);
	u16 *map = (u16*)(fake_vram + 4*0x800);
	CHECK((map[5*32+2] & 0xFF) == 'T' && (map[5*32+9] & 0xFF) == 'P');

	u32 total = 16<<20, i;
	screen_status("Dumping ROM...");
	progress_start(&p, total);
	for(i = 0; i < total*45/100; i += 4) progress_update(&p, i+4);
	dump("3_rom_45.bin");
	CHECK(pct_of(&p) == 44 && p.cells == 11);
	for(; i < total; i += 4) progress_update(&p, i+4);
	progress_finish(&p); screen_status_line("ROM dumped!"); dump("4_rom_done.bin");
	CHECK((map[14*32+2] & 0xFF) == 1 && (map[14*32+27] & 0xFF) == 1);

	// every save size, the BIOS and the smallest and largest ROM
	u32 sizes[] = { 0x200, 0x2000, 0x4000, 0x8000, 0x10000, 0x20000, 1<<20, 32<<20 };
	for(unsigned k = 0; k < sizeof(sizes)/sizeof(sizes[0]); k++)
		check_size(sizes[k]);

	screen_cart(NULL, -1, 0); screen_status("Ready"); screen_controls("A: read cartridge", "SELECT: dump BIOS"); dump("5_no_cart.bin");
	CHECK((map[14*32+2] & 0xFF) == ' ');

	printf(fails ? "\n%d FAILED\n" : "\nALL PASSED\n", fails);
	return fails != 0;
}
