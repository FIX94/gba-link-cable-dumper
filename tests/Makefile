#---------------------------------------------------------------------------------
# Host tests, built with the normal Mac/Linux compiler (no devkitPro needed)
#   make -C tests
#---------------------------------------------------------------------------------
CC		?=	cc
CFLAGS	:=	-std=gnu11 -Wall -Wextra -Wno-unused-parameter -Wno-unused-function \
			-Istub -I../source -I../common
SRCS	:=	test_settings.c \
			../source/settings/settings.c ../source/settings/ini.c \
			../source/storage/paths.c ../source/storage/storage.c
BUILD	:=	build

.PHONY: all clean

all: $(BUILD)/test_settings
	@rm -rf $(BUILD)/run && mkdir -p $(BUILD)/run
	@cd $(BUILD)/run && ../test_settings

$(BUILD)/test_settings: $(SRCS) $(wildcard stub/*.h stub/*/*.h ../source/*/*.h)
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) $(SRCS) -o $@

clean:
	@rm -rf $(BUILD)
