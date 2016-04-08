
#include <gba_console.h>
#include <gba_video.h>
#include <gba_interrupt.h>
#include <gba_systemcalls.h>
#include <gba_input.h>
#include <gba_sio.h>
#include <stdio.h>
#include <stdlib.h>
#include "libSave.h"

u8 save_data[0x20000] __attribute__ ((section (".sbss")));

s32 getGameSize(void)
{
	if(*(vu32*)(0x08000004) != 0x51AEFF24)
		return -1;
	s32 i;
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
#define JOY_WRITE 2
#define JOY_READ 4
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
	REG_HS_CTRL |= (JOY_WRITE|JOY_READ);
	u32 prevIrqMask = REG_IME;
	while (1) {
		if((REG_HS_CTRL&JOY_READ))
		{
			irqDisable(IRQ_VBLANK);
			REG_IME = 0;
			REG_HS_CTRL |= (JOY_WRITE|JOY_READ);
			s32 gamesize = getGameSize();
			u32 savesize = SaveSize(save_data,gamesize);
			REG_JOYTR = gamesize;
			//wait for a cmd receive for safety
			while((REG_HS_CTRL&JOY_WRITE) == 0) ;
			REG_HS_CTRL |= (JOY_WRITE|JOY_READ);
			REG_JOYTR = savesize;
			//wait for a cmd receive for safety
			while((REG_HS_CTRL&JOY_WRITE) == 0) ;
			REG_HS_CTRL |= (JOY_WRITE|JOY_READ);
			if(gamesize == -1)
			{
				REG_JOYTR = 0;
				REG_IME = prevIrqMask;
				irqEnable(IRQ_VBLANK);
				continue; //nothing to read
			}
			//game in, send header
			for(i = 0; i < 0xC0; i+=4)
			{
				REG_JOYTR = *(vu32*)(0x08000000+i);
				while((REG_HS_CTRL&JOY_READ) == 0) ;
				REG_HS_CTRL |= (JOY_WRITE|JOY_READ);
			}
			REG_JOYTR = 0;
			//wait for other side to choose
			while((REG_HS_CTRL&JOY_WRITE) == 0) ;
			REG_HS_CTRL |= (JOY_WRITE|JOY_READ);
			u32 choseval = REG_JOYRE;
			if(choseval == 0)
			{
				REG_JOYTR = 0;
				REG_IME = prevIrqMask;
				irqEnable(IRQ_VBLANK);
				continue; //nothing to read
			}
			else if(choseval == 1)
			{
				//dump the game
				for(i = 0; i < gamesize; i+=4)
				{
					REG_JOYTR = *(vu32*)(0x08000000+i);
					while((REG_HS_CTRL&JOY_READ) == 0) ;
					REG_HS_CTRL |= (JOY_WRITE|JOY_READ);
				}
			}
			else if(choseval == 2)
			{
				//backup save
				switch (savesize){
				case 0x200:
					GetSave_EEPROM_512B(save_data);
					break;
				case 0x2000:
					GetSave_EEPROM_8KB(save_data);
					break;
				case 0x8000:
					GetSave_SRAM_32KB(save_data);
					break;
				case 0x10000:
					GetSave_FLASH_64KB(save_data);
					break;
				case 0x20000:
					GetSave_FLASH_128KB(save_data);
					break;
				default:
					break;
				}
				REG_JOYTR = savesize;
				//wait for a cmd receive for safety
				while((REG_HS_CTRL&JOY_WRITE) == 0) ;
				REG_HS_CTRL |= (JOY_WRITE|JOY_READ);
				//send the save
				for(i = 0; i < savesize; i+=4)
				{
					REG_JOYTR = *(vu32*)(save_data+i);
					while((REG_HS_CTRL&JOY_READ) == 0) ;
					REG_HS_CTRL |= (JOY_WRITE|JOY_READ);
				}
			}
			else if(choseval == 3)
			{
				REG_JOYTR = savesize;
				//receive the save
				for(i = 0; i < savesize; i+=4)
				{
					while((REG_HS_CTRL&JOY_WRITE) == 0) ;
					REG_HS_CTRL |= (JOY_WRITE|JOY_READ);
					*(vu32*)(save_data+i) = REG_JOYRE;
				}
				//write it
				switch (savesize){
				case 0x200:
					PutSave_EEPROM_512B(save_data);
					break;
				case 0x2000:
					PutSave_EEPROM_8KB(save_data);
					break;
				case 0x8000:
					PutSave_SRAM_32KB(save_data);
					break;
				case 0x10000:
					PutSave_FLASH_64KB(save_data);
					break;
				case 0x20000:
					PutSave_FLASH_128KB(save_data);
					break;
				default:
					break;
				}
				REG_JOYTR = 0;
				//wait for a cmd receive for safety
				while((REG_HS_CTRL&JOY_WRITE) == 0) ;
				REG_HS_CTRL |= (JOY_WRITE|JOY_READ);
			}
			REG_JOYTR = 0;
			REG_IME = prevIrqMask;
			irqEnable(IRQ_VBLANK);
		}
		Halt();
	}
}


