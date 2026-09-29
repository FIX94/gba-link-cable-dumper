#pragma once
#include "gba_types.h"
#include <stdio.h>
extern u8 fake_vram[0x18000];
extern u16 fake_pal[256];
#define VRAM ((uintptr_t)fake_vram)
#define BG_PALETTE ((u16*)fake_pal)
#define RGB5(r,g,b) ((r)|((g)<<5)|((b)<<10))
#define siprintf sprintf
void consoleDemoInit(void);
