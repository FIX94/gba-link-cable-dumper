/*
 * Copyright (C) 2016 FIX94
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */
#include <gccore.h>
#include <stdio.h>
#include <malloc.h>
#include <unistd.h>
#include <limits.h>
#include "protocol.h"
#include "gba_mb_gba.h"
#include "app/app.h"
#include "app/settings_menu.h"
#include "link/si_link.h"
#include "link/multiboot.h"
#include "link/gba_protocol.h"
#include "storage/storage.h"
#include "storage/paths.h"
#include "settings/settings.h"
#include "ui/ui.h"
#include "ui/input.h"

// received data is buffered in chunks of this size before being written out
#define DUMP_BUF_SIZE 0x400000

static u8 *dumpbuf;

static bool write_chunk(const u8 *data, u32 len, void *user)
{
	fwrite(data,len,1,(FILE*)user);
	return true;
}

static void show_storage(void)
{
	const storage_device *d = storage_active();
	ui_show_storage(d ? d->name : "none", paths_dump_dir());
}

// checks the device is still there before writing to it
static bool check_storage(void)
{
	if(storage_check())
		return true;
	ui_warn("ERROR: Storage device not found, reinsert it and try again!\n");
	return false;
}

// waits for a GBA on port 2 and uploads the multiboot payload,
// returns false if something other than a GBA was found
static bool connect_gba(void)
{
	u32 type;
	bool redraw = true;
	si_link_probe_start();
	while((type = si_link_probe_poll()) == 0)
	{
		if(redraw)
		{
			ui_clear();
			show_storage();
			ui_show_waiting_help();
			ui_status("Waiting for a GBA in port 2...\n");
			redraw = false;
		}
		input_scan();
		ui_frame();
		u32 btns = input_down();
		if(btns&INPUT_START)
			ui_exit();
		else if(btns&INPUT_X)
		{
			settings_menu_run();
			redraw = true;
		}
	}
	if(!(type & SI_GBA))
		return false;
	ui_status("GBA Found! Waiting on BIOS\n");
	multiboot_wait_bios();
	ui_status("Ready, sending dumper\n");
	multiboot_send(gba_mb_gba, gba_mb_gba_size);
	ui_status("Done!\n");
	sleep(2);
	return true;
}

static const char *command_name(u32 cmd)
{
	switch(cmd)
	{
		case GBA_CMD_DUMP_ROM:		return "Dump ROM";
		case GBA_CMD_BACKUP_SAVE:	return "Back up save";
		case GBA_CMD_RESTORE_SAVE:	return "Restore save";
		case GBA_CMD_CLEAR_SAVE:	return "Clear save";
		default:					return "Cancel";
	}
}

static u32 choose_cart_command(const gba_cart_info *cart)
{
	while(1)
	{
		input_scan();
		ui_frame();
		u32 btns = input_down();
		u32 req;
		if(!btns && gba_poll_request(&req))
		{
			bool save_cmd = req == GBA_CMD_BACKUP_SAVE || req == GBA_CMD_RESTORE_SAVE || req == GBA_CMD_CLEAR_SAVE;
			if(req <= GBA_CMD_CLEAR_SAVE && (!save_cmd || cart->save_size > 0))
			{
				ui_status("Chosen on the GBA: %s\n", command_name(req));
				return req;
			}
		}
		if(btns&INPUT_START)
			ui_exit();
		else if(btns&INPUT_A)
			return GBA_CMD_DUMP_ROM;
		else if(btns&INPUT_B)
			return GBA_CMD_NONE;
		else if(cart->save_size > 0)
		{
			if(btns&INPUT_Y)
				return GBA_CMD_BACKUP_SAVE;
			else if(btns&INPUT_X)
				return GBA_CMD_RESTORE_SAVE;
			else if(btns&INPUT_Z)
				return GBA_CMD_CLEAR_SAVE;
		}
	}
}

// checks the command can run before the GBA is told about it,
// returns GBA_CMD_NONE if it can not
static u32 prepare_cart_command(u32 command, const gba_cart_info *cart,
	char *gamename, char *savename)
{
	if(command != GBA_CMD_NONE && command != GBA_CMD_CLEAR_SAVE && !check_storage())
		return GBA_CMD_NONE;
	paths_existing policy = settings_get()->existing;
	if(command == GBA_CMD_DUMP_ROM)
	{
		if(!paths_resolve_existing(gamename, PATH_MAX, policy))
		{
			ui_warn("ERROR: Game already dumped!\n");
			return GBA_CMD_NONE;
		}
	}
	else if(command == GBA_CMD_BACKUP_SAVE)
	{
		if(!paths_resolve_existing(savename, PATH_MAX, policy))
		{
			ui_warn("ERROR: Save already backed up!\n");
			return GBA_CMD_NONE;
		}
	}
	else if(command == GBA_CMD_RESTORE_SAVE)
	{
		long readsize = storage_read_file(savename, dumpbuf, cart->save_size);
		if(readsize < 0)
		{
			ui_warn("ERROR: No Save to restore!\n");
			return GBA_CMD_NONE;
		}
		if(readsize != cart->save_size)
		{
			ui_warn("ERROR: Save has the wrong size, aborting restore!\n");
			return GBA_CMD_NONE;
		}
	}
	return command;
}

static void dump_rom(const gba_cart_info *cart, const char *gamename)
{
	//create base file with size
	ui_status("Preparing file...\n");
	storage_create_file(gamename,cart->rom_size);
	FILE *f = fopen(gamename,"wb");
	if(!f)
		ui_fatal("ERROR: Could not create file! Exit...");
	ui_status("Dumping...\n");
	gba_dump_rom(cart->rom_size, dumpbuf, DUMP_BUF_SIZE, write_chunk, f, ui_rom_progress);
	ui_status("\nClosing file\n");
	fclose(f);
	ui_status("Game dumped!\n");
	sleep(5);
}

static void backup_save(const gba_cart_info *cart, const char *savename)
{
	//create base file with size
	ui_status("Preparing file...\n");
	storage_create_file(savename,cart->save_size);
	FILE *f = fopen(savename,"wb");
	if(!f)
		ui_fatal("ERROR: Could not create file! Exit...");
	ui_status("Waiting for GBA\n");
	ui_frame();
	gba_wait_save_ready(cart->save_size);
	gba_ack(); //got savesize
	ui_status("Receiving...\n");
	gba_recv_save(dumpbuf, cart->save_size);
	ui_status("Writing save...\n");
	fwrite(dumpbuf,cart->save_size,1,f);
	fclose(f);
	ui_status("Save backed up!\n");
	sleep(5);
}

// restore sends the save loaded by prepare_cart_command, clear sends nothing
static void write_save(u32 command, const gba_cart_info *cart)
{
	gba_wait_save_ready(cart->save_size);
	if(command == GBA_CMD_RESTORE_SAVE)
	{
		ui_status("Sending save\n");
		ui_frame();
		gba_send_save(dumpbuf, cart->save_size);
	}
	ui_status("Waiting for GBA\n");
	gba_wait_save_written();
	ui_status(command == GBA_CMD_RESTORE_SAVE ? "Save restored!\n" : "Save cleared!\n");
	gba_ack();
	sleep(5);
}

static void handle_cart(void)
{
	ui_status("Waiting for GBA\n");
	ui_frame();
	gba_cart_info cart;
	if(!gba_read_cart_info(&cart))
	{
		ui_warn("ERROR: No (Valid) GBA Card inserted!\n");
		return;
	}
	ui_show_cart_info(&cart);
	char gamename[PATH_MAX];
	char savename[PATH_MAX];
	paths_cart_file(gamename, sizeof(gamename), &cart, PATHS_KIND_ROM);
	paths_cart_file(savename, sizeof(savename), &cart, PATHS_KIND_SAVE);
	ui_show_cart_menu(&cart);
	u32 command = choose_cart_command(&cart);
	command = prepare_cart_command(command, &cart, gamename, savename);
	gba_send_command(command);
	//let gba prepare
	sleep(1);
	switch(command)
	{
		case GBA_CMD_DUMP_ROM:
			dump_rom(&cart, gamename);
			break;
		case GBA_CMD_BACKUP_SAVE:
			backup_save(&cart, savename);
			break;
		case GBA_CMD_RESTORE_SAVE:
		case GBA_CMD_CLEAR_SAVE:
			write_save(command, &cart);
			break;
		default:
			break;
	}
}

static void dump_bios(void)
{
	char biosname[PATH_MAX];
	paths_bios_file(biosname, sizeof(biosname));
	if(!check_storage())
		return;
	if(!paths_resolve_existing(biosname, sizeof(biosname), settings_get()->existing))
	{
		ui_warn("ERROR: BIOS already backed up!\n");
		return;
	}
	//create base file with size
	ui_status("Preparing file...\n");
	storage_create_file(biosname,GBA_BIOS_SIZE);
	FILE *f = fopen(biosname,"wb");
	if(!f)
		ui_fatal("ERROR: Could not create file! Exit...");
	gba_start_bios_dump();
	//lets go!
	ui_status("Dumping...\n");
	gba_recv_bios(dumpbuf);
	fwrite(dumpbuf,GBA_BIOS_SIZE,1,f);
	ui_status("Closing file\n");
	fclose(f);
	ui_status("BIOS dumped!\n");
	sleep(5);
}

// main menu once the payload is running on the GBA, never returns
static void run_menu(void)
{
	while(1)
	{
		ui_clear();
		show_storage();
		ui_show_main_menu();
		input_scan();
		ui_frame();
		u32 btns = input_down();
		u32 req = 0;
		if(!btns && !gba_poll_request(&req))
			req = 0;
		if(btns&INPUT_START)
			ui_exit();
		else if(btns&INPUT_X)
			settings_menu_run();
		else if((btns&INPUT_A) || req == GBA_REQ_READ_CART)
		{
			if(gba_is_ready())
				handle_cart();
		}
		else if((btns&INPUT_Y) || req == GBA_REQ_DUMP_BIOS)
			dump_bios();
	}
}

void app_run(int argc, char *argv[])
{
	si_link_init();
	dumpbuf = memalign(32,DUMP_BUF_SIZE);
	if(!dumpbuf) return;
	if(!storage_init())
	{
		ui_clear();
		ui_fatal("ERROR: No usable device found to write dumped files to!");
	}
	settings_load(argc > 0 ? argv[0] : NULL);
	if(!settings_apply())
	{
		ui_clear();
		ui_status("Storage device \"%s\" not found, using %s instead.\n",
			settings_get()->device, storage_active()->name);
		sleep(3);
	}
	if(!paths_create_dirs())
	{
		ui_clear();
		ui_fatal("ERROR: Could not create dumps folder, make sure you have a supported device connected!");
	}
	while(1)
	{
		if(connect_gba())
			run_menu();
	}
}
