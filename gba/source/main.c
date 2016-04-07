
#include <gba_console.h>
#include <gba_video.h>
#include <gba_interrupt.h>
#include <gba_systemcalls.h>
#include <gba_input.h>
#include <gba_sio.h>
#include <stdio.h>
#include <stdlib.h>

u32 getGameSize(void)
{
	if(*(vu32*)(0x08000004) != 0x51AEFF24)
		return 0;
	u32 i;
	for(i = (1<<20); i < (1<<25); i<<=1)
	{
		vu16 *rompos = (vu16*)(0x08000000+i);
		int j;
		bool romend = true;
		for(j = 0; j < 0x1000; j++)
		{
			if(rompos[j] != j)
			{
				romend = false;
				break;
			}
		}
		if(romend) break;
	}
	return i;
}
//---------------------------------------------------------------------------------
// Program entry point
//---------------------------------------------------------------------------------
int main(void) {
//---------------------------------------------------------------------------------

	// the vblank interrupt must be enabled for VBlankIntrWait() to work
	// since the default dispatcher handles the bios flags no vblank handler
	// is required
	irqInit();
	irqEnable(IRQ_VBLANK);

	consoleDemoInit();
	REG_JOYTR = 0;
	// ansi escape sequence to set print co-ordinates
	// /x1b[line;columnH
	u32 i;
	iprintf("\x1b[9;10HROM Dumper\n");
	iprintf("\x1b[10;5HPlease look at the TV\n");
	REG_HS_CTRL |= 6;
	while (1) {
		if((REG_HS_CTRL&4))
		{
			REG_HS_CTRL |= 4;
			u32 gamesize = getGameSize();
			REG_JOYTR = gamesize;
			while((REG_HS_CTRL&4) == 0) ;
			REG_HS_CTRL |= 4;
			if(gamesize == 0)
			{
				REG_JOYTR = 0;
				continue; //nothing to read
			}
			//game in, send header
			for(i = 0; i < 0xC0; i+=4)
			{
				REG_JOYTR = *(vu32*)(0x08000000+i);
				while((REG_HS_CTRL&4) == 0) ;
				REG_HS_CTRL |= 4;
			}
			//wait for other side to choose
			while((REG_HS_CTRL&2) == 0) ;
			REG_HS_CTRL |= 2;
			if(REG_JOYRE == 0)
			{
				REG_JOYTR = 0;
				continue; //nothing to read
			}
			//dump the game
			for(i = 0; i < gamesize; i+=4)
			{
				REG_JOYTR = *(vu32*)(0x08000000+i);
				while((REG_HS_CTRL&4) == 0) ;
				REG_HS_CTRL |= 4;
			}
			REG_JOYTR = 0;
		}
		Halt();
	}
}


