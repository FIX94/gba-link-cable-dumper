/*
 * Copyright (C) 2016 FIX94
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */
#include <gccore.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <limits.h>
#include <sys/stat.h>
#include <fat.h>
#include "storage/storage.h"

bool storage_init(void)
{
	return fatInitDefault();
}

bool storage_dir_exists(const char *path)
{
	DIR *dir;
	dir = opendir(path);
	if(dir)
	{
		closedir(dir);
		return true;
	}
	return false;
}

bool storage_mkdirs(const char *path)
{
	char tmp[PATH_MAX];
	size_t len = strlen(path);
	if(len == 0 || len >= sizeof(tmp))
		return false;
	strcpy(tmp, path);
	//strip trailing slash
	if(len > 1 && tmp[len-1] == '/')
		tmp[len-1] = '\0';
	char *p;
	for(p = tmp+1; *p; p++)
	{
		if(*p == '/')
		{
			*p = '\0';
			//skip device roots like "sd:"
			if(p[-1] != ':')
				mkdir(tmp, S_IREAD | S_IWRITE);
			*p = '/';
		}
	}
	mkdir(tmp, S_IREAD | S_IWRITE);
	return storage_dir_exists(tmp);
}

bool storage_file_exists(const char *path)
{
	FILE *f = fopen(path,"rb");
	if(f)
	{
		fclose(f);
		return true;
	}
	return false;
}

void storage_create_file(const char *path, size_t size)
{
	int fd = open(path, O_WRONLY|O_CREAT, 0666);
	if(fd >= 0)
	{
		ftruncate(fd, size);
		close(fd);
	}
}

long storage_read_file(const char *path, void *buf, size_t expected_size)
{
	FILE *f = fopen(path,"rb");
	if(!f)
		return -1;
	fseek(f,0,SEEK_END);
	long readsize = ftell(f);
	if(readsize == (long)expected_size)
	{
		rewind(f);
		fread(buf,readsize,1,f);
	}
	fclose(f);
	return readsize;
}
