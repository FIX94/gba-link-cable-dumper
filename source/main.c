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

//from my tests 50us seems to be the lowest
//safe si transfer delay in between calls
#define SI_TRANS_DELAY 50

#include "gba_mb_gba.h"

void printmain()
{
	printf("\x1b[2J");
	printf("\x1b[37m");
	printf("GBA Link Cable Dumper v1.6 by FIX94\n");
	printf("Save Support based on SendSave by Chishm\n");
	printf("GBA BIOS Dumper by Dark Fader\n \n");
}

u8 *resbuf,*cmdbuf;

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

void endproc()
{
	printf("Start pressed, exit\n");
	VIDEO_WaitVSync();
	VIDEO_WaitVSync();
	exit(0);
}
void fixFName(char *str)
{
	u8 i = 0;
	for(i = 0; i < strlen(str); ++i)
	{
		if(str[i] < 0x20 || str[i] > 0x7F)
			str[i] = '_';
		else switch(str[i])
		{
			case '\\':
			case '/':
			case ':':
			case '*':
			case '?':
			case '\"':
			case '<':
			case '>':
			case '|':
				str[i] = '_';
				break;
			default:
				break;
		}
	}
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
	SI_Transfer(1,cmdbuf,1,resbuf,3,transcb,SI_TRANS_DELAY);
	while(transval == 0) ;
}
void getstatus()
{
	cmdbuf[0] = 0; //status
	transval = 0;
	SI_Transfer(1,cmdbuf,1,resbuf,3,transcb,SI_TRANS_DELAY);
	while(transval == 0) ;
}
u32 recv()
{
	memset(resbuf,0,32);
	cmdbuf[0]=0x14; //read
	transval = 0;
	SI_Transfer(1,cmdbuf,1,resbuf,5,transcb,SI_TRANS_DELAY);
	while(transval == 0) ;
	return *(vu32*)resbuf;
}
void send(u32 msg)
{
	cmdbuf[0]=0x15;cmdbuf[1]=(msg>>0)&0xFF;cmdbuf[2]=(msg>>8)&0xFF;
	cmdbuf[3]=(msg>>16)&0xFF;cmdbuf[4]=(msg>>24)&0xFF;
	transval = 0;
	resbuf[0] = 0;
	SI_Transfer(1,cmdbuf,5,resbuf,1,transcb,SI_TRANS_DELAY);
	while(transval == 0) ;
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
void createFile(const char *path, size_t size)
{
	int fd = open(path, O_WRONLY|O_CREAT);
	if(fd >= 0)
	{
		ftruncate(fd, size);
		close(fd);
	}
}
void warnError(char *msg)
{
	puts(msg);
	VIDEO_WaitVSync();
	VIDEO_WaitVSync();
	sleep(2);
}
void fatalError(char *msg)
{
	puts(msg);
	VIDEO_WaitVSync();
	VIDEO_WaitVSync();
	sleep(5);
	exit(0);
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
		fatalError("ERROR: No usable device found to write dumped files to!");
	}
	mkdir("/dumps", S_IREAD | S_IWRITE);
	if(!dirExists("/dumps"))
	{
		printmain();
		fatalError("ERROR: Could not create dumps folder, make sure you have a supported device connected!");
	}
	int i;
	while(1)
	{
		printmain();
		printf("Waiting for a GBA in port 2...\n");
		resval = 0;

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
			u32 sessionkeyraw = recv();
			u32 sessionkey = __builtin_bswap32(sessionkeyraw^0x7365646F);
			//send over our own key
			send(__builtin_bswap32(ourkey));
			unsigned int fcrc = 0x15a0;
			//send over gba header
			for(i = 0; i < 0xC0; i+=4)
				send(__builtin_bswap32(*(vu32*)(gba_mb_gba+i)));
			//printf("Header done! Sending ROM...\n");
			for(i = 0xC0; i < sendsize; i+=4)
			{
				u32 enc = ((gba_mb_gba[i+3]<<24)|(gba_mb_gba[i+2]<<16)|(gba_mb_gba[i+1]<<8)|(gba_mb_gba[i]));
				fcrc=docrc(fcrc,enc);
				sessionkey = (sessionkey*0x6177614B)+1;
				enc^=sessionkey;
				enc^=((~(i+(0x20<<20)))+1);
				enc^=0x20796220;
				send(enc);
			}
			fcrc |= (sendsize<<16);
			//printf("ROM done! CRC: %08x\n", fcrc);
			//send over CRC
			sessionkey = (sessionkey*0x6177614B)+1;
			fcrc^=sessionkey;
			fcrc^=((~(i+(0x20<<20)))+1);
			fcrc^=0x20796220;
			send(fcrc);
			//get crc back (unused)
			recv();
			printf("Done!\n");
			sleep(2);
			//hm
			while(1)
			{
				printmain();
				printf("Press A once you have a GBA Game inserted.\n");
				printf("Press Y to backup the GBA BIOS.\n \n");
				PAD_ScanPads();
				VIDEO_WaitVSync();
				u32 btns = PAD_ButtonsDown(0);
				if(btns&PAD_BUTTON_START)
					endproc();
				else if(btns&PAD_BUTTON_A)
				{
					if(recv() == 0) //ready
					{
						printf("Waiting for GBA\n");
						VIDEO_WaitVSync();
						int gbasize = 0;
						while(gbasize == 0)
							gbasize = __builtin_bswap32(recv());
						send(0); //got gbasize
						u32 savesize = __builtin_bswap32(recv());
						send(0); //got savesize
						if(gbasize == -1) 
						{
							warnError("ERROR: No (Valid) GBA Card inserted!\n");
							continue;
						}
						//get rom header
						for(i = 0; i < 0xC0; i+=4)
							*(vu32*)(testdump+i) = recv();
						//print out all the info from the  game
						printf("Game Name: %.12s\n",(char*)(testdump+0xA0));
						printf("Game ID: %.4s\n",(char*)(testdump+0xAC));
						printf("Company ID: %.2s\n",(char*)(testdump+0xB0));
						printf("ROM Size: %02.02f MB\n",((float)(gbasize/1024))/1024.f);
						if(savesize > 0)
							printf("Save Size: %02.02f KB\n \n",((float)(savesize))/1024.f);
						else
							printf("No Save File\n \n");
						//generate file paths
						char gamename[64];
						sprintf(gamename,"/dumps/%.12s [%.4s%.2s].gba",
							(char*)(testdump+0xA0),(char*)(testdump+0xAC),(char*)(testdump+0xB0));
						fixFName(gamename+7); //fix name behind "/dumps/"
						char savename[64];
						sprintf(savename,"/dumps/%.12s [%.4s%.2s].sav",
							(char*)(testdump+0xA0),(char*)(testdump+0xAC),(char*)(testdump+0xB0));
						fixFName(savename+7); //fix name behind "/dumps/"
						//let the user choose the option
						printf("Press A to dump this game, it will take about %i minutes.\n",gbasize/1024/1024*3/2);
						printf("Press B if you want to cancel dumping this game.\n");
						if(savesize > 0)
						{
							printf("Press Y to backup this save file.\n");
							printf("Press X to restore this save file.\n");
							printf("Press Z to clear the save file on the GBA Cartridge.\n\n");
						}
						else
							printf("\n");
						int command = 0;
						while(1)
						{
							PAD_ScanPads();
							VIDEO_WaitVSync();
							u32 btns = PAD_ButtonsDown(0);
							if(btns&PAD_BUTTON_START)
								endproc();
							else if(btns&PAD_BUTTON_A)
							{
								command = 1;
								break;
							}
							else if(btns&PAD_BUTTON_B)
								break;
							else if(savesize > 0)
							{
								if(btns&PAD_BUTTON_Y)
								{
									command = 2;
									break;
								}
								else if(btns&PAD_BUTTON_X)
								{
									command = 3;
									break;
								}
								else if(btns&PAD_TRIGGER_Z)
								{
									command = 4;
									break;
								}
							}
						}
						if(command == 1)
						{
							FILE *f = fopen(gamename,"rb");
							if(f)
							{
								fclose(f);
								command = 0;
								warnError("ERROR: Game already dumped!\n");
							}
						}
						else if(command == 2)
						{
							FILE *f = fopen(savename,"rb");
							if(f)
							{
								fclose(f);
								command = 0;
								warnError("ERROR: Save already backed up!\n");
							}
						}
						else if(command == 3)
						{
							size_t readsize = 0;
							FILE *f = fopen(savename,"rb");
							if(f)
							{
								fseek(f,0,SEEK_END);
								readsize = ftell(f);
								if(readsize != savesize)
								{
									command = 0;
									warnError("ERROR: Save has the wrong size, aborting restore!\n");
								}
								else
								{
									rewind(f);
									fread(testdump,readsize,1,f);
								}
								fclose(f);
							}
							else
							{
								command = 0;
								warnError("ERROR: No Save to restore!\n");
							}
						}
						send(command);
						//let gba prepare
						sleep(1);
						if(command == 0)
							continue;
						else if(command == 1)
						{
							//create base file with size
							printf("Preparing file...\n");
							createFile(gamename,gbasize);
							FILE *f = fopen(gamename,"wb");
							if(!f)
								fatalError("ERROR: Could not create file! Exit...");
							printf("Dumping...\n");
							u32 bytes_read = 0;
							while(gbasize > 0)
							{
								int toread = (gbasize > 0x400000 ? 0x400000 : gbasize);
								int j;
								for(j = 0; j < toread; j+=4)
								{
									*(vu32*)(testdump+j) = recv();
									bytes_read+=4;
									if((bytes_read&0xFFFF) == 0)
										printf("\r%02.02f MB done",(float)(bytes_read/1024)/1024.f);
								}
								fwrite(testdump,toread,1,f);
								gbasize -= toread;
							}
							printf("\nClosing file\n");
							fclose(f);
							printf("Game dumped!\n");
							sleep(5);
						}
						else if(command == 2)
						{
							//create base file with size
							printf("Preparing file...\n");
							createFile(savename,savesize);
							FILE *f = fopen(savename,"wb");
							if(!f)
								fatalError("ERROR: Could not create file! Exit...");
							printf("Waiting for GBA\n");
							VIDEO_WaitVSync();
							u32 readval = 0;
							while(readval != savesize)
								readval = __builtin_bswap32(recv());
							send(0); //got savesize
							printf("Receiving...\n");
							for(i = 0; i < savesize; i+=4)
								*(vu32*)(testdump+i) = recv();
							printf("Writing save...\n");
							fwrite(testdump,savesize,1,f);
							fclose(f);
							printf("Save backed up!\n");
							sleep(5);
						}
						else if(command == 3 || command == 4)
						{
							u32 readval = 0;
							while(readval != savesize)
								readval = __builtin_bswap32(recv());
							if(command == 3)
							{
								printf("Sending save\n");
								VIDEO_WaitVSync();
								for(i = 0; i < savesize; i+=4)
									send(__builtin_bswap32(*(vu32*)(testdump+i)));
							}
							printf("Waiting for GBA\n");
							while(recv() != 0)
								VIDEO_WaitVSync();
							printf(command == 3 ? "Save restored!\n" : "Save cleared!\n");
							send(0);
							sleep(5);
						}
					}
				}
				else if(btns&PAD_BUTTON_Y)
				{
					const char *biosname = "/dumps/gba_bios.bin";
					FILE *f = fopen(biosname,"rb");
					if(f)
					{
						fclose(f);
						warnError("ERROR: BIOS already backed up!\n");
					}
					else
					{
						//create base file with size
						printf("Preparing file...\n");
						createFile(biosname,0x4000);
						f = fopen(biosname,"wb");
						if(!f)
							fatalError("ERROR: Could not create file! Exit...");
						//send over bios dump command
						send(5);
						//the gba might still be in a loop itself
						sleep(1);
						//lets go!
						printf("Dumping...\n");
						for(i = 0; i < 0x4000; i+=4)
							*(vu32*)(testdump+i) = recv();
						fwrite(testdump,0x4000,1,f);
						printf("Closing file\n");
						fclose(f);
						printf("BIOS dumped!\n");
						sleep(5);
					}
				}
			}
		}
	}
	return 0;
}
