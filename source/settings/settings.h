/*
 * Copyright (C) 2016 FIX94
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */
#ifndef __SETTINGS_H__
#define __SETTINGS_H__

#include <stdbool.h>
#include <limits.h>
#include "storage/paths.h"

// default folder searched for settings.ini on every device,
// can be changed at build time with "make SETTINGS_DIR=/somewhere"
#ifndef SETTINGS_DIR
#define SETTINGS_DIR "/gbadumper"
#endif
#define SETTINGS_FILE_NAME "settings.ini"

typedef struct
{
	char device[8];				// STORAGE_AUTO or a storage device id
	char dump_dir[PATH_MAX];	// without the device, e.g. "/dumps"
	bool split_folders;
	paths_existing existing;
} settings_t;

settings_t *settings_get(void);
void settings_defaults(settings_t *s);

// finds and loads settings.ini, searching in order:
//  1. next to the dol, if app_path (argv[0]) is known
//  2. SETTINGS_DIR on each mounted device, in storage order
// a file containing "settings_path=<file>" is a pointer and is followed once
// returns false if no settings file was found, defaults are used then
bool settings_load(const char *app_path);
bool settings_save(void);
// full path of the settings file, where it will be saved if none exists yet
const char *settings_file(void);
// moves the settings file into dir (with device, e.g. "sd2:/cfg"),
// leaving a pointer where it was found so the next startup follows it
bool settings_move(const char *dir);

// pushes the settings into storage and paths
// returns false if the chosen device is missing and auto was used instead
bool settings_apply(void);

#endif
