/*
 * Copyright (C) 2016 FIX94
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */
#ifndef __MULTIBOOT_H__
#define __MULTIBOOT_H__

#include <gccore.h>

// blocks until the GBA BIOS is ready to receive
void multiboot_wait_bios(void);
// encrypts and uploads a multiboot image to the GBA
void multiboot_send(const u8 *rom, u32 size);

#endif
