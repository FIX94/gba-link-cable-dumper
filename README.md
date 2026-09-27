# GBA Link Cable Dumper
A GC and Wii Homebrew App to get GBA BIOS, ROMs and saves via the GC GBA Link Cable.  
Save Support based on SendSave by Chishm.  
GBA BIOS Dumper by Dark Fader.  

# Usage
Grab the release from the "releases" tab above and start up the correct dol file on your GC/Wii.  
Make sure to plug in a GC Controller into Port 1 of your console and the GBA Link Cable into Port 2.  
Now Boot your GBA without a cart inserted, it should automatically boot into the dumper when connected. From there you can just follow the instructions on screen.  
If your GBA resets when you get to the step of inserting a cart, try to boot your GBA with the cart already inserted and holding down start+select on the GBA bootup, this aborts the game launch and should allow the dumper to boot up from there.  
The bin, gba and sav files dumped will be placed in a folder called "dumps" on your main device (SD Gecko on gamecube and SD/USB on Wii). Please note that dumping GBA ROMs can take a long time (32mb takes about 48 minutes) because of the cable protocol limitations, an estimation will be displayed on screen before you dump it as a reference.

# Building
You need [devkitPro](https://devkitpro.org/wiki/Getting_Started) with devkitARM (for the GBA payload) and devkitPPC (for the GC/Wii dols):  
`sudo dkp-pacman -S gba-dev gamecube-dev wii-dev`  

On macOS/Linux run `./build.sh` (or `make`), on Windows run `build.bat`.  
`./build.sh gc` or `./build.sh wii` builds a single platform, `./build.sh clean` removes all build output.  
The output is `linkcabledump_gc.dol` and `linkcabledump_wii.dol` in the repository root.

# Source layout
```
common/protocol.h        commands and sizes shared by the GC/Wii side and the GBA payload
gba/                     GBA multiboot payload (devkitARM), embedded into the dols
source/main.c            entry point
source/app/              screen flow, ties everything below together
source/link/si_link      raw GBA link cable transfers over controller port 2
source/link/multiboot    uploads the GBA payload
source/link/gba_protocol high level commands (cart info, ROM/save/BIOS transfers)
source/storage/storage   device mounting and file helpers
source/storage/paths     output folder and file naming
source/ui/ui.h           front end interface, implemented by ui_console.c
source/ui/input.h        controller abstraction, implemented by input_pad.c
```
`link/` and `storage/` never draw to the screen or read the controller, so a graphical front end only needs to replace `ui_console.c`.
