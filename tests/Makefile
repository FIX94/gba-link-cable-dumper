#---------------------------------------------------------------------------------
# Host tests, built with the normal Mac/Linux compiler (no devkitPro needed)
#   make -C tests            run all tests
#   make -C tests screens    render the GBA payload screens to build/screens/*.png
#                            (uses the libgba font, needs devkitARM for that)
#---------------------------------------------------------------------------------
CC		?=	cc
CFLAGS	:=	-std=gnu11 -Wall -Wextra -Wno-unused-parameter -Wno-unused-function
BUILD	:=	build

SETTINGS_SRCS	:=	test_settings.c \
				../source/settings/settings.c ../source/settings/ini.c \
				../source/storage/paths.c ../source/storage/storage.c
GBA_SCREEN_SRCS	:=	test_gba_screen.c ../gba/source/screen.c

DEVKITPRO	?=	/opt/devkitpro
DEVKITARM	?=	$(DEVKITPRO)/devkitARM
LIBGBA		?=	$(DEVKITPRO)/libgba
FONT		:=	$(BUILD)/amiga_fnt.bin

.PHONY: all screens clean

all: $(BUILD)/test_settings $(BUILD)/test_gba_screen
	@rm -rf $(BUILD)/run && mkdir -p $(BUILD)/run
	@echo "== settings, ini, paths, storage"
	@cd $(BUILD)/run && ../test_settings
	@echo "== gba screen"
	@$(BUILD)/test_gba_screen

$(BUILD)/test_settings: $(SETTINGS_SRCS) $(wildcard stub/*.h stub/*/*.h ../source/*/*.h)
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) -Istub -I../source -I../common $(SETTINGS_SRCS) -o $@

$(BUILD)/test_gba_screen: $(GBA_SCREEN_SRCS) $(wildcard gba_stub/*.h ../gba/source/*.h)
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) -Igba_stub -I../gba/source -I../common $(GBA_SCREEN_SRCS) -o $@

# the font consoleDemoInit uses, taken out of libgba
$(FONT):
	@mkdir -p $(BUILD)/font
	@cd $(BUILD)/font && $(DEVKITARM)/bin/arm-none-eabi-ar x $(LIBGBA)/lib/libgba.a amiga.fnt.o
	@$(DEVKITARM)/bin/arm-none-eabi-objcopy -O binary --only-section=.rodata.amiga_fnt $(BUILD)/font/amiga.fnt.o $@

screens: $(BUILD)/test_gba_screen $(FONT)
	@rm -rf $(BUILD)/screens && mkdir -p $(BUILD)/screens
	@$(BUILD)/test_gba_screen $(FONT) $(BUILD)/screens > /dev/null
	@python3 render_gba_screen.py $(BUILD)/screens/*.bin
	@rm $(BUILD)/screens/*.bin
	@ls $(BUILD)/screens

clean:
	@rm -rf $(BUILD)
