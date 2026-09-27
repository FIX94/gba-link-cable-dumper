/*
 * Copyright (C) 2016 FIX94
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include "storage/storage.h"
#include "storage/paths.h"

static char dump_dir[PATH_MAX] = PATHS_DEFAULT_DUMP_DIR;
static bool split_folders = false;

static const char *kind_subdir[] = { "ROMs", "Saves", "BIOS" };
static const char *kind_ext[] = { ".gba", ".sav", ".bin" };

void paths_set_dump_dir(const char *dir)
{
	snprintf(dump_dir, sizeof(dump_dir), "%s", dir);
	size_t len = strlen(dump_dir);
	//keep the slash of a device root like "sd:/"
	if(len > 1 && dump_dir[len-1] == '/' && dump_dir[len-2] != ':')
		dump_dir[len-1] = '\0';
}

const char *paths_dump_dir(void)
{
	return dump_dir;
}

void paths_set_split_folders(bool split)
{
	split_folders = split;
}

void paths_kind_dir(char *out, size_t len, paths_kind kind)
{
	size_t dirlen = strlen(dump_dir);
	const char *sep = (dirlen > 0 && dump_dir[dirlen-1] == '/') ? "" : "/";
	int n;
	if(split_folders)
		n = snprintf(out, len, "%s%s%s", dump_dir, sep, kind_subdir[kind]);
	else
		n = snprintf(out, len, "%s", dump_dir);
	//a cut off path would point somewhere else, leave it empty so writes fail
	if(n < 0 || (size_t)n >= len)
		out[0] = '\0';
}

bool paths_create_dirs(void)
{
	if(!storage_mkdirs(dump_dir))
		return false;
	if(split_folders)
	{
		char dir[PATH_MAX];
		int kind;
		for(kind = PATHS_KIND_ROM; kind <= PATHS_KIND_BIOS; kind++)
		{
			paths_kind_dir(dir, sizeof(dir), kind);
			if(!storage_mkdirs(dir))
				return false;
		}
	}
	return true;
}

static int kind_dir_prefix(char *out, size_t len, paths_kind kind)
{
	paths_kind_dir(out, len, kind);
	size_t dirlen = strlen(out);
	if(dirlen > 0 && out[dirlen-1] != '/' && dirlen+1 < len)
	{
		out[dirlen++] = '/';
		out[dirlen] = '\0';
	}
	return dirlen;
}

void paths_cart_file(char *out, size_t len, const gba_cart_info *cart, paths_kind kind)
{
	int dirlen = kind_dir_prefix(out, len, kind);
	if((size_t)dirlen >= len)
		return;
	snprintf(out+dirlen, len-dirlen, "%.12s [%.4s%.2s]%s",
		GBA_CART_TITLE(cart), GBA_CART_GAME_CODE(cart), GBA_CART_MAKER_CODE(cart), kind_ext[kind]);
	paths_sanitize_filename(out+dirlen); //fix name behind the dump dir
}

void paths_bios_file(char *out, size_t len)
{
	int dirlen = kind_dir_prefix(out, len, PATHS_KIND_BIOS);
	if((size_t)dirlen >= len)
		return;
	snprintf(out+dirlen, len-dirlen, "gba_bios.bin");
}

bool paths_resolve_existing(char *path, size_t len, paths_existing policy)
{
	if(!storage_file_exists(path))
		return true;
	if(policy == PATHS_EXISTING_SKIP)
		return false;
	if(policy == PATHS_EXISTING_OVERWRITE)
		return true;
	//keep both, find a free "name (n).ext"
	char base[PATH_MAX];
	snprintf(base, sizeof(base), "%s", path);
	char *slash = strrchr(base, '/');
	char *dot = strrchr(base, '.');
	char ext[16] = "";
	if(dot && (!slash || dot > slash))
	{
		snprintf(ext, sizeof(ext), "%s", dot);
		*dot = '\0';
	}
	int n;
	for(n = 1; n < 1000; n++)
	{
		snprintf(path, len, "%s (%d)%s", base, n, ext);
		if(!storage_file_exists(path))
			return true;
	}
	return false;
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
