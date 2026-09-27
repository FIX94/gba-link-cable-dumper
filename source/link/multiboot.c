/*
 * Copyright (C) 2016 FIX94
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */
#include <gccore.h>
#include "link/si_link.h"
#include "link/multiboot.h"

static unsigned int docrc(u32 crc, u32 val)
{
	int i;
	for(i = 0; i < 0x20; i++)
	{
		if((crc^val)&1)
		{
			crc>>=1;
			crc^=0xa1c1;
		}
		else
			crc>>=1;
		val>>=1;
	}
	return crc;
}

static unsigned int calckey(unsigned int size)
{
	unsigned int ret = 0;
	size=(size-0x200) >> 3;
	int res1 = (size&0x3F80) << 1;
	res1 |= (size&0x4000) << 2;
	res1 |= (size&0x7F);
	res1 |= 0x380000;
	int res2 = res1;
	res1 = res2 >> 0x10;
	int res3 = res2 >> 8;
	res3 += res1;
	res3 += res2;
	res3 <<= 24;
	res3 |= res2;
	res3 |= 0x80808080;

	if((res3&0x200) == 0)
	{
		ret |= (((res3)&0xFF)^0x4B)<<24;
		ret |= (((res3>>8)&0xFF)^0x61)<<16;
		ret |= (((res3>>16)&0xFF)^0x77)<<8;
		ret |= (((res3>>24)&0xFF)^0x61);
	}
	else
	{
		ret |= (((res3)&0xFF)^0x73)<<24;
		ret |= (((res3>>8)&0xFF)^0x65)<<16;
		ret |= (((res3>>16)&0xFF)^0x64)<<8;
		ret |= (((res3>>24)&0xFF)^0x6F);
	}
	return ret;
}

void multiboot_wait_bios(void)
{
	u8 status = 0;
	while(!(status&0x10))
	{
		si_link_reset();
		status = si_link_status();
	}
}

void multiboot_send(const u8 *rom, u32 size)
{
	unsigned int i;
	unsigned int sendsize = ((size+7)&~7);
	unsigned int ourkey = calckey(sendsize);
	//printf("Our Key: %08x\n", ourkey);
	//get current sessionkey
	u32 sessionkeyraw = si_link_recv();
	u32 sessionkey = __builtin_bswap32(sessionkeyraw^0x7365646F);
	//send over our own key
	si_link_send(__builtin_bswap32(ourkey));
	unsigned int fcrc = 0x15a0;
	//send over gba header
	for(i = 0; i < 0xC0; i+=4)
		si_link_send(__builtin_bswap32(*(vu32*)(rom+i)));
	//printf("Header done! Sending ROM...\n");
	for(i = 0xC0; i < sendsize; i+=4)
	{
		u32 enc = ((rom[i+3]<<24)|(rom[i+2]<<16)|(rom[i+1]<<8)|(rom[i]));
		fcrc=docrc(fcrc,enc);
		sessionkey = (sessionkey*0x6177614B)+1;
		enc^=sessionkey;
		enc^=((~(i+(0x20<<20)))+1);
		enc^=0x20796220;
		si_link_send(enc);
	}
	fcrc |= (sendsize<<16);
	//printf("ROM done! CRC: %08x\n", fcrc);
	//send over CRC
	sessionkey = (sessionkey*0x6177614B)+1;
	fcrc^=sessionkey;
	fcrc^=((~(i+(0x20<<20)))+1);
	fcrc^=0x20796220;
	si_link_send(fcrc);
	//get crc back (unused)
	si_link_recv();
}
