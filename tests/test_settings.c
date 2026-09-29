/*
 * Host tests for settings, ini, paths and storage.
 * Builds the real sources against the stub libogc headers in tests/stub,
 * device roots like "sd2:/" are plain folders in a temporary directory.
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <utime.h>
#include <time.h>
#include "settings/settings.h"
#include "settings/ini.h"
#include "storage/storage.h"
#include "storage/paths.h"
bool stub_present_sd2 = true, stub_present_sda = true;
static int fails = 0;
#define CHECK(c) do { if(!(c)) { printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #c); fails++; } else printf("ok   %s\n", #c); } while(0)
typedef struct { int count; char keys[8][16]; char values[8][16]; } ini_result;
static void ini_collect(const char *k, const char *v, void *u)
{
	ini_result *r = u;
	if(r->count < 8) { snprintf(r->keys[r->count], 16, "%s", k); snprintf(r->values[r->count], 16, "%s", v); r->count++; }
}
static void slurp(const char *p, char *out, size_t n){ FILE*f=fopen(p,"r"); size_t r=f?fread(out,1,n-1,f):0; out[r]=0; if(f)fclose(f);}
int main(void){
	char buf[4096];
	// ini parser: comments, sections, whitespace, CRLF, malformed lines, no trailing newline
	char ini[] = "; comment\n[section]\n  device = sd2  \r\ndump_dir=/my dumps\n#x=y\nnoequals\n=novalue\nempty=\nlast=nolf";
	ini_result r = {0};
	ini_parse_buffer(ini, ini_collect, &r);
	CHECK(r.count == 4);
	CHECK(strcmp(r.keys[0], "device") == 0 && strcmp(r.values[0], "sd2") == 0);
	CHECK(strcmp(r.values[1], "/my dumps") == 0);
	CHECK(strcmp(r.keys[2], "empty") == 0 && r.values[2][0] == '\0');
	CHECK(strcmp(r.keys[3], "last") == 0 && strcmp(r.values[3], "nolf") == 0);
	mkdir("sd2:", 0777); mkdir("sda:", 0777); mkdir("sdb:", 0777);
	settings_t *s = settings_get();
	storage_init();
	// first run: nothing found, defaults, auto picks sd2 (first in order)
	CHECK(!settings_load("sda:/apps/gbadumper/boot.dol"));
	CHECK(settings_apply());
	CHECK(strcmp(storage_active()->id, "sd2") == 0);
	CHECK(strcmp(paths_dump_dir(), "sd2:/dumps") == 0);
	CHECK(strcmp(settings_file(), "sd2:/gbadumper/settings.ini") == 0);
	// save + reload round trip
	s->split_folders = true; s->existing = PATHS_EXISTING_KEEP_BOTH; strcpy(s->dump_dir, "/gba/dumps");
	CHECK(settings_save());
	settings_defaults(s);
	CHECK(settings_load(NULL));
	CHECK(s->split_folders && s->existing == PATHS_EXISTING_KEEP_BOTH && strcmp(s->dump_dir, "/gba/dumps") == 0);
	settings_apply();
	char p[1024]; paths_kind_dir(p, sizeof p, PATHS_KIND_SAVE);
	CHECK(strcmp(p, "sd2:/gba/dumps/Saves") == 0);
	CHECK(paths_create_dirs());
	CHECK(storage_dir_exists("sd2:/gba/dumps/ROMs"));
	// app dir wins over device default
	storage_mkdirs("sda:/apps/gbadumper");
	FILE *f = fopen("sda:/apps/gbadumper/settings.ini","w"); fprintf(f, "device=sda\n"); fclose(f);
	CHECK(settings_load("sda:/apps/gbadumper/boot.dol"));
	CHECK(strcmp(s->device, "sda") == 0 && strcmp(settings_file(), "sda:/apps/gbadumper/settings.ini") == 0);
	remove("sda:/apps/gbadumper/settings.ini");
	// move settings to a custom folder, pointer is left at the default location
	CHECK(settings_load(NULL));
	CHECK(settings_move("sda:/custom/cfg"));
	CHECK(strcmp(settings_file(), "sda:/custom/cfg/settings.ini") == 0);
	slurp("sd2:/gbadumper/settings.ini", buf, sizeof buf);
	CHECK(strstr(buf, "settings_path=sda:/custom/cfg/settings.ini") != NULL);
	// next start follows the pointer
	settings_defaults(s);
	CHECK(settings_load(NULL));
	CHECK(strcmp(settings_file(), "sda:/custom/cfg/settings.ini") == 0 && s->split_folders);
	// move again: both the default pointer and old real file must point at the new place (no 2-hop chain)
	CHECK(settings_move("sd2:/other"));
	slurp("sd2:/gbadumper/settings.ini", buf, sizeof buf);
	CHECK(strstr(buf, "settings_path=sd2:/other/settings.ini") != NULL);
	slurp("sda:/custom/cfg/settings.ini", buf, sizeof buf);
	CHECK(strstr(buf, "settings_path=sd2:/other/settings.ini") != NULL);
	settings_defaults(s);
	CHECK(settings_load(NULL) && strcmp(settings_file(), "sd2:/other/settings.ini") == 0 && s->split_folders);
	// move back to the default location: real file replaces the pointer
	CHECK(settings_move("sd2:/gbadumper"));
	slurp("sd2:/gbadumper/settings.ini", buf, sizeof buf);
	CHECK(strstr(buf, "settings_path") == NULL && strstr(buf, "device=") != NULL);
	settings_defaults(s);
	CHECK(settings_load(NULL) && strcmp(settings_file(), "sd2:/gbadumper/settings.ini") == 0 && s->split_folders);
	// chosen device missing -> falls back
	strcpy(s->device, "sdb");
	CHECK(!settings_apply() && strcmp(storage_active()->id, "sd2") == 0);
	// existing-file policies
	f = fopen("sd2:/gba/dumps/ROMs/x.gba","w"); fclose(f);
	strcpy(p, "sd2:/gba/dumps/ROMs/x.gba");
	CHECK(!paths_resolve_existing(p, sizeof p, PATHS_EXISTING_SKIP));
	CHECK(paths_resolve_existing(p, sizeof p, PATHS_EXISTING_OVERWRITE) && strcmp(p, "sd2:/gba/dumps/ROMs/x.gba") == 0);
	CHECK(paths_resolve_existing(p, sizeof p, PATHS_EXISTING_KEEP_BOTH) && strcmp(p, "sd2:/gba/dumps/ROMs/x (1).gba") == 0);
	f = fopen(p,"w"); fclose(f); strcpy(p, "sd2:/gba/dumps/ROMs/x.gba");
	CHECK(paths_resolve_existing(p, sizeof p, PATHS_EXISTING_KEEP_BOTH) && strcmp(p, "sd2:/gba/dumps/ROMs/x (2).gba") == 0);
	// cart naming with sanitising
	gba_cart_info c; memset(&c, 0, sizeof c);
	memcpy(c.header+0xA0, "POKEMON:EMER", 12); memcpy(c.header+0xAC, "BPEE", 4); memcpy(c.header+0xB0, "01", 2);
	paths_cart_file(p, sizeof p, &c, PATHS_KIND_SAVE);
	CHECK(strcmp(p, "sd2:/gba/dumps/Saves/POKEMON_EMER [BPEE01].sav") == 0);
	paths_set_split_folders(false); paths_bios_file(p, sizeof p);
	CHECK(strcmp(p, "sd2:/gba/dumps/gba_bios.bin") == 0);
	// device root as dump dir
	paths_set_dump_dir("sd2:/"); paths_cart_file(p, sizeof p, &c, PATHS_KIND_ROM);
	CHECK(strcmp(p, "sd2:/POKEMON_EMER [BPEE01].gba") == 0);
	// save backup names with a timestamp
	paths_set_dump_dir("sd2:/ts"); paths_set_split_folders(true); CHECK(paths_create_dirs());
	struct tm when = {0};
	when.tm_year = 2026-1900; when.tm_mon = 8; when.tm_mday = 29; when.tm_hour = 18; when.tm_min = 5; when.tm_sec = 7;
	paths_save_backup_file(p, sizeof p, &c, &when);
	CHECK(strcmp(p, "sd2:/ts/Saves/POKEMON_EMER [BPEE01] 2026-09-29 18-05-07.sav") == 0);
	paths_save_backup_file(p, sizeof p, &c, NULL);
	CHECK(strcmp(p, "sd2:/ts/Saves/POKEMON_EMER [BPEE01].sav") == 0);
	// latest backup: none yet, then newest by modification time whatever the name style
	CHECK(!paths_find_latest_save(p, sizeof p, &c));
	const char *backups[] = {
		"sd2:/ts/Saves/POKEMON_EMER [BPEE01].sav",						// plain, from older versions
		"sd2:/ts/Saves/POKEMON_EMER [BPEE01] 2026-09-28 10-00-00.sav",
		"sd2:/ts/Saves/POKEMON_EMER [BPEE01] (1).sav",					// keep both
		"sd2:/ts/Saves/POKEMON_EMER [BPEE01] 2026-09-29 18-05-07.sav",
		"sd2:/ts/Saves/POKEMON_EMER [BPEE02] 2030-01-01 00-00-00.sav",	// other game (maker), newer
		"sd2:/ts/Saves/POKEMON_EMERALD.sav",							// not ours
		"sd2:/ts/Saves/POKEMON_EMER [BPEE01].txt",						// not a save
	};
	time_t base = 1790000000;
	int mtimes[] = { 100, 200, 300, 400, 900, 950, 999 };
	for(int k = 0; k < 7; k++) {
		f = fopen(backups[k], "w"); fclose(f);
		struct utimbuf t = { base + mtimes[k], base + mtimes[k] }; utime(backups[k], &t);
	}
	CHECK(paths_find_latest_save(p, sizeof p, &c) && strcmp(p, backups[3]) == 0);
	// an older timestamped name that was modified later (e.g. copied back) wins by mtime
	struct utimbuf t2 = { base + 500, base + 500 }; utime(backups[1], &t2);
	CHECK(paths_find_latest_save(p, sizeof p, &c) && strcmp(p, backups[1]) == 0);
	// same mtime: the later name wins, so timestamped names order by date
	struct utimbuf t3 = { base + 500, base + 500 }; utime(backups[3], &t3);
	CHECK(paths_find_latest_save(p, sizeof p, &c) && strcmp(p, backups[3]) == 0);
	// only the old plain backup: still found for restore
	for(int k = 1; k < 4; k++) remove(backups[k]);
	CHECK(paths_find_latest_save(p, sizeof p, &c) && strcmp(p, backups[0]) == 0);
	// save_names setting round trip, default is timestamp
	settings_defaults(s);
	CHECK(s->timestamp_saves);
	s->timestamp_saves = false;
	CHECK(settings_save());
	settings_defaults(s);
	CHECK(settings_load(NULL) && !s->timestamp_saves);
	slurp(settings_file(), buf, sizeof buf);
	CHECK(strstr(buf, "save_names=plain") != NULL);

	printf(fails ? "\n%d FAILED\n" : "\nALL PASSED\n", fails);
	return fails != 0;
}
