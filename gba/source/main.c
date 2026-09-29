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
#include "protocol.h"
#include "version.h"
#include "screen.h"

#define	REG_WAITCNT *(vu16 *)(REG_BASE + 0x204)
#define JOY_WRITE 2
#define JOY_READ 4
#define JOY_RW 6

u8 save_data[GBA_MAX_SAVE_SIZE] __attribute__ ((section (".sbss")));

//---------------------------------------------------------------------------------
// requests posted by pressing a button on the GBA, see common/protocol.h
//---------------------------------------------------------------------------------
static bool request_posted = false;

static void post_request(u32 code, const char *msg)
{
	REG_JOYTR = GBA_REQUEST_MAGIC | code;
	REG_JSTAT |= GBA_JSTAT_REQUEST;
	request_posted = true;
	screen_status(msg);
	screen_controls(NULL, NULL);
}

// takes the request down, JOYTR is reset first because the gc reads it
// again as soon as it sees the status bit clear
static void clear_request(void)
{
	REG_JOYTR = 0;
	REG_JSTAT &= ~GBA_JSTAT_REQUEST;
	request_posted = false;
}

// the gc read our request, clear only the read flag so a command
// that arrives at the same time is not lost
static void ack_request(void)
{
	REG_JOYTR = 0;
	REG_HS_CTRL = JOY_READ;
	REG_JSTAT &= ~GBA_JSTAT_REQUEST;
	request_posted = false;
}

static void show_idle_controls(void)
{
	screen_controls("A: read cartridge", "SELECT: dump BIOS");
}

static void show_cart_controls(u32 savesize)
{
	if(savesize > 0)
		screen_controls("A: dump ROM    B: cancel", "R:backup L:restore SEL:clear");
	else
		screen_controls("A: dump ROM    B: cancel", NULL);
}

// cart screen buttons, destructive ones need a second press
// returns true once a choice was posted
static bool handle_cart_keys(u32 savesize, u16 *confirm)
{
	scanKeys();
	u16 down = keysDown();
	if(!down)
		return false;
	if(down & KEY_A)
		post_request(GBA_CMD_DUMP_ROM, "Starting ROM dump...");
	else if(down & KEY_B)
		post_request(GBA_CMD_NONE, "Cancelling...");
	else if(savesize > 0 && (down & KEY_R))
		post_request(GBA_CMD_BACKUP_SAVE, "Starting save backup...");
	else if(savesize > 0 && (down & (KEY_L|KEY_SELECT)))
	{
		u16 key = (down & KEY_L) ? KEY_L : KEY_SELECT;
		if(*confirm == key)
		{
			*confirm = 0;
			if(key == KEY_L)
				post_request(GBA_CMD_RESTORE_SAVE, "Starting save restore...");
			else
				post_request(GBA_CMD_CLEAR_SAVE, "Starting save clear...");
		}
		else
		{
			*confirm = key;
			screen_status(key == KEY_L ? "Press L again to restore" : "Press SELECT again to clear");
			return false;
		}
	}
	else
		return false;
	return request_posted;
}

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

	screen_init();
	REG_JOYTR = 0;
	//the bios may have left the general purpose flags set
	REG_JSTAT = 0;
	show_idle_controls();
	u32 i;
	progress_t progress;
	// disable this, needs power
	SNDSTAT = 0;
	SNDBIAS = 0;
	// Set up waitstates for EEPROM access etc. 
	REG_WAITCNT = 0x0317;
	//clear out previous messages
	REG_HS_CTRL |= JOY_RW;
	while (1) {
		if((REG_HS_CTRL&JOY_READ) && request_posted)
		{
			//the gc took our request, it acts on it next and may refuse it,
			//so go back to ready until its command arrives
			ack_request();
			screen_status("Ready");
			show_idle_controls();
		}
		else if(REG_HS_CTRL&JOY_READ)
		{
			REG_HS_CTRL |= JOY_RW;
			screen_status("Reading cartridge...");
			screen_controls(NULL, NULL);
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
				screen_cart(NULL, -1, 0);
				screen_status("Ready");
				show_idle_controls();
				continue; //nothing to read
			}
			//game in, send header
			for(i = 0; i < GBA_HEADER_SIZE; i+=4)
			{
				REG_JOYTR = *(vu32*)(0x08000000+i);
				while((REG_HS_CTRL&JOY_READ) == 0) ;
				REG_HS_CTRL |= JOY_RW;
			}
			REG_JOYTR = 0;
			//the gc side waits for the user now, so there is time to draw
			screen_cart((const u8*)0x08000000, gamesize, savesize);
			screen_status("Choose on the GBA or TV");
			show_cart_controls(savesize);
			//wait for other side to choose, the choice can also be made here
			u16 confirm = 0;
			while((REG_HS_CTRL&JOY_WRITE) == 0)
			{
				if(request_posted)
				{
					if(REG_HS_CTRL&JOY_READ)
						ack_request();
				}
				else
					handle_cart_keys(savesize, &confirm);
			}
			//chosen on the gc at the same time, drop ours
			if(request_posted)
				clear_request();
			REG_HS_CTRL |= JOY_RW;
			screen_controls(NULL, NULL);
			u32 choseval = REG_JOYRE;
			if(choseval == GBA_CMD_NONE)
			{
				REG_JOYTR = 0;
				screen_status("Ready");
				show_idle_controls();
				continue; //nothing to read
			}
			else if(choseval == GBA_CMD_DUMP_ROM)
			{
				//the gc side waits a second before reading, set up the screen now
				screen_status("Dumping ROM...");
				progress_start(&progress, gamesize);
				//disable interrupts
				u32 prevIrqMask = REG_IME;
				REG_IME = 0;
				//dump the game
				for(i = 0; i < gamesize; i+=4)
				{
					REG_JOYTR = *(vu32*)(0x08000000+i);
					//the word is loaded, update the bar while the gc reads it
					progress_update(&progress, i+4);
					while((REG_HS_CTRL&JOY_READ) == 0) ;
					REG_HS_CTRL |= JOY_RW;
				}
				//restore interrupts
				REG_IME = prevIrqMask;
				progress_finish(&progress);
				screen_status_line("ROM dumped!");
			}
			else if(choseval == GBA_CMD_BACKUP_SAVE)
			{
				screen_status("Reading save...");
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
				//the gc side reads straight after the next handshake, draw first
				screen_status("Sending save...");
				progress_start(&progress, savesize);
				//say gc side we read it
				REG_JOYTR = savesize;
				//wait for a cmd receive for safety
				while((REG_HS_CTRL&JOY_WRITE) == 0) ;
				REG_HS_CTRL |= JOY_RW;
				//send the save
				for(i = 0; i < savesize; i+=4)
				{
					REG_JOYTR = *(vu32*)(save_data+i);
					progress_update(&progress, i+4);
					while((REG_HS_CTRL&JOY_READ) == 0) ;
					REG_HS_CTRL |= JOY_RW;
				}
				progress_finish(&progress);
				screen_status_line("Save backed up!");
			}
			else if(choseval == GBA_CMD_RESTORE_SAVE || choseval == GBA_CMD_CLEAR_SAVE)
			{
				//the gc side sends straight after seeing the save size, draw first
				if(choseval == GBA_CMD_RESTORE_SAVE)
				{
					screen_status("Receiving save...");
					progress_start(&progress, savesize);
				}
				else
					screen_status("Clearing save...");
				REG_JOYTR = savesize;
				if(choseval == GBA_CMD_RESTORE_SAVE)
				{
					//receive the save
					for(i = 0; i < savesize; i+=4)
					{
						while((REG_HS_CTRL&JOY_WRITE) == 0) ;
						REG_HS_CTRL |= JOY_RW;
						*(vu32*)(save_data+i) = REG_JOYRE;
						progress_update(&progress, i+4);
					}
					progress_finish(&progress);
					//the gc side waits for us to report back, no rush now
					screen_status_line("Writing to cartridge...");
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
				screen_status_line(choseval == GBA_CMD_RESTORE_SAVE ? "Save restored!" : "Save cleared!");
				//say gc side we're done
				REG_JOYTR = 0;
				//wait for a cmd receive for safety
				while((REG_HS_CTRL&JOY_WRITE) == 0) ;
				REG_HS_CTRL |= JOY_RW;
			}
			REG_JOYTR = 0;
			show_idle_controls();
		}
		else if(REG_HS_CTRL&JOY_WRITE)
		{
			//a command from the gc wins over a request we posted meanwhile
			if(request_posted)
				clear_request();
			REG_HS_CTRL |= JOY_RW;
			u32 choseval = REG_JOYRE;
			if(choseval == GBA_CMD_DUMP_BIOS)
			{
				//the gc side waits a second before reading, set up the screen now
				screen_status("Dumping BIOS...");
				progress_start(&progress, GBA_BIOS_SIZE);
				//disable interrupts
				u32 prevIrqMask = REG_IME;
				REG_IME = 0;
				//dump BIOS
				for (i = 0; i < GBA_BIOS_SIZE; i+=4)
				{
					// the lower bits are inaccurate, so just get it four times :)
					u32 a = MidiKey2Freq((WaveData *)(i-4), 180-12, 0) * 2;
					u32 b = MidiKey2Freq((WaveData *)(i-3), 180-12, 0) * 2;
					u32 c = MidiKey2Freq((WaveData *)(i-2), 180-12, 0) * 2;
					u32 d = MidiKey2Freq((WaveData *)(i-1), 180-12, 0) * 2;
					REG_JOYTR = ((a>>24<<24) | (d>>24<<16) | (c>>24<<8) | (b>>24));
					progress_update(&progress, i+4);
					while((REG_HS_CTRL&JOY_READ) == 0) ;
					REG_HS_CTRL |= JOY_RW;
				}
				//restore interrupts
				REG_IME = prevIrqMask;
				progress_finish(&progress);
				screen_status_line("BIOS dumped!");
			}
			REG_JOYTR = 0;
			show_idle_controls();
		}
		else if(!request_posted)
		{
			scanKeys();
			u16 down = keysDown();
			if(down & KEY_A)
				post_request(GBA_REQ_READ_CART, "Waiting for the GameCube...");
			else if(down & KEY_SELECT)
				post_request(GBA_REQ_DUMP_BIOS, "Waiting for the GameCube...");
		}
		Halt();
	}
}


