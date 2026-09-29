# Roadmap

Planned features for GBA Link Cable Dumper, roughly in the order they should be built.
Each phase builds on the one before it, and each should land on its own branch and be merged into `master`.

Status key: ✅ done · 🚧 in progress · ⬜ not started

---

## Phase 0: Foundations ✅

- ✅ macOS/Linux build (`build.sh`, top-level `Makefile`) alongside `build.bat`
- ✅ Builds with current devkitPro (devkitPPC r47, libogc 2.13, devkitARM r66)
- ✅ `main.c` split into `app/`, `link/`, `storage/` and `ui/` modules (see the README's source layout)
- ✅ Commands and sizes shared with the GBA payload via `common/protocol.h`
- ✅ Output folder handled in one place (`storage/paths.c`) instead of hard-coded `/dumps/` strings

---

## Phase 1: Settings and storage 🚧

Everything the GUI and file manager need underneath them.

### Settings
- ✅ `source/settings/settings.c/.h`: a settings struct with load/save and defaults, `settings/ini.c` parser
- ✅ Stored as a simple `key=value` INI file, searched for next to the dol and then at `/gbadumper/settings.ini` on each device
- ✅ Settings file location is configurable: moved from the settings screen (leaves a `settings_path=` pointer behind), default folder set at build time with `SETTINGS_DIR`
- Settings:
  - ✅ storage device (see below)
  - ✅ output folder
  - ✅ separate subfolders for ROMs, saves and BIOS (`ROMs/`, `Saves/`, `BIOS/`)
  - ✅ what to do when a file already exists: **skip** (the original behaviour), **overwrite** or **keep both** (adds `(1)`, `(2)` …)
  - ⬜ keep save history: each backup is timestamped instead of skipped, so older saves are never lost
- ✅ Settings screen in the console UI (X on the waiting screen or main menu), drawn through `ui_draw_menu` so the GUI can reuse it
- ✅ Host tests for settings, INI parsing and paths (`make test`)

### Storage devices
- ✅ Replace `fatInitDefault()` with explicit mounting per device in `storage/storage.c`
- ✅ A device list, with the ones present on this console detected at startup:

  | Device | Platform | libogc interface | Mount name |
  |---|---|---|---|
  | SD Gecko slot A | GC / Wii | `__io_gcsda` | `sda:` |
  | SD Gecko slot B | GC / Wii | `__io_gcsdb` | `sdb:` |
  | **SD2SP2 (Serial Port 2)** | GC | `__io_gcsd2` | `sd2:` |
  | Front SD slot | Wii | `__io_wiisd` | `sd:` |
  | USB storage | Wii | `__io_usbstorage` | `usb:` |

- ✅ "Auto" picks the first device present (SD2SP2 → SD Gecko A → B on GC; SD → USB → SD Gecko A → B on Wii)
- ✅ Falls back to Auto with a message if the chosen device is missing at startup
- ✅ Show which device is in use on the waiting and main screens
- ✅ Check the active card is still inserted before every write, remount it or show an error instead of failing mid-write
- ⬜ Rescan for devices inserted after startup (currently only devices present at startup can be chosen)
- 🚧 **Test on hardware**: ✅ SD2SP2 on GameCube via Swiss (settings, subfolders, save backup); ⬜ SD Gecko A/B, Wii SD, USB
- Note: Swiss does not pass the dol's location (argv), so settings next to the dol only apply with loaders that do (e.g. the Homebrew Channel)

> The SD2SP2 sits in Serial Port 2 on the bottom of the GameCube (EXI bus). It does not clash with the link cable, which uses controller port 2 (SI bus).

### Folder selection
- ✅ Folder browser: list directories, go into or up, pick the current folder, switch device (for the settings file location)
- ✅ Create a new folder from the browser (generated "New Folder" names in the console UI)
- ⬜ Rename folders / type a name (needs an on-screen keyboard, better done with the GUI)
- ✅ The chosen folder is saved in settings and created on startup if it's missing

---

## Phase 2: GUI ⬜

### Library choice
**Recommended: SDL2** (`gamecube-sdl2` / `wii-sdl2`, plus `sdl2_ttf` and `sdl2_image`, all available through `dkp-pacman`).
- One codebase for GameCube and Wii
- TrueType fonts and PNG images out of the box
- The same GUI code can be **built as a normal Mac/Linux app** with a simulated link backend. That makes layout and flow testable without a console or GBA, which speeds up iteration a lot.

Alternatives: GRRLIB (popular for homebrew, but not in devkitPro's package repo) or raw GX (most control, most work).

### Structure
- ⬜ `source/ui/ui_gui.c` implements the existing `ui.h` interface, so `app/` doesn't change
- ⬜ Build option to pick the console UI or the GUI (`make UI=console`). Keep the console UI as a fallback.
- ⬜ `link/` fake backend (`si_link_sim.c`) for desktop builds that replays a cart dump from a file
- ⬜ Rendering during transfers: transfers currently block. Either render a frame from the progress callback (simple, and the link runs at only ~11 KB/s) or move transfers to an LWP thread (cleaner, needed for a cancel button). Decide before building the progress screen.

### Screens
- ⬜ **Home**: connection status, active device and folder, menu (Dump / Library / Settings)
- ⬜ **Connect**: "plug in GBA" instructions with an animation, and the tips from the README (the start+select boot trick)
- ⬜ **Cart**: title, game code, maker, ROM size, save type and size; actions for dump ROM, backup, restore and clear save; warns if a ROM or save already exists in the library
- ⬜ **Transfer**: progress bar, MB done, speed, time remaining, and "don't unplug" messaging
- ⬜ **Library**: the file manager (phase 3)
- ⬜ **Settings**: every phase 1 setting, including the folder browser
- ⬜ **Dialogs**: confirm (restore/clear/delete), error, info

### Input
- ⬜ GameCube controller (current)
- ⬜ Wii: Wii Remote, Classic Controller and GC controller
- ⬜ A consistent layout: A select, B back, Start menu

---

## Phase 3: File manager / Library ⬜

A menu for browsing everything that has been dumped and using it again.

### Browsing
- ⬜ Scan the output folder and build a library grouped **by game** (matched on game code + title), with three views:
  - **Games**: each game with its ROM, all of its save backups, and status icons
  - **Saves**: every save backup, newest first, filterable by game
  - **ROMs**: every ROM dump
  - plus the **BIOS** dump, if present
- ⬜ Sort by name, date or size

### Viewing
- ⬜ **ROM details**: title, game code, maker, version, file size, and a header checksum check (the 0xBD complement byte) to spot bad dumps
- ⬜ **Save details**: size, likely save type (EEPROM 512B/8KB, SRAM 32KB, Flash 64/128KB), date, and which game it belongs to
- ⬜ Hex viewer for any file (read-only), useful for debugging bad saves
- ⬜ CRC32 of ROMs and saves, shown and saved alongside the dump

### Restoring and managing
- ⬜ **Restore save to cart** from the library. Pick any backup for the inserted game, not just the latest one.
  - check the game code matches the inserted cart, with a strong warning on mismatch
  - check the size matches the cart's save size (already done today)
  - **automatically back up the cart's current save before overwriting it**
- ⬜ Rename, delete (with confirmation) and duplicate save backups
- ⬜ Mark a save as "favourite/protected" so it can't be deleted by accident

> **ROMs can't be written back to a retail cartridge.** The ROM chip is read-only (mask ROM). ROM entries in the library can be viewed, verified and deleted but not restored. Writing to flash carts is a possible stretch goal, not planned.

---

## Phase 4: GBA payload improvements ⬜

The program uploaded to the GBA (`gba/source/main.c`) currently just says "Please look at the TV".

### Game info on the GBA screen
- ⬜ When a cart is read, show title, game code, maker, ROM size and **detected save type** (libSave already works this out in `SaveSize`)
- ⬜ Show "No cart / invalid cart" when the header check fails

### Progress bar on the GBA screen
- ⬜ Progress bar and percentage during ROM dumps, save backups, save restores and BIOS dumps
- ⬜ The payload already knows the total size, so it doesn't need anything extra from the console
- ⬜ Interrupts are disabled during transfers, so update VRAM directly every N KB (e.g. every 16 KB) inside the send loop, not from a vblank handler. Measure the effect on dump speed.
- ⬜ "Done" / "Error" screens when a transfer finishes

### Controls on the GBA
- ⬜ Let the user start actions from the GBA buttons (e.g. A: dump ROM, B: back up save, Start: cancel)
- ⬜ Needs a **protocol change**: at the moment the console always asks and the GBA answers. Add a "GBA request" message that the GBA posts while idle and the console reads when it polls.
- ⬜ The console's UI mirrors actions started on the GBA (the console still writes the files)
- ⬜ Cancel: split long transfers into acknowledged blocks so either side can stop cleanly

### Protocol notes
- All command numbers and sizes live in `common/protocol.h`, used by both sides
- The GBA payload is embedded in the dol, so the two sides are always built together and can't be mismatched. No version negotiation needed.
- Multiboot images are limited to 256 KB, so graphics and fonts on the GBA side have to stay small

---

## Phase 5: Nice to have ⬜

- ⬜ Continuous integration on Gitea Actions using the `devkitpro/devkitppc` and `devkitpro/devkitarm` Docker images, publishing the `.dol` files as artifacts
- 🚧 Host-side unit tests (built with the Mac/Linux compiler) for code that doesn't touch hardware: ✅ paths, settings, INI parser (`make test`); ⬜ library scanning, header parsing
- ⬜ Look into faster transfers (the 50 µs SI delay and 4-byte transfers are the bottleneck; 32 MB takes ~48 minutes)
- ⬜ Dump log file (what was dumped, when, CRC32)
- ⬜ Compare dumps against a No-Intro DAT file to confirm good dumps
- ⬜ Widescreen layout on Wii
