/*
	libSave
	Cartridge backup memory save routines. To use, call the required
	routine with a pointer to an appropriately sized array of data to
	be read from or written to the cartridge.
	Data types are from wintermute's gba_types.h libgba library.
	Original file from SendSave by Chishm
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gba_dma.h>

#define EEPROM_ADDRESS (0xDFFFF00)
#define REG_EEPROM *(vu16 *)(EEPROM_ADDRESS)
#define	REG_DMA3CNT_H *(vu16 *)(REG_BASE + 0x0de)
#define	REG_WAITCNT *(vu16 *)(REG_BASE + 0x204)

//-----------------------------------------------------------------------
// Common EEPROM Routines
//-----------------------------------------------------------------------

void EEPROM_SendPacket( u16* packet, int size ) 
{ 
   REG_WAITCNT = (REG_WAITCNT & 0xF8FF) | 0x0300;
   REG_DMA3SAD = (u32)packet; 
   REG_DMA3DAD = EEPROM_ADDRESS; 
   REG_DMA3CNT = 0x80000000 + size; 
   while((REG_DMA3CNT_H & 0x8000) != 0) ;
} 

void EEPROM_ReceivePacket( u16* packet, int size ) 
{ 
   REG_WAITCNT = (REG_WAITCNT & 0xF8FF) | 0x0300;
   REG_DMA3SAD = EEPROM_ADDRESS; 
   REG_DMA3DAD = (u32)packet; 
   REG_DMA3CNT = 0x80000000 + size; 
   while((REG_DMA3CNT_H & 0x8000) != 0) ;
} 

//-----------------------------------------------------------------------
// Routines for 512B EEPROM
//-----------------------------------------------------------------------

void EEPROM_Read_512B( volatile u8 offset, u8* dest ) // dest must point to 8 bytes 
{ 
   u16 packet[68]; 
   u8* out_pos; 
   u16* in_pos; 
   u8 out_byte; 
   int byte, bit; 

   memset( packet, 0, 68*2 );

   // Read request 
   packet[0] = 1; 
   packet[1] = 1; 

   // 6 bits eeprom address (MSB first) 
   packet[2] = (offset>>5)&1; 
   packet[3] = (offset>>4)&1; 
   packet[4] = (offset>>3)&1; 
   packet[5] = (offset>>2)&1; 
   packet[6] = (offset>>1)&1; 
   packet[7] = (offset)&1; 

   // End of request 
   packet[8] = 0; 

   // Do transfers 
   EEPROM_SendPacket( packet, 9 ); 
   memset( packet, 0, 68*2 );
   EEPROM_ReceivePacket( packet, 68 ); 
   
   // Extract data 
   in_pos = &packet[4]; 
   out_pos = dest; 
   for( byte = 7; byte >= 0; --byte ) 
   { 
      out_byte = 0; 
      for( bit = 7; bit >= 0; --bit ) 
      { 
//         out_byte += (*in_pos++)<<bit; 
		  out_byte += ((*in_pos++)&1)<<bit;
      } 
      *out_pos++ = out_byte ; 
   } 
} 

void EEPROM_Write_512B( volatile u8 offset, u8* source ) // source must point to 8 bytes 
{ 
   u16 packet[73]; 
   u8* in_pos; 
   u16* out_pos; 
   u8 in_byte; 
   int byte, bit; 

   memset( packet, 0, 73*2 );

   // Write request 
   packet[0] = 1; 
   packet[1] = 0; 

   // 6 bits eeprom address (MSB first) 
   packet[2] = (offset>>5)&1; 
   packet[3] = (offset>>4)&1; 
   packet[4] = (offset>>3)&1; 
   packet[5] = (offset>>2)&1; 
   packet[6] = (offset>>1)&1; 
   packet[7] = (offset)&1; 

   // Extract data 
   in_pos = source; 
   out_pos = &packet[8]; 
   for( byte = 7; byte >= 0; --byte ) 
   { 
      in_byte = *in_pos++; 
      for( bit = 7; bit >= 0; --bit ) 
      { 
         *out_pos++ = (in_byte>>bit)&1; 
      } 
   } 

   // End of request 
   packet[72] = 0; 

   // Do transfers 
   EEPROM_SendPacket( packet, 73 ); 

   // Wait for EEPROM to finish (should timeout after 10 ms) 
   while( (REG_EEPROM & 1) == 0 ); 
} 

//---------------------------------------------------------------------------------
void GetSave_EEPROM_512B(u8* data)
//---------------------------------------------------------------------------------
{
	volatile u8 x;
	u32 sleep;

    for (x=0;x<64;++x){
		EEPROM_Read_512B(x,&data[x*8]);
		for(sleep=0;sleep<512000;sleep++);
	}
}

//---------------------------------------------------------------------------------
void PutSave_EEPROM_512B(u8* data)
//---------------------------------------------------------------------------------
{
	volatile u8 x;
	u32 sleep;

    for (x=0;x<64;x++){ 
		EEPROM_Write_512B(x,&data[x*8]);
		for(sleep=0;sleep<512000;sleep++);
	}
}
//-----------------------------------------------------------------------
// Routines for 8KB EEPROM
//-----------------------------------------------------------------------

void EEPROM_Read_8KB( volatile u16 offset, u8* dest ) // dest must point to 8 bytes 
{ 
   u16 packet[68]; 
   u8* out_pos; 
   u16* in_pos; 
   u8 out_byte; 
   int byte, bit; 

   memset( packet, 0, 68*2 );

   // Read request 
   packet[0] = 1; 
   packet[1] = 1; 

   // 14 bits eeprom address (MSB first) 
   packet[2] = (offset>>13)&1; 
   packet[3] = (offset>>12)&1; 
   packet[4] = (offset>>11)&1; 
   packet[5] = (offset>>10)&1; 
   packet[6] = (offset>>9)&1; 
   packet[7] = (offset>>8)&1;
   packet[8] = (offset>>7)&1; 
   packet[9] = (offset>>6)&1; 
   packet[10] = (offset>>5)&1; 
   packet[11] = (offset>>4)&1; 
   packet[12] = (offset>>3)&1; 
   packet[13] = (offset>>2)&1;
   packet[14] = (offset>>1)&1;
   packet[15] = (offset)&1;

   // End of request 
   packet[16] = 0; 

   // Do transfers 
   EEPROM_SendPacket( packet, 17 ); 
   memset( packet, 0, 68*2 );
   EEPROM_ReceivePacket( packet, 68 ); 
   
   // Extract data 
   in_pos = &packet[4]; 
   out_pos = dest; 
   for( byte = 7; byte >= 0; --byte ) 
   { 
      out_byte = 0; 
      for( bit = 7; bit >= 0; --bit ) 
      { 
//         out_byte += (*in_pos++)<<bit; 
		  out_byte += ((*in_pos++)&1)<<bit;
      } 
      *out_pos++ = out_byte; 
   } 

} 

void EEPROM_Write_8KB( volatile u16 offset, u8* source ) // source must point to 8 bytes 
{ 
   u16 packet[81]; 
   u8* in_pos; 
   u16* out_pos; 
   u8 in_byte; 
   int byte, bit; 

   memset( packet, 0, 81*2 );
   
   // Write request 
   packet[0] = 1; 
   packet[1] = 0; 

   // 14 bits eeprom address (MSB first) 
   packet[2] = (offset>>13)&1; 
   packet[3] = (offset>>12)&1; 
   packet[4] = (offset>>11)&1; 
   packet[5] = (offset>>10)&1; 
   packet[6] = (offset>>9)&1; 
   packet[7] = (offset>>8)&1;
   packet[8] = (offset>>7)&1; 
   packet[9] = (offset>>6)&1; 
   packet[10] = (offset>>5)&1; 
   packet[11] = (offset>>4)&1; 
   packet[12] = (offset>>3)&1; 
   packet[13] = (offset>>2)&1;
   packet[14] = (offset>>1)&1;
   packet[15] = (offset)&1;

   // Extract data 
   in_pos = source; 
   out_pos = &packet[16]; 
   for( byte = 7; byte >= 0; --byte ) 
   { 
      in_byte = *in_pos++; 
      for( bit = 7; bit >= 0; --bit ) 
      { 
         *out_pos++ = (in_byte>>bit)&1; 
      } 
   } 

   // End of request 
   packet[80] = 0; 

   // Do transfers 
   EEPROM_SendPacket( packet, 81 ); 

   // Wait for EEPROM to finish (should timeout after 10 ms) 
   while( (REG_EEPROM & 1) == 0 ); 
} 

//---------------------------------------------------------------------------------
void GetSave_EEPROM_8KB(u8* data)
//---------------------------------------------------------------------------------
{
	volatile u16 x;
	u32 sleep;

    for (x=0;x<1024;x++){ 
		EEPROM_Read_8KB(x,&data[x*8]);
		for(sleep=0;sleep<512000;sleep++);
	}

}

//---------------------------------------------------------------------------------
void PutSave_EEPROM_8KB(u8* data)
//---------------------------------------------------------------------------------
{
	volatile u16 x;
	u32 sleep;

    for (x=0;x<1024;x++){ 
		EEPROM_Write_8KB(x,&data[x*8]);
		for(sleep=0;sleep<512000;sleep++);
	}
}

//---------------------------------------------------------------------------------
void GetSave_SRAM_32KB(u8* data)
//---------------------------------------------------------------------------------
{
	volatile u8 *sram= (u8*) 0x0E000000;
	volatile u16 x;

	for (x = 0; x < 0x8000; ++x)
	{
		data[x] = sram[x];
	}

}

//---------------------------------------------------------------------------------
void PutSave_SRAM_32KB(u8* data)
//---------------------------------------------------------------------------------
{
	volatile u8 *sram= (u8*) 0x0E000000;
	volatile u16 x;

	for (x = 0; x < 0x8000; ++x)
	{
		sram[x] = data[x];
	}
}

//---------------------------------------------------------------------------------
void GetSave_FLASH_64KB(u8* data)
//---------------------------------------------------------------------------------
{
	volatile u8 *sram= (u8*) 0x0E000000;
	volatile u32 x;

	for (x = 0; x < 0x10000; ++x)
	{
		data[x] = sram[x];
	}
}

//---------------------------------------------------------------------------------
void PutSave_FLASH_64KB(u8* foo)
//---------------------------------------------------------------------------------
{
	volatile u8 *fctrl0 = (u8*) 0xE005555; 
	volatile u8 *fctrl1 = (u8*) 0xE002AAA; 
	volatile u8 *fctrl2 = (u8*) 0xE000000; 

	//init flash 
	*fctrl0 = 0xAA; 
	*fctrl1 = 0x55; 
	*fctrl0 = 0x90; 
	*fctrl2 = 0xF0; 

	//erase chip 
	*fctrl0 = 0xAA; 
	*fctrl1 = 0x55; 
	*fctrl0 = 0x80; 
	*fctrl0 = 0xAA; 
	*fctrl1 = 0x55; 
	*fctrl0 = 0x10; 

	//wait for erase done 
	u8 val1;
	u8 val2; 
	val1 = *fctrl2;
	val2 = *fctrl2; 
	while (val1 != val2) {
		val1 = *fctrl2;
		val2 = *fctrl2;
	}
	val1 = *fctrl2;
	val2 = *fctrl2; 
	while (val1 != val2) {
		val1 = *fctrl2;
		val2 = *fctrl2; 
	}

	volatile u8 *data = fctrl2; 
	u32 i; 
	//write data 
	for (i=0; i<65536; i++) { 
		*fctrl0 = 0xAA; 
		*fctrl1 = 0x55; 
		*fctrl0 = 0xA0; 
		data [i] = foo [ i ]; 
		val1 = data [ i ]; 
		val2 = data [ i ]; 
	
		while (val1 != val2) {
			val1 = data [ i ]; 
			val2 = data [ i ]; 
		}
		val1 = data [ i ]; 
		val2 = data [ i ]; 
		while (val1 != val2) {
			val1 = data [ i ];
			val2 = data [ i ]; 
		} 
		val1 = data [ i ]; 
		val2 = data [ i ]; 
		while (val1 != val2) {
			val1 = data [ i ]; 
			val2 = data [ i ]; 
		} 
	}
}

//---------------------------------------------------------------------------------
void GetSave_FLASH_128KB(u8* data)
//---------------------------------------------------------------------------------
{
	const u32 size = 0x10000;
	
	volatile u8 *fctrl0 = (u8*) 0xE005555; 
	volatile u8 *fctrl1 = (u8*) 0xE002AAA; 
	volatile u8 *fctrl2 = (u8*) 0xE000000; 
	volatile u32 i; 
	volatile u8 *sram= (u8*) 0x0E000000;
	
	//init flash 
	*fctrl0 = 0xAA; 
	*fctrl1 = 0x55; 
	*fctrl0 = 0x90; 
	*fctrl2 = 0xF0; 

	// read first bank
	*fctrl0 = 0xAA;
	*fctrl1 = 0x55;
	*fctrl0 = 0xB0;
	*fctrl2 = 0x00;
	
	for (i=0; i<size; i++){
		data[i] = sram[i];
	}

	// read second bank
	*fctrl0 = 0xAA;
	*fctrl1 = 0x55;
	*fctrl0 = 0xB0;
	*fctrl2 = 0x01;
	
	for (i=0; i<size; i++){
		data[i + size] = sram[i];
	}

}

//---------------------------------------------------------------------------------
void PutSave_FLASH_128KB(u8* foo)
//---------------------------------------------------------------------------------
{
	volatile u8 *fctrl0 = (u8*) 0xE005555; 
	volatile u8 *fctrl1 = (u8*) 0xE002AAA; 
	volatile u8 *fctrl2 = (u8*) 0xE000000; 

	u8 val1;
	u8 val2; 

	//init flash 
	*fctrl0 = 0xAA; 
	*fctrl1 = 0x55; 
	*fctrl0 = 0x90; 
	*fctrl2 = 0xF0; 

	//erase chip 
	*fctrl0 = 0xAA; 
	*fctrl1 = 0x55; 
	*fctrl0 = 0x80; 
	*fctrl0 = 0xAA; 
	*fctrl1 = 0x55; 
	*fctrl0 = 0x10; 
	
	//wait for erase done 
	val1 = *fctrl2;
	val2 = *fctrl2; 
	while (val1 != val2) {
		val1 = *fctrl2;
		val2 = *fctrl2;
	}
	val1 = *fctrl2;
	val2 = *fctrl2; 
	while (val1 != val2) {
		val1 = *fctrl2;
		val2 = *fctrl2; 
	}

	volatile u8 *data = fctrl2; 
	volatile u32 i; 
	// change to bank 0
	*fctrl0 = 0xAA;
	*fctrl1 = 0x55;
	*fctrl0 = 0xB0;
	*fctrl2 = 0x00;
		
	//write data 
	for (i=0; i<65536; i++) { 
		*fctrl0 = 0xAA; 
		*fctrl1 = 0x55; 
		*fctrl0 = 0xA0; 
		data [i] = foo [ i ]; 
		val1 = data [ i ]; 
		val2 = data [ i ]; 
	
		while (val1 != val2) {
			val1 = data [ i ]; 
			val2 = data [ i ]; 
		}
		val1 = data [ i ]; 
		val2 = data [ i ]; 
		while (val1 != val2) {
			val1 = data [ i ];
			val2 = data [ i ]; 
		} 
		val1 = data [ i ]; 
		val2 = data [ i ]; 
		while (val1 != val2) {
			val1 = data [ i ]; 
			val2 = data [ i ]; 
		} 
	}

	// Change to bank 1
	*fctrl0 = 0xAA;
	*fctrl1 = 0x55;
	*fctrl0 = 0xB0;
	*fctrl2 = 0x01;
	
	//write data 
	for (i=0; i<65536; i++) { 
		*fctrl0 = 0xAA; 
		*fctrl1 = 0x55; 
		*fctrl0 = 0xA0; 
		data [i] = foo [ i + 0x10000]; 
		val1 = data [ i ]; 
		val2 = data [ i ]; 
	
		while (val1 != val2) {
			val1 = data [ i ]; 
			val2 = data [ i ]; 
		}
		val1 = data [ i ]; 
		val2 = data [ i ]; 
		while (val1 != val2) {
			val1 = data [ i ];
			val2 = data [ i ]; 
		} 
		val1 = data [ i ]; 
		val2 = data [ i ]; 
		while (val1 != val2) {
			val1 = data [ i ]; 
			val2 = data [ i ]; 
		} 
	}

}

//---------------------------------------------------------------------------------
u32 SaveSize(u8* data, s32 gamesize)
//---------------------------------------------------------------------------------
{
	if(gamesize < 0)
	 return 0;

	u32 *pak= ((u32*)0x08000000);
	s32 x;
	u16 i;
	s32 size = gamesize/4;


	for (x=size-1;x>=0;x--){
		switch (pak[x]) {
		case 0x53414C46:
			if (pak[x+1] == 0x5F4D3148){
				return 0x20000;					// FLASH_128KB
			} else if ((pak[x+1] & 0x0000FFFF) == 0x00005F48){
				return 0x10000;					// FLASH_64KB
			} else if (pak[x+1] == 0x32313548){
				return 0x10000;					// FLASH_64KB
			}
			break;
		case 0x52504545:
			if ((pak[x+1] & 0x00FFFFFF) == 0x005F4D4F){
				GetSave_EEPROM_8KB(data);
				for(i = 8; i < 0x800; i += 8) {
					if(memcmp(data, data+i, 8) != 0)
						return 0x2000;			// EEPROM_8KB
				}
				return 0x200;					// EEPROM_512B
			}
			break;
		case 0x4D415253:
			if ((pak[x+1] & 0x000000FF) == 0x0000005F){
				return 0x8000;					// SRAM_32KB
			}
		}
	}
	return 0;
}

