/*
 * Copyright (C) 2016 FIX94
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "settings/ini.h"

// settings files are tiny, anything bigger is not ours
#define INI_MAX_SIZE 0x4000

static char *trim(char *s)
{
	while(isspace((unsigned char)*s))
		s++;
	char *end = s + strlen(s);
	while(end > s && isspace((unsigned char)end[-1]))
		end--;
	*end = '\0';
	return s;
}

void ini_parse_buffer(char *buf, ini_entry_cb cb, void *user)
{
	char *line = buf;
	while(line && *line)
	{
		char *next = strpbrk(line, "\r\n");
		if(next)
			*next++ = '\0';
		char *s = trim(line);
		char *eq = strchr(s, '=');
		if(*s != ';' && *s != '#' && *s != '[' && eq)
		{
			*eq = '\0';
			char *key = trim(s);
			if(*key)
				cb(key, trim(eq+1), user);
		}
		line = next;
	}
}

bool ini_parse_file(const char *path, ini_entry_cb cb, void *user)
{
	FILE *f = fopen(path, "rb");
	if(!f)
		return false;
	fseek(f, 0, SEEK_END);
	long size = ftell(f);
	rewind(f);
	if(size < 0 || size > INI_MAX_SIZE)
	{
		fclose(f);
		return false;
	}
	char *buf = malloc(size+1);
	if(!buf)
	{
		fclose(f);
		return false;
	}
	size = fread(buf, 1, size, f);
	buf[size] = '\0';
	fclose(f);
	ini_parse_buffer(buf, cb, user);
	free(buf);
	return true;
}
