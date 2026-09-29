# GBA Link Cable Dumper
A GC and Wii Homebrew App to get GBA BIOS, ROMs and saves via the GC GBA Link Cable.  
Save Support based on SendSave by Chishm.  
GBA BIOS Dumper by Dark Fader.  

See the [roadmap](docs/ROADMAP.md) for planned features (GUI, SD2SP2 support, settings, a save/ROM library and GBA-side improvements).

# Usage
Grab the release from the "releases" tab above and start up the correct dol file on your GC/Wii.  
Make sure to plug in a GC Controller into Port 1 of your console and the GBA Link Cable into Port 2.  
Now Boot your GBA without a cart inserted, it should automatically boot into the dumper when connected. From there you can just follow the instructions on screen.  
If your GBA resets when you get to the step of inserting a cart, try to boot your GBA with the cart already inserted and holding down start+select on the GBA bootup, this aborts the game launch and should allow the dumper to boot up from there.  
The bin, gba and sav files dumped will be placed in a folder called "dumps" on your storage device by default, this can be changed in the settings. Please note that dumping GBA ROMs can take a long time (32mb takes about 48 minutes) because of the cable protocol limitations, an estimation will be displayed on screen before you dump it as a reference.

## Storage devices
| Device | GameCube | Wii |
|---|---|---|
| SD2SP2 (Serial Port 2) | ✓ | |
| SD Gecko in memory card slot A or B | ✓ | ✓ |
| Front SD slot | | ✓ |
| USB storage | | ✓ |

All devices that are present are mounted at startup. By default the first one found is used (SD2SP2, then SD Gecko A, then B on GameCube; SD, then USB, then SD Gecko on Wii), a specific device can be picked in the settings.

## Settings
Press X on the "Waiting for a GBA" screen or the main menu to open the settings. Use the D-Pad to move, A or Left/Right to change a value and B to save and go back.
- **Storage device**: Auto or a specific device
- **Output folder**: pick or create a folder with the folder browser
- **ROMs/Saves/BIOS subfolders**: put each kind of file into its own subfolder of the output folder
- **If a file already exists**: Skip (don't dump it again), Overwrite, or Keep both (adds " (1)", " (2)", ... to the new file)
- **Settings file**: move `settings.ini` to any folder on any device

Settings are stored in `settings.ini`. At startup it is looked for in this order, the first one found is used:
1. Next to the dol, if the loader passes its location (e.g. `sd:/apps/gbadumper/settings.ini` from the Homebrew Channel)
2. `/gbadumper/settings.ini` on each device, in the order above

When the settings file is moved somewhere else, a small file containing `settings_path=<new location>` is left in its old place so it's still found on the next start. The default `/gbadumper` folder can be changed at build time with `./build.sh SETTINGS_DIR=/somewhere` (run `./build.sh clean` first).

# Building
You need [devkitPro](https://devkitpro.org/wiki/Getting_Started) with devkitARM (for the GBA payload) and devkitPPC (for the GC/Wii dols):  
`sudo dkp-pacman -S gba-dev gamecube-dev wii-dev`  

On macOS/Linux run `./build.sh` (or `make`), on Windows run `build.bat`.  
`./build.sh gc` or `./build.sh wii` builds a single platform, `./build.sh clean` removes all build output.  
The output is `linkcabledump_gc_v<version>.dol` and `linkcabledump_wii_v<version>.dol` in the repository root, the version comes from `common/version.h`.  
`make test` runs host tests for the settings, paths and storage code with your normal compiler (no devkitPro or console needed).

# Source layout
```
common/version.h           version number, used on screen and in the output file names
common/protocol.h          commands and sizes shared by the GC/Wii side and the GBA payload
gba/                       GBA multiboot payload (devkitARM), embedded into the dols
source/main.c              entry point
source/app/app             screen flow, ties everything below together
source/app/settings_menu   settings screen
source/app/folder_browser  folder picker used by the settings screen
source/link/si_link        raw GBA link cable transfers over controller port 2
source/link/multiboot      uploads the GBA payload
source/link/gba_protocol   high level commands (cart info, ROM/save/BIOS transfers)
source/settings/settings   settings.ini search, loading, saving and moving
source/settings/ini        key=value file parser
source/storage/storage     device table, mounting and file helpers
source/storage/paths       output folder, subfolders and file naming
source/ui/ui.h             front end interface, implemented by ui_console.c
source/ui/input.h          controller abstraction, implemented by input_pad.c
tests/                     host tests, built against the stub libogc headers in tests/stub
```
`link/` and `storage/` never draw to the screen or read the controller, so a graphical front end only needs to replace `ui_console.c`.
