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

// mounts the device dumps are written to, returns false if none is usable
bool storage_init(void);

bool storage_dir_exists(const char *path);
// creates path and any missing parent directories
bool storage_mkdirs(const char *path);
bool storage_file_exists(const char *path);
// preallocates a file of the given size
void storage_create_file(const char *path, size_t size);
// reads a whole file into buf if it is exactly expected_size bytes
// returns the file size, or -1 if the file could not be opened
long storage_read_file(const char *path, void *buf, size_t expected_size);

#endif
