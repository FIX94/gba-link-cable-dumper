/*
 * Copyright (C) 2016 FIX94
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */
#ifndef __PROTOCOL_H__
#define __PROTOCOL_H__

// shared between the GC/Wii side and the GBA multiboot payload

#define APP_VERSION "v1.6"

// commands sent from the GC/Wii to the GBA
#define GBA_CMD_NONE			0
#define GBA_CMD_DUMP_ROM		1
#define GBA_CMD_BACKUP_SAVE		2
#define GBA_CMD_RESTORE_SAVE	3
#define GBA_CMD_CLEAR_SAVE		4
#define GBA_CMD_DUMP_BIOS		5

#define GBA_HEADER_SIZE			0xC0
#define GBA_BIOS_SIZE			0x4000
#define GBA_MAX_SAVE_SIZE		0x20000

#endif
