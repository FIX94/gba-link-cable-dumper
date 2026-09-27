/*
 * Copyright (C) 2016 FIX94
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include "storage/paths.h"

static char dump_dir[PATH_MAX] = PATHS_DEFAULT_DUMP_DIR;

void paths_set_dump_dir(const char *dir)
{
	snprintf(dump_dir, sizeof(dump_dir), "%s", dir);
	size_t len = strlen(dump_dir);
	if(len > 1 && dump_dir[len-1] == '/')
		dump_dir[len-1] = '\0';
}

const char *paths_dump_dir(void)
{
	return dump_dir;
}

void paths_cart_file(char *out, size_t len, const gba_cart_info *cart, const char *ext)
{
	int dirlen = snprintf(out, len, "%s/", dump_dir);
	if(dirlen < 0 || (size_t)dirlen >= len)
		return;
	snprintf(out+dirlen, len-dirlen, "%.12s [%.4s%.2s]%s",
		GBA_CART_TITLE(cart), GBA_CART_GAME_CODE(cart), GBA_CART_MAKER_CODE(cart), ext);
	paths_sanitize_filename(out+dirlen); //fix name behind the dump dir
}

void paths_bios_file(char *out, size_t len)
{
	snprintf(out, len, "%s/gba_bios.bin", dump_dir);
}

void paths_sanitize_filename(char *str)
{
	for(; *str; ++str)
	{
		if(*str < 0x20 || *str > 0x7F)
			*str = '_';
		else switch(*str)
		{
			case '\\':
			case '/':
			case ':':
			case '*':
			case '?':
			case '\"':
			case '<':
			case '>':
			case '|':
				*str = '_';
				break;
			default:
				break;
		}
	}
}
