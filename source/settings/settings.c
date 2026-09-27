/*
 * Copyright (C) 2016 FIX94
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include "settings/ini.h"
#include "settings/settings.h"
#include "storage/storage.h"
#include "storage/paths.h"

#define POINTER_KEY "settings_path"

static settings_t settings;
// the real settings file
static char settings_path[PATH_MAX];
// where the search found it, differs from settings_path when a pointer was followed
static char found_path[PATH_MAX];

static const char *existing_names[] = { "skip", "overwrite", "keep_both" };

typedef struct
{
	settings_t *s;
	char pointer[PATH_MAX];
} load_ctx;

static bool parse_bool(const char *v)
{
	return strcmp(v, "1") == 0 || strcasecmp(v, "yes") == 0 ||
		strcasecmp(v, "true") == 0 || strcasecmp(v, "on") == 0;
}

static void load_entry(const char *key, const char *value, void *user)
{
	load_ctx *ctx = user;
	settings_t *s = ctx->s;
	if(strcmp(key, "device") == 0)
	{
		if(strcmp(value, STORAGE_AUTO) == 0 || storage_find_device(value))
			snprintf(s->device, sizeof(s->device), "%s", value);
	}
	else if(strcmp(key, "dump_dir") == 0)
	{
		if(*value)
			snprintf(s->dump_dir, sizeof(s->dump_dir), "%s%s", value[0] == '/' ? "" : "/", value);
	}
	else if(strcmp(key, "split_folders") == 0)
		s->split_folders = parse_bool(value);
	else if(strcmp(key, "existing_files") == 0)
	{
		int i;
		for(i = 0; i < 3; i++)
		{
			if(strcasecmp(value, existing_names[i]) == 0)
				s->existing = i;
		}
	}
	else if(strcmp(key, POINTER_KEY) == 0)
		snprintf(ctx->pointer, sizeof(ctx->pointer), "%s", value);
}

// device id from "id:/..." if it is one of ours and mounted
static bool path_device_mounted(const char *path)
{
	const char *colon = strchr(path, ':');
	if(!colon || colon[1] != '/' || colon - path >= 8)
		return false;
	char id[8];
	memcpy(id, path, colon - path);
	id[colon - path] = '\0';
	const storage_device *d = storage_find_device(id);
	return d && d->mounted;
}

static void default_file_on(char *out, size_t len, const char *device_id)
{
	snprintf(out, len, "%s:" SETTINGS_DIR "/" SETTINGS_FILE_NAME, device_id);
}

static void parent_dir(char *out, size_t len, const char *path)
{
	snprintf(out, len, "%s", path);
	char *slash = strrchr(out, '/');
	if(slash)
	{
		//keep the slash of a device root like "sd:/"
		if(slash > out && slash[-1] == ':')
			slash[1] = '\0';
		else
			*slash = '\0';
	}
}

static bool try_load(const char *path)
{
	load_ctx ctx;
	ctx.s = &settings;
	ctx.pointer[0] = '\0';
	if(!ini_parse_file(path, load_entry, &ctx))
		return false;
	snprintf(found_path, sizeof(found_path), "%s", path);
	snprintf(settings_path, sizeof(settings_path), "%s", path);
	if(ctx.pointer[0] && strcmp(ctx.pointer, path) != 0)
	{
		//follow the pointer once, a missing target is recreated on save
		snprintf(settings_path, sizeof(settings_path), "%s", ctx.pointer);
		if(path_device_mounted(ctx.pointer))
		{
			ctx.pointer[0] = '\0';
			ini_parse_file(settings_path, load_entry, &ctx);
		}
	}
	return true;
}

settings_t *settings_get(void)
{
	return &settings;
}

void settings_defaults(settings_t *s)
{
	snprintf(s->device, sizeof(s->device), STORAGE_AUTO);
	snprintf(s->dump_dir, sizeof(s->dump_dir), PATHS_DEFAULT_DUMP_DIR);
	s->split_folders = false;
	s->existing = PATHS_EXISTING_SKIP;
}

bool settings_load(const char *app_path)
{
	char path[PATH_MAX];
	settings_defaults(&settings);
	settings_path[0] = '\0';
	found_path[0] = '\0';
	//1. next to the dol
	if(app_path && path_device_mounted(app_path))
	{
		char dir[PATH_MAX];
		parent_dir(dir, sizeof(dir), app_path);
		size_t dirlen = strlen(dir);
		int n = snprintf(path, sizeof(path), "%s%s" SETTINGS_FILE_NAME, dir,
			(dirlen > 0 && dir[dirlen-1] == '/') ? "" : "/");
		if(n > 0 && (size_t)n < sizeof(path) && try_load(path))
			return true;
	}
	//2. default folder on each device
	int i;
	for(i = 0; i < storage_device_count(); i++)
	{
		const storage_device *d = storage_device_at(i);
		if(!d->mounted)
			continue;
		default_file_on(path, sizeof(path), d->id);
		if(try_load(path))
			return true;
	}
	return false;
}

static bool ensure_settings_path(void)
{
	if(settings_path[0])
		return true;
	const storage_device *d = storage_active();
	if(!d)
		return false;
	default_file_on(settings_path, sizeof(settings_path), d->id);
	return true;
}

static FILE *open_for_write(const char *path)
{
	char dir[PATH_MAX];
	parent_dir(dir, sizeof(dir), path);
	storage_mkdirs(dir);
	return fopen(path, "w");
}

static bool write_pointer(const char *at, const char *target)
{
	FILE *f = open_for_write(at);
	if(!f)
		return false;
	fprintf(f, "; GBA Link Cable Dumper settings were moved, see:\n");
	fprintf(f, POINTER_KEY "=%s\n", target);
	fclose(f);
	return true;
}

bool settings_save(void)
{
	if(!ensure_settings_path())
		return false;
	FILE *f = open_for_write(settings_path);
	if(!f)
		return false;
	fprintf(f, "; GBA Link Cable Dumper settings\n");
	fprintf(f, "; device: auto");
	int i;
	for(i = 0; i < storage_device_count(); i++)
		fprintf(f, ", %s", storage_device_at(i)->id);
	fprintf(f, "\ndevice=%s\n", settings.device);
	fprintf(f, "; dump_dir: folder on the device, starting with /\n");
	fprintf(f, "dump_dir=%s\n", settings.dump_dir);
	fprintf(f, "; split_folders: 1 puts files into ROMs/, Saves/ and BIOS/\n");
	fprintf(f, "split_folders=%d\n", settings.split_folders ? 1 : 0);
	fprintf(f, "; existing_files: skip, overwrite or keep_both\n");
	fprintf(f, "existing_files=%s\n", existing_names[settings.existing]);
	fclose(f);
	if(!found_path[0])
		snprintf(found_path, sizeof(found_path), "%s", settings_path);
	return true;
}

const char *settings_file(void)
{
	ensure_settings_path();
	return settings_path;
}

bool settings_move(const char *dir)
{
	char target[PATH_MAX];
	size_t dirlen = strlen(dir);
	int n = snprintf(target, sizeof(target), "%s%s" SETTINGS_FILE_NAME, dir,
		(dirlen > 0 && dir[dirlen-1] == '/') ? "" : "/");
	if(n < 0 || (size_t)n >= sizeof(target))
		return false;
	char old[PATH_MAX];
	snprintf(old, sizeof(old), "%s", settings_path);
	snprintf(settings_path, sizeof(settings_path), "%s", target);
	if(!settings_save())
	{
		snprintf(settings_path, sizeof(settings_path), "%s", old);
		return false;
	}
	//leave pointers where the search will look, so the next start finds it
	if(!found_path[0] || strcmp(found_path, target) == 0)
	{
		const storage_device *d = storage_active();
		if(d)
			default_file_on(found_path, sizeof(found_path), d->id);
	}
	if(strcmp(found_path, target) != 0)
		write_pointer(found_path, target);
	if(old[0] && strcmp(old, target) != 0 && strcmp(old, found_path) != 0)
		write_pointer(old, target);
	return true;
}

bool settings_apply(void)
{
	bool ok = storage_select(settings.device);
	const storage_device *d = storage_active();
	if(d)
	{
		char full[PATH_MAX];
		int n = snprintf(full, sizeof(full), "%s:%s", d->id, settings.dump_dir);
		//fall back to the default folder rather than a cut off path
		if(n < 0 || (size_t)n >= sizeof(full))
			snprintf(full, sizeof(full), "%s:" PATHS_DEFAULT_DUMP_DIR, d->id);
		paths_set_dump_dir(full);
	}
	paths_set_split_folders(settings.split_folders);
	return ok && d;
}
