/*
 * Copyright (C) 2016 FIX94
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */
#include <gccore.h>
#include <unistd.h>
#include "link/si_link.h"
#include "link/gba_protocol.h"

bool gba_is_ready(void)
{
	return si_link_recv() == 0;
}

bool gba_read_cart_info(gba_cart_info *info)
{
	int i;
	s32 gbasize = 0;
	while(gbasize == 0)
		gbasize = __builtin_bswap32(si_link_recv());
	si_link_send(0); //got gbasize
	u32 savesize = __builtin_bswap32(si_link_recv());
	si_link_send(0); //got savesize
	info->rom_size = gbasize;
	info->save_size = savesize;
	if(gbasize == -1)
		return false;
	//get rom header
	for(i = 0; i < GBA_HEADER_SIZE; i+=4)
		*(vu32*)(info->header+i) = si_link_recv();
	return true;
}

void gba_send_command(u32 cmd)
{
	si_link_send(cmd);
}

bool gba_dump_rom(u32 rom_size, u8 *buf, u32 chunk_size,
	gba_chunk_cb chunk, void *user, gba_progress_cb progress)
{
	u32 bytes_read = 0;
	u32 remaining = rom_size;
	while(remaining > 0)
	{
		u32 toread = (remaining > chunk_size ? chunk_size : remaining);
		u32 j;
		for(j = 0; j < toread; j+=4)
		{
			*(vu32*)(buf+j) = si_link_recv();
			bytes_read+=4;
			if((bytes_read&0xFFFF) == 0 && progress)
				progress(bytes_read, rom_size);
		}
		if(!chunk(buf, toread, user))
			return false;
		remaining -= toread;
	}
	return true;
}

void gba_wait_save_ready(u32 save_size)
{
	u32 readval = 0;
	while(readval != save_size)
		readval = __builtin_bswap32(si_link_recv());
}

void gba_recv_save(u8 *buf, u32 save_size)
{
	u32 i;
	for(i = 0; i < save_size; i+=4)
		*(vu32*)(buf+i) = si_link_recv();
}

void gba_send_save(const u8 *buf, u32 save_size)
{
	u32 i;
	for(i = 0; i < save_size; i+=4)
		si_link_send(__builtin_bswap32(*(vu32*)(buf+i)));
}

void gba_wait_save_written(void)
{
	while(si_link_recv() != 0)
		VIDEO_WaitVSync();
}

void gba_ack(void)
{
	si_link_send(0);
}

void gba_start_bios_dump(void)
{
	si_link_send(GBA_CMD_DUMP_BIOS);
	//the gba might still be in a loop itself
	sleep(1);
}

void gba_recv_bios(u8 *buf)
{
	int i;
	for(i = 0; i < GBA_BIOS_SIZE; i+=4)
		*(vu32*)(buf+i) = si_link_recv();
}
