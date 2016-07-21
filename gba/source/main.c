/*
 * Copyright (C) 2016 FIX94
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */
#include <gba.h>
#include <stdio.h>
#include <stdlib.h>
#include "libSave.h"

#define	REG_WAITCNT *(vu16 *)(REG_BASE + 0x204)
#define JOY_WRITE 2
#define JOY_READ 4
#define JOY_RW 6

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
	iprintf("\x1b[9;2HGBA Link Cable Dumper v1.6\n");
	iprintf("\x1b[10;4HPlease look at the TV\n");
	// disable this, needs power
	SNDSTAT = 0;
	SNDBIAS = 0;
	// Set up waitstates for EEPROM access etc. 
	REG_WAITCNT = 0x0317;
	//clear out previous messages
	REG_HS_CTRL |= JOY_RW;
	while (1) {
		if(REG_HS_CTRL&JOY_READ)
		{
			REG_HS_CTRL |= JOY_RW;
			s32 gamesize = getGameSize();
			u32 savesize = SaveSize(save_data,gamesize);
			REG_JOYTR = gamesize;
			//wait for a cmd receive for safety
			while((REG_HS_CTRL&JOY_WRITE) == 0) ;
			REG_HS_CTRL |= JOY_RW;
			REG_JOYTR = savesize;
			//wait for a cmd receive for safety
			while((REG_HS_CTRL&JOY_WRITE) == 0) ;
			REG_HS_CTRL |= JOY_RW;
			if(gamesize == -1)
			{
				REG_JOYTR = 0;
				continue; //nothing to read
			}
			//game in, send header
			for(i = 0; i < 0xC0; i+=4)
			{
				REG_JOYTR = *(vu32*)(0x08000000+i);
				while((REG_HS_CTRL&JOY_READ) == 0) ;
				REG_HS_CTRL |= JOY_RW;
			}
			REG_JOYTR = 0;
			//wait for other side to choose
			while((REG_HS_CTRL&JOY_WRITE) == 0) ;
			REG_HS_CTRL |= JOY_RW;
			u32 choseval = REG_JOYRE;
			if(choseval == 0)
			{
				REG_JOYTR = 0;
				continue; //nothing to read
			}
			else if(choseval == 1)
			{
				//disable interrupts
				u32 prevIrqMask = REG_IME;
				REG_IME = 0;
				//dump the game
				for(i = 0; i < gamesize; i+=4)
				{
					REG_JOYTR = *(vu32*)(0x08000000+i);
					while((REG_HS_CTRL&JOY_READ) == 0) ;
					REG_HS_CTRL |= JOY_RW;
				}
				//restore interrupts
				REG_IME = prevIrqMask;
			}
			else if(choseval == 2)
			{
				//disable interrupts
				u32 prevIrqMask = REG_IME;
				REG_IME = 0;
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
				//restore interrupts
				REG_IME = prevIrqMask;
				//say gc side we read it
				REG_JOYTR = savesize;
				//wait for a cmd receive for safety
				while((REG_HS_CTRL&JOY_WRITE) == 0) ;
				REG_HS_CTRL |= JOY_RW;
				//send the save
				for(i = 0; i < savesize; i+=4)
				{
					REG_JOYTR = *(vu32*)(save_data+i);
					while((REG_HS_CTRL&JOY_READ) == 0) ;
					REG_HS_CTRL |= JOY_RW;
				}
			}
			else if(choseval == 3 || choseval == 4)
			{
				REG_JOYTR = savesize;
				if(choseval == 3)
				{
					//receive the save
					for(i = 0; i < savesize; i+=4)
					{
						while((REG_HS_CTRL&JOY_WRITE) == 0) ;
						REG_HS_CTRL |= JOY_RW;
						*(vu32*)(save_data+i) = REG_JOYRE;
					}
				}
				else
				{
					//clear the save
					for(i = 0; i < savesize; i+=4)
						*(vu32*)(save_data+i) = 0;
				}
				//disable interrupts
				u32 prevIrqMask = REG_IME;
				REG_IME = 0;
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
				//restore interrupts
				REG_IME = prevIrqMask;
				//say gc side we're done
				REG_JOYTR = 0;
				//wait for a cmd receive for safety
				while((REG_HS_CTRL&JOY_WRITE) == 0) ;
				REG_HS_CTRL |= JOY_RW;
			}
			REG_JOYTR = 0;
		}
		else if(REG_HS_CTRL&JOY_WRITE)
		{
			REG_HS_CTRL |= JOY_RW;
			u32 choseval = REG_JOYRE;
			if(choseval == 5)
			{
				//disable interrupts
				u32 prevIrqMask = REG_IME;
				REG_IME = 0;
				//dump BIOS
				for (i = 0; i < 0x4000; i+=4)
				{
					// the lower bits are inaccurate, so just get it four times :)
					u32 a = MidiKey2Freq((WaveData *)(i-4), 180-12, 0) * 2;
					u32 b = MidiKey2Freq((WaveData *)(i-3), 180-12, 0) * 2;
					u32 c = MidiKey2Freq((WaveData *)(i-2), 180-12, 0) * 2;
					u32 d = MidiKey2Freq((WaveData *)(i-1), 180-12, 0) * 2;
					REG_JOYTR = ((a>>24<<24) | (d>>24<<16) | (c>>24<<8) | (b>>24));
					while((REG_HS_CTRL&JOY_READ) == 0) ;
					REG_HS_CTRL |= JOY_RW;
				}
				//restore interrupts
				REG_IME = prevIrqMask;
			}
			REG_JOYTR = 0;
		}
		Halt();
	}
}


