/*
 * Copyright (C) 2016 FIX94
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */
#ifndef __PROTOCOL_H__
#define __PROTOCOL_H__

// shared between the GC/Wii side and the GBA multiboot payload

// commands sent from the GC/Wii to the GBA
#define GBA_CMD_NONE			0
#define GBA_CMD_DUMP_ROM		1
#define GBA_CMD_BACKUP_SAVE		2
#define GBA_CMD_RESTORE_SAVE	3
#define GBA_CMD_CLEAR_SAVE		4
#define GBA_CMD_DUMP_BIOS		5

// requests from the GBA, posted when a button is pressed on the GBA:
// the GBA puts GBA_REQUEST_MAGIC|code into JOYTR and sets GBA_JSTAT_REQUEST,
// a general purpose JOYSTAT bit the GC sees in the status reply without
// side effects. The GC reads the request, waits for the GBA to clear the
// bit (it resets JOYTR to 0 first), then acts on it like a button press.
// Choices on the cart screen use the GBA_CMD_* values as the code.
#define GBA_JSTAT_REQUEST		0x20
#define GBA_REQUEST_MAGIC		0x47420000	// "GB" in the top half
#define GBA_REQUEST_MASK		0xFFFF0000
#define GBA_REQ_READ_CART		0x10
#define GBA_REQ_DUMP_BIOS		0x11

#define GBA_HEADER_SIZE			0xC0
#define GBA_BIOS_SIZE			0x4000
#define GBA_MAX_SAVE_SIZE		0x20000

#endif
