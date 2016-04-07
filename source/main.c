/*
 * Copyright (C) 2016 FIX94
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */
#include <gccore.h>
#include <stdio.h>
#include <malloc.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <fat.h>

extern u8 gba_mb_gba[];
extern u32 gba_mb_gba_size;

void printmain()
{
	printf("\x1b[2J");
	printf("\x1b[37m");
	printf("GBA Link Cable Dumper v1.0 by FIX94\n");
}

u8 *resbuf,*cmdbuf;
volatile u16 pads = 0;
volatile bool ctrlerr = false;
void ctrlcb(s32 chan, u32 ret)
{
	if(ret)
	{
		ctrlerr = true;
		return;
	}
	//just call us again
	pads = (~((resbuf[1]<<8)|resbuf[0]))&0x3FF;
	SI_Transfer(1,cmdbuf,1,resbuf,5,ctrlcb,350);
}

volatile u32 transval = 0;
void transcb(s32 chan, u32 ret)
{
	transval = 1;
}

volatile u32 resval = 0;
void acb(s32 res, u32 val)
{
	resval = val;
}

unsigned int docrc(u32 crc, u32 val)
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

static inline void wait_for_transfer()
{
	//350 is REALLY pushing it already, cant go further
	do{ usleep(350); }while(transval == 0);
}

void endproc()
{
	printf("Start pressed, exit\n");
	VIDEO_WaitVSync();
	VIDEO_WaitVSync();
	exit(0);
}
unsigned int calckey(unsigned int size)
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
void doreset()
{
	cmdbuf[0] = 0xFF; //reset
	transval = 0;
	SI_Transfer(1,cmdbuf,1,resbuf,3,transcb,0);
	wait_for_transfer();
}
void getstatus()
{
	cmdbuf[0] = 0; //status
	transval = 0;
	SI_Transfer(1,cmdbuf,1,resbuf,3,transcb,0);
	wait_for_transfer();
}
u32 recvsafe()
{
	memset(resbuf,0,32);
	cmdbuf[0]=0x14; //read
	transval = 0;
	SI_Transfer(1,cmdbuf,1,resbuf,5,transcb,0);
	wait_for_transfer();
	return *(vu32*)resbuf;
}
void sendsafe(u32 msg)
{
	cmdbuf[0]=0x15;cmdbuf[1]=(msg>>0)&0xFF;cmdbuf[2]=(msg>>8)&0xFF;
	cmdbuf[3]=(msg>>16)&0xFF;cmdbuf[4]=(msg>>24)&0xFF;
	transval = 0;
	resbuf[0] = 0;
	SI_Transfer(1,cmdbuf,5,resbuf,1,transcb,0);
	wait_for_transfer();
}
u32 recvfast()
{
	cmdbuf[0]=0x14; //read
	transval = 0;
	SI_Transfer(1,cmdbuf,1,resbuf,5,transcb,0);
	usleep(275);
	while(transval == 0) ;
	return *(vu32*)resbuf;
}
bool dirExists(const char *path)
{
	DIR *dir;
	dir = opendir(path);
	if(dir)
	{
		closedir(dir);
		return true;
	}
	return false;
}
int main(int argc, char *argv[]) 
{
	void *xfb = NULL;
	GXRModeObj *rmode = NULL;
	VIDEO_Init();
	rmode = VIDEO_GetPreferredMode(NULL);
	xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
	VIDEO_Configure(rmode);
	VIDEO_SetNextFramebuffer(xfb);
	VIDEO_SetBlack(FALSE);
	VIDEO_Flush();
	VIDEO_WaitVSync();
	if(rmode->viTVMode&VI_NON_INTERLACE) VIDEO_WaitVSync();
	int x = 24, y = 32, w, h;
	w = rmode->fbWidth - (32);
	h = rmode->xfbHeight - (48);
	CON_InitEx(rmode, x, y, w, h);
	VIDEO_ClearFrameBuffer(rmode, xfb, COLOR_BLACK);
	PAD_Init();
	cmdbuf = memalign(32,32);
	resbuf = memalign(32,32);
	u8 *testdump = memalign(32,0x400000);
	if(!testdump) return 0;
	if(!fatInitDefault())
	{
		printmain();
		printf("ERROR: No usable device found to write dumped files to!\n");
		VIDEO_WaitVSync();
		VIDEO_WaitVSync();
		sleep(5);
		exit(0);
	}
	mkdir("/dumps", S_IREAD | S_IWRITE);
	if(!dirExists("/dumps"))
	{
		printmain();
		printf("ERROR: Could not create dumps folder, make sure you have a supported device connected!\n");
		VIDEO_WaitVSync();
		VIDEO_WaitVSync();
		sleep(5);
		exit(0);
	}
	int i;
	while(1)
	{
		printmain();
		printf("Waiting for a GBA in port 2...\n");
		resval = 0;
		ctrlerr = false;

		SI_GetTypeAsync(1,acb);
		while(1)
		{
			if(resval)
			{
				if(resval == 0x80 || resval & 8)
				{
					resval = 0;
					SI_GetTypeAsync(1,acb);
				}
				else if(resval)
					break;
			}
			PAD_ScanPads();
			VIDEO_WaitVSync();
			if(PAD_ButtonsHeld(0))
				endproc();
		}
		if(resval & SI_GBA)
		{
			printf("GBA Found! Waiting on BIOS\n");
			resbuf[2]=0;
			while(!(resbuf[2]&0x10))
			{
				doreset();
				getstatus();
			}
			printf("Ready, sending dumper\n");
			unsigned int sendsize = ((gba_mb_gba_size+7)&~7);
			unsigned int ourkey = calckey(sendsize);
			//printf("Our Key: %08x\n", ourkey);
			//get current sessionkey
			u32 sessionkeyraw = recvsafe();
			u32 sessionkey = __builtin_bswap32(sessionkeyraw^0x7365646F);
			//send over our own key
			sendsafe(__builtin_bswap32(ourkey));
			unsigned int fcrc = 0x15a0;
			//send over gba header
			for(i = 0; i < 0xC0; i+=4)
			{
				sendsafe(__builtin_bswap32(*(vu32*)(gba_mb_gba+i)));
				if(!(resbuf[0]&0x2)) printf("Possible error %02x\n",resbuf[0]);
			}
			//printf("Header done! Sending ROM...\n");
			for(i = 0xC0; i < sendsize; i+=4)
			{
				u32 enc = ((gba_mb_gba[i+3]<<24)|(gba_mb_gba[i+2]<<16)|(gba_mb_gba[i+1]<<8)|(gba_mb_gba[i]));
				fcrc=docrc(fcrc,enc);
				sessionkey = (sessionkey*0x6177614B)+1;
				enc^=sessionkey;
				enc^=((~(i+(0x20<<20)))+1);
				enc^=0x20796220;
				sendsafe(enc);
				if(!(resbuf[0]&0x2)) printf("Possible error %02x\n",resbuf[0]);
			}
			fcrc |= (sendsize<<16);
			//printf("ROM done! CRC: %08x\n", fcrc);
			//send over CRC
			sessionkey = (sessionkey*0x6177614B)+1;
			fcrc^=sessionkey;
			fcrc^=((~(i+(0x20<<20)))+1);
			fcrc^=0x20796220;
			sendsafe(fcrc);
			//get crc back (unused)
			recvsafe();
			printf("Done!\n");
			sleep(2);
			//hm
			while(1)
			{
				printmain();
				printf("Press A once you have a GBA Game inserted.\n \n");
				PAD_ScanPads();
				VIDEO_WaitVSync();
				u32 btns = PAD_ButtonsDown(0);
				if(btns&PAD_BUTTON_START)
					endproc();
				else if(btns&PAD_BUTTON_A)
				{
					if(recvsafe() == 0) //ready
					{
						sleep(1); //gba rom prepare
						u32 gbasize = __builtin_bswap32(recvsafe());
						if(gbasize == 0) 
						{
							printf("ERROR: No (Valid) GBA Card inserted!\n");
							VIDEO_WaitVSync();
							VIDEO_WaitVSync();
							sleep(2);
							continue;
						}
						for(i = 0; i < 0xC0; i+=4)
							*(vu32*)(testdump+i) = recvfast();
						printf("Game Name: %.12s\n",(char*)(testdump+0xA0));
						printf("Game ID: %.4s\n",(char*)(testdump+0xAC));
						printf("Company ID: %.2s\n",(char*)(testdump+0xB0));
						printf("ROM Size: %02.02f MB\n \n",((float)(gbasize/1024))/1024.f);
						char gamename[64];
						sprintf(gamename,"/dumps/%.12s [%.4s%.2s].gba",
							(char*)(testdump+0xA0),(char*)(testdump+0xAC),(char*)(testdump+0xB0));
						FILE *f = fopen(gamename,"rb");
						if(f)
						{
							fclose(f);
							sendsafe(0);
							printf("ERROR: Game already dumped! Please insert another game.\n");
							VIDEO_WaitVSync();
							VIDEO_WaitVSync();
							sleep(2);
							continue;
						}
						printf("Press A to dump this game, it will take about %i minutes.\n",gbasize/1024/1024*3/2);
						printf("Press B if you want to cancel dumping this game.\n\n");
						int dumping = 0;
						while(1)
						{
							PAD_ScanPads();
							VIDEO_WaitVSync();
							u32 btns = PAD_ButtonsDown(0);
							if(btns&PAD_BUTTON_START)
								endproc();
							else if(btns&PAD_BUTTON_A)
							{
								dumping = 1;
								break;
							}
							else if(btns&PAD_BUTTON_B)
								break;
						}
						sendsafe(dumping);
						if(dumping == 0)
							continue;
						//create base file with size
						printf("Creating file...\n");
						int fd = open(gamename, O_WRONLY|O_CREAT);
						if(fd >= 0)
						{
							ftruncate(fd, gbasize);
							close(fd);
						}
						f = fopen(gamename,"wb");
						if(!f)
						{
							printf("ERROR: Could not create file! Exit...\n");
							VIDEO_WaitVSync();
							VIDEO_WaitVSync();
							sleep(5);
							exit(0);
						}
						printf("Dumping...\n");
						u32 bytes_read = 0;
						while(gbasize > 0)
						{
							int toread = (gbasize > 0x400000 ? 0x400000 : gbasize);
							int j;
							for(j = 0; j < toread; j+=4)
							{
								*(vu32*)(testdump+j) = recvfast();
								bytes_read+=4;
								if((bytes_read&0xFFFF) == 0)
								{
									printf("\r%02.02f MB done",(float)(bytes_read/1024)/1024.f);
									VIDEO_WaitVSync();
								}
								//printf("%02x%02x%02x%02x",resbuf[0],resbuf[1],resbuf[2],resbuf[3]);
							}
							fwrite(testdump,toread,1,f);
							gbasize -= toread;
						}
						printf("\nClosing file\n");
						fclose(f);
						printf("Game dumped!\n");
						sleep(5);
					}
				}
			}
		}
	}
	return 0;
}
