# GBA Link Cable Dumper

Back up your Game Boy Advance cartridges with a GameCube or Wii. Dump ROMs, back up, restore and clear saves, and dump the GBA BIOS, all over the official GameCube–GBA link cable. You don't need a flash cart or any special dumping hardware.

This is a continued fork of [FIX94's GBA Link Cable Dumper](https://github.com/FIX94/gba-link-cable-dumper). It builds with current devkitPro, adds SD2SP2 support, storage selection, settings and a folder browser, and has a restructured codebase that's ready for a GUI. See the [roadmap](docs/ROADMAP.md) for what's coming next.

---

## Features

- **Dump GBA ROMs** to `.gba` files. The size is detected automatically, from 1 MB to 32 MB.
- **Back up saves** to `.sav` files. All common save types are supported:

  | Save type | Size |
  |---|---|
  | EEPROM | 512 B / 8 KB |
  | SRAM | 32 KB |
  | Flash | 64 KB / 128 KB |

- **Restore saves** from a `.sav` file back to the cartridge (the size is checked first).
- **Clear saves** on the cartridge.
- **Dump the GBA BIOS** to `gba_bios.bin`.
- **Choose your storage device**: SD2SP2, SD Gecko (slot A/B), Wii SD slot or USB. Auto-detect picks the first one found.
- **Settings screen**: storage device, output folder, ROMs/Saves/BIOS subfolders, and what to do when a file already exists.
- **Folder browser**: pick or create the output folder on the console.
- **Portable settings file**: `settings.ini` can live in the default folder or anywhere you choose.
- **Safe writing**: the card is checked before every write and existing dumps are never overwritten unless you ask for that.
- **Game info and a progress bar on the GBA screen**: title, game code, ROM size and detected save type as soon as the cartridge is read, plus a live progress bar during every transfer.
- **Control it from the GBA**: read the cartridge, dump the ROM or BIOS, and back up, restore or clear saves with the GBA's buttons. Restoring and clearing need a second press to confirm. Everything is mirrored on the TV.
- **No setup on the GBA**: the dumper is sent to the GBA over the cable (multiboot), so the GBA only needs to be switched on.

---

## What you need

| | |
|---|---|
| **Console** | Nintendo GameCube, or a Wii with GameCube controller ports (the original RVL-001 model; not the Wii Family Edition or Wii mini) |
| **Homebrew** | A way to run `.dol` files: [Swiss](https://github.com/emukidid/swiss-gc) on GameCube, the Homebrew Channel on Wii |
| **Cable** | Official GameCube Game Boy Advance Cable (DOL-011) |
| **Handheld** | Game Boy Advance or Game Boy Advance SP |
| **Controller** | GameCube controller in **port 1** |
| **Storage** | GameCube: SD2SP2 or SD Gecko. Wii: SD card, USB storage or SD Gecko |

---

## Installation

Download the latest `.dol` for your console from the [Releases](../../releases) page, or [build it yourself](#building).

- **GameCube (Swiss):** copy `linkcabledump_gc_v<version>.dol` anywhere on your SD card (e.g. `/tools/`) and launch it from Swiss.
- **Wii (Homebrew Channel):** create `sd:/apps/gbadumper/`, copy `linkcabledump_wii_v<version>.dol` into it and rename it to `boot.dol`.

---

## Usage

1. Plug a GameCube controller into **port 1** and the GBA link cable into **port 2**.
2. Start the dumper on your console. The screen shows which device and folder dumps will be saved to.
3. Connect the link cable to the GBA and turn the GBA on **without a cartridge**. The dumper is sent to it automatically, and the GBA screen says "Ready, look at the TV".
4. Insert the cartridge into the GBA and press **A** on the controller **or on the GBA**.
5. The game's name, ID, ROM size and save size are shown on the TV, and the title, code, ROM size and save type on the GBA. Choose what to do, with either the GameCube controller or the GBA:

   | Action | GameCube controller | GBA |
   |---|---|---|
   | Dump the ROM | **A** | **A** |
   | Cancel | **B** | **B** |
   | Back up the save | **Y** | **R** |
   | Restore the save from the SD card to the cartridge | **X** | **L**, then **L** again to confirm |
   | Clear the save on the cartridge | **Z** | **SELECT**, then **SELECT** again to confirm |

   The GBA shows what it's doing and a progress bar with a percentage for ROM dumps, save transfers and BIOS dumps.

6. Other buttons:

   | Action | GameCube controller | GBA |
   |---|---|---|
   | Read the cartridge | **A** (main menu) | **A** |
   | Dump the GBA BIOS | **Y** (main menu) | **SELECT** |
   | Open settings | **X** (waiting screen or main menu) | |
   | Exit | **Start** | |

   The GBA screen always shows which of its buttons do something right now.

> **The GBA resets when you insert the cartridge?** Turn the GBA off, insert the cartridge, then hold **Start + Select** while turning it on. This skips the game's boot, and the dumper can then be sent as normal.

### Dump times

The link cable is slow, so ROM dumps take a while. The estimated time is shown before you start.

| ROM size | Approximate time |
|---|---|
| 4 MB | 6 minutes |
| 8 MB | 12 minutes |
| 16 MB | 24 minutes |
| 32 MB | 48 minutes |

Save backups, restores and BIOS dumps take a few seconds.

### Output files

By default everything goes into `/dumps` on the storage device, named after the game's header:

```
dumps/
├── POKEMON SAPP [AXPE01].gba
├── POKEMON SAPP [AXPE01].sav
└── gba_bios.bin
```

With **ROMs/Saves/BIOS subfolders** turned on, the files go into `dumps/ROMs/`, `dumps/Saves/` and `dumps/BIOS/` instead. Characters that aren't allowed in file names are replaced with `_`.

---

## Storage devices

| Device | GameCube | Wii | Auto-detect order |
|---|:---:|:---:|---|
| SD2SP2 (Serial Port 2) | ✓ | | GC: 1st |
| SD Gecko in memory card slot A | ✓ | ✓ | GC: 2nd, Wii: 3rd |
| SD Gecko in memory card slot B | ✓ | ✓ | GC: 3rd, Wii: 4th |
| Front SD slot | | ✓ | Wii: 1st |
| USB storage | | ✓ | Wii: 2nd |

Every device present at startup is mounted. **Auto** uses the first one in the order above. You can pick a specific device in the settings. If it's missing at startup, the dumper tells you and falls back to Auto.

The SD2SP2 sits in Serial Port 2 on the bottom of the GameCube and works alongside the link cable, which uses controller port 2.

---

## Settings

Press **X** on the waiting screen or the main menu. Use the **D-Pad** to move, **A** or **Left/Right** to change a value, and **B** to save and go back.

| Setting | Options |
|---|---|
| Storage device | Auto, or any device found at startup |
| Output folder | Any folder, chosen with the folder browser (you can create new folders there too) |
| ROMs/Saves/BIOS subfolders | On / Off |
| If a file already exists | **Skip** (default, never dumps twice), **Overwrite**, or **Keep both** (saves as `name (1).sav`, `name (2).sav`, …) |
| Settings file | Move `settings.ini` to any folder on any device |

### settings.ini

Settings are saved to a plain text file you can also edit on a computer:

```ini
; GBA Link Cable Dumper settings
; device: auto, sd2, sda, sdb
device=auto
; dump_dir: folder on the device, starting with /
dump_dir=/dumps
; split_folders: 1 puts files into ROMs/, Saves/ and BIOS/
split_folders=0
; existing_files: skip, overwrite or keep_both
existing_files=skip
```

At startup the dumper looks for `settings.ini` in this order and uses the first one it finds:

1. Next to the `.dol`, if the loader passes its location (the Homebrew Channel does, Swiss doesn't)
2. `/gbadumper/settings.ini` on each device, in auto-detect order

If you move the settings file from the settings screen, a small pointer file (`settings_path=<new location>`) is left behind so it's still found next time. If no settings file exists, the defaults are used, and the file is only created once you change something.

---

## How it works

```
 GameCube / Wii                                        Game Boy Advance
┌─────────────────────┐   GC–GBA link cable      ┌──────────────────────┐
│ linkcabledump.dol   │   (JOY Bus, port 2)      │ gba_mb.gba (payload) │
│                     │ ───── 1. multiboot ────► │  runs from GBA RAM   │
│ link/  storage/     │ ◄──── 2. cart info ───── │  reads the cartridge │
│ ui/    settings/    │ ───── 3. command ──────► │  (ROM, save, BIOS)   │
│                     │ ◄──── 4. data ────────── │                      │
└─────────┬───────────┘                          └──────────────────────┘
          │ writes .gba / .sav / .bin
          ▼
   SD2SP2 / SD Gecko / SD / USB
```

1. **Multiboot upload.** When a GBA without a cartridge boots, its BIOS waits for a multiboot program over the link port. The console acts as the multiboot host. It exchanges session keys with the GBA BIOS, then sends the small payload program (`gba/`) encrypted and with a checksum, just like official GameCube games do for GBA link features. The payload runs from the GBA's own RAM, so the cartridge slot is free.
2. **Reading the cartridge.** When you insert a cartridge and press A, the payload:
   - checks the cartridge header to see that a valid game is present
   - works out the ROM size. Reads past the end of a cartridge return a known pattern, so the payload looks for where that pattern starts.
   - identifies the save type by searching the ROM for the save library ID strings that Nintendo's SDK builds into every game (`EEPROM_V`, `SRAM_V`, `FLASH_V`, `FLASH512_V`, `FLASH1M_V`)
   - sends the sizes and the 192-byte ROM header to the console
3. **Commands.** The console sends a command number: dump ROM, back up, restore or clear the save, or dump the BIOS. The numbers are defined once in [`common/protocol.h`](common/protocol.h), which both programs use.
   - **Buttons on the GBA.** The console controls the cable, and even a single read of the GBA's data register tells the payload to start reading the cartridge. So a button press on the GBA can't simply be placed where the console will read it. Instead:
     1. The payload puts a request (a `"GB"` magic value plus the action) into its data register and sets a spare general-purpose bit in its link status register.
     2. Every frame the console sends a *status* query, which the GBA doesn't react to. When it sees the bit, it reads the request.
     3. The console waits for the payload to clear the bit, after resetting its data register first, then acts on the request as if its own controller had been pressed.
     4. If both sides act at once, the console's command wins and the GBA drops its request.
4. **Transfers.** Data moves 4 bytes at a time over the JOY Bus. The GBA disables interrupts during a transfer to keep the timing exact. Saves are read and written with each chip's own protocol (EEPROM, SRAM, Flash), using code from Chishm's SendSave. The GBA doesn't let programs read the BIOS directly, so it's dumped with Dark Fader's trick of reading it back through a BIOS sound function.

The payload is embedded in the `.dol` at build time, so the console side and GBA side always come from the same build and match.

**The GBA screen and timing.** The console reads each 4-byte word without checking that the GBA has loaded a new one, so the GBA has to keep up. That's about 345 µs, or roughly 5,800 GBA CPU cycles, per word. The progress bar is built to fit in that time:
- two unused font tiles are replaced with solid bar tiles
- every division is done before the transfer starts
- inside the loop the payload only compares a counter, and when a step is crossed it writes a few tile numbers straight into the screen map, with no `printf`
- the bar is updated right after a word has been loaded, while the console is reading it

---

## Building

### Requirements

[devkitPro](https://devkitpro.org/wiki/Getting_Started) with devkitARM (for the GBA payload) and devkitPPC (for the GameCube/Wii app):

```bash
sudo dkp-pacman -S gba-dev gamecube-dev wii-dev
```

Tested with devkitPPC r47.1, libogc 2.13.0, libfat-ogc 2.1.0 and devkitARM r66.

### Build

**macOS / Linux:**

```bash
./build.sh          # build everything
./build.sh gc       # GameCube only
./build.sh wii      # Wii only
./build.sh clean    # remove all build output
```

**Windows:** run `build.bat` from the devkitPro MSYS2 shell.

The output is `linkcabledump_gc_v<version>.dol` and `linkcabledump_wii_v<version>.dol` in the repository root.

The build compiles the GBA payload first, copies it into `data/`, then embeds it in both `.dol` files. `make` works as well as `./build.sh`.

### Build options

| Option | Default | Description |
|---|---|---|
| `SETTINGS_DIR` | `/gbadumper` | Folder searched for `settings.ini` on each device, e.g. `./build.sh SETTINGS_DIR=/apps/gbadumper`. Run `./build.sh clean` first. |

### Tests

```bash
make test
```

Runs host tests for the settings, INI parser, paths and storage code, and for the GBA screen's progress bar maths, using your normal compiler. You don't need devkitPro or a console. The real source files are compiled against stub libogc headers in [`tests/stub`](tests/stub).

### GBA screen previews

```bash
make screens
```

Renders every GBA payload screen (ready, cart info, ROM dump at 45%, done, no cartridge) to PNG files in `tests/build/screens/`, using the real `gba/source/screen.c` and libgba's font. You can check the layout without a GBA. Needs devkitARM for the font.

### Versioning

The version is set in one place, [`common/version.h`](common/version.h). It appears in the on-screen title (on the TV and on the GBA) and in the output file names.

---

## Project layout

```
common/version.h           version number, used on screen and in the output file names
common/protocol.h          commands and sizes shared by the GC/Wii side and the GBA payload
gba/                       GBA multiboot payload (devkitARM), embedded into the dols
  source/main.c            payload: command loop, ROM/save/BIOS transfers
  source/screen.c          payload screen: cart info, status line, progress bar
  source/libSave.c         save chip access (EEPROM, SRAM, Flash), from SendSave
source/main.c              entry point
source/app/app             screen flow, ties everything below together
source/app/settings_menu   settings screen
source/app/folder_browser  folder picker used by the settings screen
source/link/si_link        raw link cable transfers over controller port 2
source/link/multiboot      uploads the GBA payload
source/link/gba_protocol   high-level commands (cart info, ROM/save/BIOS transfers)
source/settings/settings   settings.ini search, loading, saving and moving
source/settings/ini        key=value file parser
source/storage/storage     device table, mounting and file helpers
source/storage/paths       output folder, subfolders and file naming
source/ui/ui.h             front end interface, implemented by ui_console.c
source/ui/input.h          controller abstraction, implemented by input_pad.c
tests/                     host tests with stub libogc headers
docs/ROADMAP.md            planned features
```

`link/` and `storage/` never draw to the screen or read the controller. Everything on screen goes through `ui.h` and every button read goes through `input.h`, so a graphical front end only needs to replace `ui_console.c`.

---

## Troubleshooting

| Problem | Fix |
|---|---|
| GBA resets when the cartridge is inserted | Hold **Start + Select** while turning the GBA on with the cartridge already inserted |
| "Waiting for a GBA in port 2..." never finishes | Check the cable is in **controller port 2** and firmly in the GBA's link port, and turn the GBA on without a cartridge. |
| "No usable device found" | Check the SD card is FAT32 and inserted before starting the dumper |
| "Storage device … not found, using … instead" | The device chosen in settings is missing. Insert it and restart, or change the device in settings. |
| "No (Valid) GBA Card inserted!" | Clean the cartridge contacts and reseat it. The header check failed. |
| "Game already dumped!" / "Save already backed up!" | A file with that name already exists. Change **If a file already exists** in the settings to Overwrite or Keep both. |
| "Save has the wrong size" | The `.sav` file must be exactly the cartridge's save size. Emulator saves sometimes have extra bytes at the end. |
| Wii: nothing happens | Only Wii models with GameCube controller ports can use the link cable |

---

## Roadmap

Planned work includes save history with timestamps, a graphical interface, a library for browsing and restoring your dumps, and button controls on the GBA. See [docs/ROADMAP.md](docs/ROADMAP.md) for the details and current status.

## Contributing

Issues and pull requests are welcome. Before opening a pull request:

- make sure `./build.sh` builds both platforms with **no warnings**
- make sure `make test` passes
- follow the code style and module rules in [CLAUDE.md](CLAUDE.md) (C, tabs, module-prefixed function names, no screen/input access from `link/` or `storage/`)
- mention what you've tested on real hardware

## Credits

- **[FIX94](https://github.com/FIX94)**: original GBA Link Cable Dumper
- **Chishm**: SendSave, which the save support is based on
- **Dark Fader**: GBA BIOS dumping method
- **[devkitPro](https://devkitpro.org)**: devkitPPC, devkitARM, libogc, libgba and libfat
- **Tom Vardy**: this fork (build system, restructure, storage devices, SD2SP2, settings)

## License

MIT. See [LICENSE](LICENSE).
