/*
 * Copyright (C) 2016 FIX94
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */
#ifndef __PATHS_H__
#define __PATHS_H__

#include <stddef.h>
#include <stdbool.h>
#include "link/gba_protocol.h"

#define PATHS_DEFAULT_DUMP_DIR "/dumps"

typedef enum
{
	PATHS_KIND_ROM,
	PATHS_KIND_SAVE,
	PATHS_KIND_BIOS,
} paths_kind;

typedef enum
{
	PATHS_EXISTING_SKIP,		// refuse to write, the original behaviour
	PATHS_EXISTING_OVERWRITE,
	PATHS_EXISTING_KEEP_BOTH,	// write "name (1).ext", "name (2).ext", ...
} paths_existing;

// folder all dumps are written to including the device, e.g. "sd2:/dumps"
void paths_set_dump_dir(const char *dir);
const char *paths_dump_dir(void);
// put ROMs, saves and the BIOS into ROMs/, Saves/ and BIOS/ subfolders
void paths_set_split_folders(bool split);
// the folder files of this kind go to
void paths_kind_dir(char *out, size_t len, paths_kind kind);
// creates the dump folder and any subfolders, returns false on failure
bool paths_create_dirs(void);

// "<kind dir>/<title> [<game code><maker code>]<ext>"
void paths_cart_file(char *out, size_t len, const gba_cart_info *cart, paths_kind kind);
void paths_bios_file(char *out, size_t len);

// applies the policy to a path that is about to be written
// returns false if nothing should be written, may change path for KEEP_BOTH
bool paths_resolve_existing(char *path, size_t len, paths_existing policy);

// replaces characters that are invalid in FAT filenames with '_'
void paths_sanitize_filename(char *str);

#endif
