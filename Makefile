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
#   make clean    remove all build output
#---------------------------------------------------------------------------------
export DEVKITPRO	?=	/opt/devkitpro
export DEVKITARM	?=	$(DEVKITPRO)/devkitARM
export DEVKITPPC	?=	$(DEVKITPRO)/devkitPPC

GBA_PAYLOAD	:=	data/gba_mb.gba

.PHONY: all gba gc wii test screens clean

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

clean:
	@$(MAKE) --no-print-directory -C tests clean
	@$(MAKE) --no-print-directory -C gba clean
	@$(MAKE) --no-print-directory -f Makefile.gc clean
	@$(MAKE) --no-print-directory -f Makefile.wii clean
	@rm -rf data

FORCE:
