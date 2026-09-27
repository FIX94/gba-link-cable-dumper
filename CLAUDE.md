# CLAUDE.md

GameCube/Wii homebrew that dumps GBA ROMs, saves and BIOS over the GC–GBA link cable. It has two programs:
- **GC/Wii app** (`source/`, devkitPPC + libogc), built as `linkcabledump_gc.dol` and `linkcabledump_wii.dol`
- **GBA payload** (`gba/`, devkitARM + libgba), uploaded to the GBA over the cable (multiboot) and embedded in the dols

Planned work is in [docs/ROADMAP.md](docs/ROADMAP.md). Update its status markers when a feature lands.

## Build

```bash
./build.sh          # everything (GBA payload -> data/gba_mb.gba -> gc + wii dols)
./build.sh gc       # GameCube only
./build.sh wii      # Wii only
./build.sh clean
```

- The toolchain is in `/opt/devkitpro` (devkitPPC r47, libogc 2.13, devkitARM r66). `build.sh` sets `DEVKITPRO`/`DEVKITPPC`/`DEVKITARM` if they're unset.
- `data/` is generated. The top-level `Makefile` copies the GBA payload there, and bin2o turns it into `build_*/gba_mb_gba.h`.
- GC and Wii use separate build dirs (`build_gc/`, `build_wii/`). New source subdirectories must be added to `SOURCES` in **both** `Makefile.gc` and `Makefile.wii`. Object files are named by basename, so **source file names must be unique across directories**.
- A change is done when `./build.sh` passes with **no warnings** on both platforms.
- There's no hardware or emulator testing here. The GBA link cable can't be emulated, so say clearly that a change is untested on hardware. When refactoring the GBA side, compare `gba/gba_mb.gba` against the previous build to confirm it's byte-identical when no behaviour change is intended.

## Layout and rules

```
common/protocol.h   commands/sizes/version shared by both programs
source/app/         screen flow; the only place that combines link + storage + ui
source/link/        SI transfers, multiboot upload, high-level GBA commands
source/storage/     device mounting, file helpers, output paths
source/ui/          ui.h (front end interface) + ui_console.c, input.h + input_pad.c
gba/source/         GBA payload (main.c) and libSave (save chip access)
```

- `link/` and `storage/` must **never** print or read input. Report progress with callbacks (`gba_progress_cb`, `gba_chunk_cb`).
- All on-screen output goes through `ui.h`, and all button reads go through `input.h` (`INPUT_*` bits, not `PAD_*`). This is what lets a GUI replace `ui_console.c`.
- Any change to the cable protocol must update **both** `source/link/gba_protocol.c` and `gba/source/main.c`, with constants in `common/protocol.h`. The payload is embedded, so both sides always ship together.
- Build file paths with `storage/paths.c` (it sanitises FAT-invalid characters). Don't hard-code `/dumps`.
- GBA payload constraints: multiboot image ≤ 256 KB. Interrupts are disabled during transfers (`REG_IME = 0`), so don't rely on vblank IRQs there. The GBA side waits in busy loops on `REG_HS_CTRL`.
- SI timing: `SI_TRANS_DELAY` of 50 µs is the lowest value found to be reliable. Don't lower it without hardware testing.

## Code style

- C, tabs for indentation, braces on their own line, `lower_snake_case` for new functions, prefixed by module (`gba_`, `si_link_`, `storage_`, `paths_`, `ui_`, `input_`)
- Every source file starts with the FIX94 MIT license header used in the existing files
- Header guards look like `__MODULE_H__`
- Short `//` comments, only where the reason isn't obvious

## Git

- Branch for each feature, merge into `master` with `--no-ff`
- Push to the **`gitea`** remote (`ssh://git@192.168.86.101:2222/tom/gba-link-cable-dumper.git`). `origin` is FIX94's upstream GitHub repo, so never push there.
