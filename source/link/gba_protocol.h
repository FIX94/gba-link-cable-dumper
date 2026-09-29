/*
 * Copyright (C) 2016 FIX94
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */
#ifndef __GBA_PROTOCOL_H__
#define __GBA_PROTOCOL_H__

#include <gccore.h>
#include "protocol.h"

// high level commands understood by the GBA multiboot payload (gba/source/main.c)
// nothing in here touches the screen, input or the filesystem

typedef struct
{
	s32 rom_size;	// -1 when no (valid) cart is inserted
	u32 save_size;	// 0 when the cart has no save
	u8 header[GBA_HEADER_SIZE];
} gba_cart_info;

#define GBA_CART_TITLE(c)		((const char*)((c)->header+0xA0))	// 12 chars, not terminated
#define GBA_CART_GAME_CODE(c)	((const char*)((c)->header+0xAC))	// 4 chars, not terminated
#define GBA_CART_MAKER_CODE(c)	((const char*)((c)->header+0xB0))	// 2 chars, not terminated

// called every 64KB while dumping a ROM
typedef void (*gba_progress_cb)(u32 bytes_done, u32 bytes_total);
// called with each received ROM chunk, return false to abort
typedef bool (*gba_chunk_cb)(const u8 *data, u32 len, void *user);

// payload is idle and ready for a new command
bool gba_is_ready(void);
// checks for a button pressed on the GBA (see common/protocol.h), without
// side effects when there is none; returns true with the request code
// (GBA_REQ_* in the main menu, GBA_CMD_* on the cart screen)
bool gba_poll_request(u32 *code);
// asks the payload for cart info, returns false if no (valid) cart is inserted
bool gba_read_cart_info(gba_cart_info *info);
// sends the chosen command after gba_read_cart_info, GBA_CMD_NONE cancels
void gba_send_command(u32 cmd);

// buf must hold at least chunk_size bytes
bool gba_dump_rom(u32 rom_size, u8 *buf, u32 chunk_size,
	gba_chunk_cb chunk, void *user, gba_progress_cb progress);
// save transfers are split into stages so the caller can report each one
// backup:  gba_wait_save_ready, gba_ack, gba_recv_save
// restore: gba_wait_save_ready, gba_send_save, gba_wait_save_written, gba_ack
// clear:   gba_wait_save_ready, gba_wait_save_written, gba_ack
void gba_wait_save_ready(u32 save_size);
void gba_recv_save(u8 *buf, u32 save_size);
void gba_send_save(const u8 *buf, u32 save_size);
void gba_wait_save_written(void);
void gba_ack(void);
// sends the bios dump command and gives the payload time to react
void gba_start_bios_dump(void);
// buf must hold GBA_BIOS_SIZE bytes
void gba_recv_bios(u8 *buf);

#endif
