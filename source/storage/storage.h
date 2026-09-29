/*
 * Copyright (C) 2016 FIX94
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */
#ifndef __STORAGE_H__
#define __STORAGE_H__

#include <gccore.h>
#include <stddef.h>
#include <time.h>

#define STORAGE_AUTO "auto"

typedef struct
{
	const char *id;		// mount name and settings value, e.g. "sd2" -> "sd2:/"
	const char *name;	// shown to the user
	bool mounted;
} storage_device;

// mounts every device present on this console, returns false if none mounted
bool storage_init(void);

int storage_device_count(void);
const storage_device *storage_device_at(int index);
// returns the device with this id, NULL if it is unknown on this platform
const storage_device *storage_find_device(const char *id);

// selects where dumps go, id is STORAGE_AUTO or a device id
// returns false if that device is not mounted, auto is used instead then
bool storage_select(const char *id);
// NULL when no device is mounted
const storage_device *storage_active(void);
// checks the active device is still inserted, remounting it if needed
bool storage_check(void);

bool storage_dir_exists(const char *path);
// creates path and any missing parent directories
bool storage_mkdirs(const char *path);
bool storage_file_exists(const char *path);
// preallocates a file of the given size
void storage_create_file(const char *path, size_t size);
// reads a whole file into buf if it is exactly expected_size bytes
// returns the file size, or -1 if the file could not be opened
long storage_read_file(const char *path, void *buf, size_t expected_size);

// calls cb for each subdirectory of path, skipping "." and ".."
// returns false if path could not be opened
typedef void (*storage_dir_cb)(const char *name, void *user);
bool storage_list_dirs(const char *path, storage_dir_cb cb, void *user);

// calls cb for each regular file in path with its modification time
// returns false if path could not be opened
typedef void (*storage_file_cb)(const char *name, time_t mtime, void *user);
bool storage_list_files(const char *path, storage_file_cb cb, void *user);

#endif
