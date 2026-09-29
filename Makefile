#---------------------------------------------------------------------------------
# Top level Makefile, builds the GBA multiboot payload first and then embeds it
# into the GameCube and Wii dols.
#
#   make          build everything (gc + wii)
#   make gc       build only the GameCube dol
#   make wii      build only the Wii dol
#   make gba      build only the GBA multiboot payload
#   make test     run the host tests (no devkitPro needed)
#   make screens  render the GBA payload screens to tests/build/screens
#   make dist     build both and package a release zip in dist/
#   make clean    remove all build output
#---------------------------------------------------------------------------------
export DEVKITPRO	?=	/opt/devkitpro
export DEVKITARM	?=	$(DEVKITPRO)/devkitARM
export DEVKITPPC	?=	$(DEVKITPRO)/devkitPPC

GBA_PAYLOAD	:=	data/gba_mb.gba

# e.g. v1.9, read from common/version.h like Makefile.gc/.wii do
VERSION		:=	$(shell sed -n 's/^\#define VERSION_MAJOR //p' common/version.h).$(shell sed -n 's/^\#define VERSION_MINOR //p' common/version.h)
DIST_NAME	:=	gba-link-cable-dumper-v$(VERSION)
DIST_DIR	:=	dist/$(DIST_NAME)

.PHONY: all gba gc wii test screens dist clean

all: gc wii

gba: $(GBA_PAYLOAD)

$(GBA_PAYLOAD): FORCE
	@$(MAKE) --no-print-directory -C gba
	@mkdir -p data
	@cmp -s gba/gba_mb.gba $@ || cp gba/gba_mb.gba $@

gc: $(GBA_PAYLOAD)
	@$(MAKE) --no-print-directory -f Makefile.gc

wii: $(GBA_PAYLOAD)
	@$(MAKE) --no-print-directory -f Makefile.wii

test:
	@$(MAKE) --no-print-directory -C tests

screens:
	@$(MAKE) --no-print-directory -C tests screens

# GameCube/  the dol for Swiss or any other dol loader
# Wii/       apps/gbadumper/ ready to copy to the root of an SD card for the Homebrew Channel
dist: gc wii
	@rm -rf $(DIST_DIR) dist/$(DIST_NAME).zip
	@mkdir -p $(DIST_DIR)/GameCube $(DIST_DIR)/Wii/apps/gbadumper
	@cp linkcabledump_gc_v$(VERSION).dol $(DIST_DIR)/GameCube/
	@cp linkcabledump_wii_v$(VERSION).dol $(DIST_DIR)/Wii/apps/gbadumper/boot.dol
	@sed -e 's/@VERSION@/$(VERSION)/' -e "s/@DATE@/$$(date +%Y%m%d%H%M%S)/" dist_files/meta.xml.in > $(DIST_DIR)/Wii/apps/gbadumper/meta.xml
	@cp README.md LICENSE $(DIST_DIR)/
	@cd dist && zip -qrX $(DIST_NAME).zip $(DIST_NAME)
	@echo "dist/$(DIST_NAME).zip"

clean:
	@rm -rf dist
	@$(MAKE) --no-print-directory -C tests clean
	@$(MAKE) --no-print-directory -C gba clean
	@$(MAKE) --no-print-directory -f Makefile.gc clean
	@$(MAKE) --no-print-directory -f Makefile.wii clean
	@rm -rf data

FORCE:
