/*
 * Copyright (C) 2016 FIX94
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */
#ifndef __PATHS_H__
#define __PATHS_H__

#include <stddef.h>
#include "link/gba_protocol.h"

#define PATHS_DEFAULT_DUMP_DIR "/dumps"

// folder all dumps are written to, without a trailing slash
void paths_set_dump_dir(const char *dir);
const char *paths_dump_dir(void);

// "<dump dir>/<title> [<game code><maker code>]<ext>"
void paths_cart_file(char *out, size_t len, const gba_cart_info *cart, const char *ext);
void paths_bios_file(char *out, size_t len);

// replaces characters that are invalid in FAT filenames with '_'
void paths_sanitize_filename(char *str);

#endif
