/*
 * Copyright (C) 2016 FIX94
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */
#include <gccore.h>
#include <malloc.h>
#include <string.h>
#include "link/si_link.h"

// the gba link cable is connected to controller port 2
#define SI_LINK_CHAN 1

//from my tests 50us seems to be the lowest
//safe si transfer delay in between calls
#define SI_TRANS_DELAY 50

static u8 *resbuf,*cmdbuf;

static volatile u32 transval = 0;
static void transcb(s32 chan, u32 ret)
{
	transval = 1;
}

static volatile u32 resval = 0;
static void acb(s32 res, u32 val)
{
	resval = val;
}

void si_link_init(void)
{
	cmdbuf = memalign(32,32);
	resbuf = memalign(32,32);
}

void si_link_probe_start(void)
{
	resval = 0;
	SI_GetTypeAsync(SI_LINK_CHAN,acb);
}

u32 si_link_probe_poll(void)
{
	if(resval)
	{
		if(resval == 0x80 || resval & 8)
		{
			resval = 0;
			SI_GetTypeAsync(SI_LINK_CHAN,acb);
		}
		else
			return resval;
	}
	return 0;
}

void si_link_reset(void)
{
	cmdbuf[0] = 0xFF; //reset
	transval = 0;
	SI_Transfer(SI_LINK_CHAN,cmdbuf,1,resbuf,3,transcb,SI_TRANS_DELAY);
	while(transval == 0) ;
}

u8 si_link_status(void)
{
	cmdbuf[0] = 0; //status
	transval = 0;
	SI_Transfer(SI_LINK_CHAN,cmdbuf,1,resbuf,3,transcb,SI_TRANS_DELAY);
	while(transval == 0) ;
	return resbuf[2];
}

u32 si_link_recv(void)
{
	memset(resbuf,0,32);
	cmdbuf[0]=0x14; //read
	transval = 0;
	SI_Transfer(SI_LINK_CHAN,cmdbuf,1,resbuf,5,transcb,SI_TRANS_DELAY);
	while(transval == 0) ;
	return *(vu32*)resbuf;
}

void si_link_send(u32 msg)
{
	cmdbuf[0]=0x15;cmdbuf[1]=(msg>>0)&0xFF;cmdbuf[2]=(msg>>8)&0xFF;
	cmdbuf[3]=(msg>>16)&0xFF;cmdbuf[4]=(msg>>24)&0xFF;
	transval = 0;
	resbuf[0] = 0;
	SI_Transfer(SI_LINK_CHAN,cmdbuf,5,resbuf,1,transcb,SI_TRANS_DELAY);
	while(transval == 0) ;
}
