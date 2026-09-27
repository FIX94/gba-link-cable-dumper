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
#include <sdcard/gcsd.h>
#ifdef HW_RVL
#include <sdcard/wiisd_io.h>
#include <ogc/usbstorage.h>
#endif
#include "storage/storage.h"

typedef struct
{
	storage_device dev;
	const DISC_INTERFACE *iface;
} storage_slot;

// in auto selection order
static storage_slot slots[] =
{
#ifdef HW_RVL
	{ { "sd",  "Wii SD Slot",     false }, &__io_wiisd },
	{ { "usb", "USB Storage",     false }, &__io_usbstorage },
#else
	{ { "sd2", "SD2SP2",          false }, &__io_gcsd2 },
#endif
	{ { "sda", "SD Gecko Slot A", false }, &__io_gcsda },
	{ { "sdb", "SD Gecko Slot B", false }, &__io_gcsdb },
};
#define SLOT_COUNT (int)(sizeof(slots)/sizeof(slots[0]))

static storage_slot *active = NULL;

static bool mount_slot(storage_slot *s)
{
	s->dev.mounted = false;
	if(!s->iface->startup() || !s->iface->isInserted())
		return false;
	s->dev.mounted = fatMountSimple(s->dev.id, s->iface);
	return s->dev.mounted;
}

static storage_slot *find_slot(const char *id)
{
	int i;
	for(i = 0; i < SLOT_COUNT; i++)
	{
		if(strcmp(slots[i].dev.id, id) == 0)
			return &slots[i];
	}
	return NULL;
}

bool storage_init(void)
{
	int i;
	bool any = false;
	for(i = 0; i < SLOT_COUNT; i++)
		any |= mount_slot(&slots[i]);
	storage_select(STORAGE_AUTO);
	return any;
}

int storage_device_count(void)
{
	return SLOT_COUNT;
}

const storage_device *storage_device_at(int index)
{
	if(index < 0 || index >= SLOT_COUNT)
		return NULL;
	return &slots[index].dev;
}

const storage_device *storage_find_device(const char *id)
{
	storage_slot *s = find_slot(id);
	return s ? &s->dev : NULL;
}

bool storage_select(const char *id)
{
	storage_slot *s = find_slot(id);
	if(s && s->dev.mounted)
	{
		active = s;
		return true;
	}
	active = NULL;
	int i;
	for(i = 0; i < SLOT_COUNT; i++)
	{
		if(slots[i].dev.mounted)
		{
			active = &slots[i];
			break;
		}
	}
	return strcmp(id, STORAGE_AUTO) == 0;
}

const storage_device *storage_active(void)
{
	return active ? &active->dev : NULL;
}

bool storage_check(void)
{
	if(!active)
		return false;
	if(active->iface->isInserted())
		return true;
	//card was pulled, try again in case it got reinserted
	fatUnmount(active->dev.id);
	return mount_slot(active);
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
	if(len > 1 && tmp[len-1] == '/' && tmp[len-2] != ':')
		tmp[len-1] = '\0';
	char *p;
	for(p = tmp+1; *p; p++)
	{
		if(*p == '/')
		{
			*p = '\0';
			//skip device roots like "sd:"
			if(p[-1] != ':')
				mkdir(tmp, 0777);
			*p = '/';
		}
	}
	mkdir(tmp, 0777);
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

bool storage_list_dirs(const char *path, storage_dir_cb cb, void *user)
{
	DIR *dir = opendir(path);
	if(!dir)
		return false;
	struct dirent *ent;
	while((ent = readdir(dir)) != NULL)
	{
		if(ent->d_type != DT_DIR)
			continue;
		if(strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
			continue;
		cb(ent->d_name, user);
	}
	closedir(dir);
	return true;
}
