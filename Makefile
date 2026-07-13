ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

TOPDIR ?= $(CURDIR)
include $(DEVKITARM)/3ds_rules

TARGET       := 3dsfin
BUILD        := build
SOURCES      := source
DATA         := data
INCLUDES     := source
ROMFS        := romfs

APP_TITLE    := 3DSFin
APP_DESCRIPTION := Jellyfin client
APP_AUTHOR   := 3dsfin project

# libctru/citro2d/citro3d are already on the default search path via
# devkitARM's 3dsx.specs -- no need to add them manually. Portlibs (curl,
# mbedtls, zlib) are NOT covered by that, so we add them explicitly here.
PORTLIBS := $(DEVKITPRO)/portlibs/3ds

ARCH := -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft

CFLAGS := -g -Wall -O2 -mword-relocations \
          -fomit-frame-pointer -ffunction-sections \
          $(ARCH) $(INCLUDE) -D__3DS__

CXXFLAGS := $(CFLAGS) -fno-rtti -fno-exceptions -std=gnu++11

ASFLAGS := -g $(ARCH)
LDFLAGS := -specs=3dsx.specs -g $(ARCH) -Wl,-Map,$(notdir $*.map)

LIBS := -lcitro2d -lcitro3d -lctru -lcurl -lmbedtls -lmbedx509 -lmbedcrypto -lz -lm

LIBDIRS := $(PORTLIBS)

# ---- standard devkitPro 3ds template boilerplate below ----

ifneq ($(BUILD),$(notdir $(CURDIR)))
export OUTPUT := $(CURDIR)/$(TARGET)
export TOPDIR := $(CURDIR)

CFILES   := $(notdir $(wildcard $(SOURCES)/*.c))
CPPFILES := $(notdir $(wildcard $(SOURCES)/*.cpp))
SFILES   := $(notdir $(wildcard $(SOURCES)/*.s))

export OFILES := $(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)
export INCLUDE := -I$(CURDIR)/$(SOURCES) -I$(PORTLIBS)/include
export LIBPATHS := -L$(PORTLIBS)/lib

# Without this, the sub-make invoked from inside build/ has no idea where
# to find app_state.c etc. -- it only looks in its own (build/) directory,
# hence "No rule to make target 'app_state.o'".
export VPATH := $(foreach dir,$(SOURCES) $(DATA),$(CURDIR)/$(dir))

.PHONY: all clean

all: $(BUILD)
	@[ -d $(BUILD) ] || mkdir -p $(BUILD)
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

$(BUILD):
	@mkdir -p $@

clean:
	@echo clean ...
	@rm -fr $(BUILD) $(TARGET).3dsx $(TARGET).smdh $(TARGET).elf $(TARGET).cia

else

DEPENDS := $(OFILES:.o=.d)

$(OUTPUT).3dsx: $(OUTPUT).elf $(OUTPUT).smdh
$(OUTPUT).elf: $(OFILES)

$(OUTPUT).smdh:
	smdhtool --create "$(APP_TITLE)" "$(APP_DESCRIPTION)" "$(APP_AUTHOR)" \
		$(TOPDIR)/romfs/gfx/icon.png $(OUTPUT).smdh

-include $(DEPENDS)

endif
