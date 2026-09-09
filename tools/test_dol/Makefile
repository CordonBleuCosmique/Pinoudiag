#---------------------------------------------------------------------------------
# PinouDiag Wii V1 - test.dol (POC4)
# Binaire de test minimal et independant, voir docs/build.md.
#---------------------------------------------------------------------------------

ifeq ($(strip $(DEVKITPPC)),)
$(error "Please set DEVKITPPC in your environment. export DEVKITPPC=<path to>devkitPPC")
endif

include $(DEVKITPPC)/wii_rules

TARGET		:=	test
BUILD		:=	build
ROOT		:=	../..
SOURCES		:=	source \
				$(ROOT)/src/core/addon \
				$(ROOT)/src/core
INCLUDES	:=	source $(ROOT)/src

LIBOGC_INC ?= $(DEVKITPRO)/libogc/include
LIBOGC_LIB ?= $(DEVKITPRO)/libogc/lib/wii

INCLUDE	:=	$(foreach dir,$(INCLUDES),-I$(dir)) -I$(BUILD) -I$(LIBOGC_INC)
LIBPATHS :=	-L$(LIBOGC_LIB)

CFLAGS		:=	-g -O2 -Wall $(MACHDEP) $(INCLUDE)
LDFLAGS		:=	-g $(MACHDEP) -Wl,-Map,$(TARGET).map

LIBS	:=	-lfat -logc -lm

CFILES		:=	$(foreach dir,$(SOURCES),$(wildcard $(dir)/*.c))
OFILES		:=	$(CFILES:%.c=$(BUILD)/%.o)

.PHONY: all clean

all: $(TARGET).dol

$(TARGET).dol: $(TARGET).elf
	@echo creating $(notdir $@)
	@elf2dol "$<" "$@"

$(TARGET).elf: $(OFILES)
	@echo linking $(notdir $@)
	@$(CC) $(LDFLAGS) $(OFILES) $(LIBPATHS) $(LIBS) -o "$@"

$(BUILD)/%.o: %.c
	@mkdir -p $(dir $@)
	@echo $(notdir $<)
	@$(CC) $(CFLAGS) -c "$<" -o "$@"

clean:
	@rm -rf "$(BUILD)" "$(TARGET).elf" "$(TARGET).dol" "$(TARGET).map"
