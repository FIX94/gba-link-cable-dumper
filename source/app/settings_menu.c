/*
 * Copyright (C) 2016 FIX94
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include "app/settings_menu.h"
#include "app/folder_browser.h"
#include "settings/settings.h"
#include "storage/storage.h"
#include "storage/paths.h"
#include "ui/ui.h"
#include "ui/input.h"

enum
{
	ITEM_DEVICE,
	ITEM_DUMP_DIR,
	ITEM_SPLIT,
	ITEM_EXISTING,
	ITEM_SETTINGS_FILE,
	ITEM_BACK,
	ITEM_COUNT
};

static const char *existing_labels[] = { "Skip", "Overwrite", "Keep both" };

static void device_label(char *out, size_t len, const settings_t *s)
{
	const storage_device *active = storage_active();
	const char *active_name = active ? active->name : "none";
	if(strcmp(s->device, STORAGE_AUTO) == 0)
		snprintf(out, len, "Auto (%s)", active_name);
	else
	{
		const storage_device *d = storage_find_device(s->device);
		if(d && d->mounted)
			snprintf(out, len, "%s", d->name);
		else
			snprintf(out, len, "%s (missing, using %s)", d ? d->name : s->device, active_name);
	}
}

// steps through auto and the mounted devices
static void cycle_device(settings_t *s, int dir)
{
	int count = storage_device_count();
	//index -1 is auto
	int cur = -1;
	int i;
	for(i = 0; i < count; i++)
	{
		if(strcmp(storage_device_at(i)->id, s->device) == 0)
			cur = i;
	}
	for(i = 0; i <= count; i++)
	{
		cur += dir;
		if(cur >= count) cur = -1;
		if(cur < -1) cur = count-1;
		if(cur == -1 || storage_device_at(cur)->mounted)
			break;
	}
	snprintf(s->device, sizeof(s->device), "%s", cur == -1 ? STORAGE_AUTO : storage_device_at(cur)->id);
}

static void choose_dump_dir(settings_t *s)
{
	const storage_device *d = storage_active();
	if(!d)
		return;
	char picked[PATH_MAX];
	if(!folder_browser_run("Choose the output folder", paths_dump_dir(), false, picked, sizeof(picked)))
		return;
	//store without the device, the device is its own setting
	const char *colon = strchr(picked, ':');
	const char *dir = colon ? colon+1 : picked;
	snprintf(s->dump_dir, sizeof(s->dump_dir), "%s", *dir ? dir : "/");
}

static void choose_settings_location(void)
{
	char start[PATH_MAX];
	snprintf(start, sizeof(start), "%s", settings_file());
	char *slash = strrchr(start, '/');
	if(slash)
		*slash = '\0';
	char picked[PATH_MAX];
	if(!folder_browser_run("Choose where settings.ini is stored", start, true, picked, sizeof(picked)))
		return;
	if(!settings_move(picked))
		ui_warn("ERROR: Could not write the settings file there!");
}

void settings_menu_run(void)
{
	settings_t *s = settings_get();
	bool dirty = false;
	bool redraw = true;
	int cursor = 0;
	char device[96], split[8], settings_path[PATH_MAX];
	ui_menu_item items[ITEM_COUNT];
	while(1)
	{
		if(redraw)
		{
			device_label(device, sizeof(device), s);
			snprintf(split, sizeof(split), "%s", s->split_folders ? "On" : "Off");
			snprintf(settings_path, sizeof(settings_path), "%s", settings_file());
			items[ITEM_DEVICE] = (ui_menu_item){ "Storage device", device };
			items[ITEM_DUMP_DIR] = (ui_menu_item){ "Output folder", paths_dump_dir() };
			items[ITEM_SPLIT] = (ui_menu_item){ "ROMs/Saves/BIOS subfolders", split };
			items[ITEM_EXISTING] = (ui_menu_item){ "If a file already exists", existing_labels[s->existing] };
			items[ITEM_SETTINGS_FILE] = (ui_menu_item){ "Settings file", settings_path };
			items[ITEM_BACK] = (ui_menu_item){ "Save and go back", NULL };
			ui_draw_menu("Settings", items, ITEM_COUNT, cursor,
				"A/Right: change  Left: change back  B: save and go back");
			redraw = false;
		}
		input_scan();
		ui_frame();
		u32 btns = input_down();
		int dir = (btns & INPUT_LEFT) ? -1 : ((btns & (INPUT_A|INPUT_RIGHT)) ? 1 : 0);
		if(btns & INPUT_START)
			ui_exit();
		else if(btns & INPUT_B)
			break;
		else if(btns & INPUT_UP)
			cursor = (cursor + ITEM_COUNT - 1) % ITEM_COUNT;
		else if(btns & INPUT_DOWN)
			cursor = (cursor + 1) % ITEM_COUNT;
		else if(dir != 0)
		{
			switch(cursor)
			{
				case ITEM_DEVICE:
					cycle_device(s, dir);
					break;
				case ITEM_DUMP_DIR:
					if(btns & INPUT_A)
						choose_dump_dir(s);
					break;
				case ITEM_SPLIT:
					s->split_folders = !s->split_folders;
					break;
				case ITEM_EXISTING:
					s->existing = (s->existing + 3 + dir) % 3;
					break;
				case ITEM_SETTINGS_FILE:
					if(btns & INPUT_A)
						choose_settings_location();
					break;
				case ITEM_BACK:
					if(btns & INPUT_A)
						goto done;
					break;
			}
			dirty = true;
			//device and folder changes show up straight away
			settings_apply();
		}
		else
			continue;
		redraw = true;
	}
done:
	settings_apply();
	if(dirty && !settings_save())
		ui_warn("ERROR: Could not save settings!");
	if(!paths_create_dirs())
		ui_warn("ERROR: Could not create the output folder!");
}
