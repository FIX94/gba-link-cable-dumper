/*
 * Copyright (C) 2016 FIX94
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */
#ifndef __VERSION_H__
#define __VERSION_H__

// the only place the version is set, the Makefiles read these two lines
// to put the version into the output file names
#define VERSION_MAJOR 1
#define VERSION_MINOR 9

#define VERSION_STR_(x) #x
#define VERSION_STR(x) VERSION_STR_(x)
// "v1.7"
#define APP_VERSION "v" VERSION_STR(VERSION_MAJOR) "." VERSION_STR(VERSION_MINOR)

#endif
