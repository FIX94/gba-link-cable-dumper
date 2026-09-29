/*
 * Copyright (C) 2016 FIX94
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */
#ifndef __INI_H__
#define __INI_H__

#include <stdbool.h>

// minimal "key=value" reader, no hardware access so it can be tested on a host
// lines starting with ';' or '#' and [section] headers are ignored,
// whitespace around keys and values is trimmed

typedef void (*ini_entry_cb)(const char *key, const char *value, void *user);

// returns false if the file could not be opened
bool ini_parse_file(const char *path, ini_entry_cb cb, void *user);
// parses a buffer in place, modifying it
void ini_parse_buffer(char *buf, ini_entry_cb cb, void *user);

#endif
