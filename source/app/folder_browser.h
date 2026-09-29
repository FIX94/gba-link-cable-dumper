/*
 * Copyright (C) 2016 FIX94
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */
#ifndef __FOLDER_BROWSER_H__
#define __FOLDER_BROWSER_H__

#include <stdbool.h>
#include <stddef.h>

// lets the user pick a folder, starting at start (with device, e.g. "sd2:/dumps")
// allow_devices lets the user go above a device root and switch devices
// returns false if cancelled, out is only written on success
bool folder_browser_run(const char *title, const char *start, bool allow_devices, char *out, size_t len);

#endif
