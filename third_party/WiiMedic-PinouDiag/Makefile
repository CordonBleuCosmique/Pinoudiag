#---------------------------------------------------------------------------------
# WiiMedic (PinouDiag build) - fork de https://github.com/PowFPS1/WiiMedic
# GPLv2 - voir LICENSE, CHANGES.md et docs/licensing.md dans le depot PinouDiag.
#
# Base sur le Makefile original de WiiMedic (PowFPS1), etendu pour compiler
# les fichiers vendus depuis le core PinouDiag (pd_types, dol_loader) qui
# implementent le retour vers PinouDiag - voir CHANGES.md.
#---------------------------------------------------------------------------------

ifeq ($(strip $(DEVKITPPC)),)
$(error "Please set DEVKITPPC in your environment. export DEVKITPPC=<path to>devkitPPC")
endif

include $(DEVKITPPC)/wii_rules

#---------------------------------------------------------------------------------
TARGET		:=	boot
BUILD		:=	build
ROOT		:=	../..
SOURCES		:=	source \
				$(ROOT)/src/core/addon \
				$(ROOT)/src/core
DATA		:=	data
INCLUDES	:=	source $(ROOT)/src $(ROOT)/src/core $(ROOT)/src/core/addon

LIBOGC_INC ?= $(DEVKITPRO)/libogc/include
LIBOGC_LIB ?= $(DEVKITPRO)/libogc/lib/wii

#---------------------------------------------------------------------------------
INCLUDE	:=	$(foreach dir,$(INCLUDES),-I$(dir)) \
			-I$(BUILD) \
			-I$(LIBOGC_INC)

LIBPATHS :=	-L$(LIBOGC_LIB)

#---------------------------------------------------------------------------------
CFLAGS		:=	-g -O2 -Wall $(MACHDEP) $(INCLUDE)
CXXFLAGS	:=	$(CFLAGS)
LDFLAGS		:=	-g $(MACHDEP) -Wl,-Map,$(TARGET).map

#---------------------------------------------------------------------------------
LIBS	:=	-lwiiuse -lbte -lfat -logc -lm

#---------------------------------------------------------------------------------
# Substitution generique (pas $(SOURCES)/%.c) car SOURCES contient
# desormais plusieurs repertoires, dont deux hors de cet arbre (ROOT).
#---------------------------------------------------------------------------------
CFILES		:=	$(foreach dir,$(SOURCES),$(wildcard $(dir)/*.c))
CPPFILES	:=	$(foreach dir,$(SOURCES),$(wildcard $(dir)/*.cpp))
sFILES		:=	$(foreach dir,$(SOURCES),$(wildcard $(dir)/*.s))
SFILES		:=	$(foreach dir,$(SOURCES),$(wildcard $(dir)/*.S))

OFILES		:=	$(CFILES:%.c=$(BUILD)/%.o) \
				$(CPPFILES:%.cpp=$(BUILD)/%.o) \
				$(sFILES:%.s=$(BUILD)/%.o) \
				$(SFILES:%.S=$(BUILD)/%.o)

VERSION		:=	1.3.1-pinoudiag1
DIST_DIR	:=	$(CURDIR)/dist
# Destination par defaut : le dossier addon livre par le depot PinouDiag
# (structure decrite section 8 du brief, voir addons/WiiMedic/manifest.json).
INSTALL_DIR	?=	$(CURDIR)/../../addons/WiiMedic

.PHONY: all clean dist install

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

$(BUILD)/%.o: %.cpp
	@mkdir -p $(dir $@)
	@echo $(notdir $<)
	@$(CXX) $(CXXFLAGS) -c "$<" -o "$@"

$(BUILD)/%.o: %.s
	@mkdir -p $(dir $@)
	@echo $(notdir $<)
	@$(CC) -x assembler-with-cpp $(CFLAGS) -c "$<" -o "$@"

$(BUILD)/%.o: %.S
	@mkdir -p $(dir $@)
	@echo $(notdir $<)
	@$(CC) -x assembler-with-cpp $(CFLAGS) -c "$<" -o "$@"

clean:
	@echo clean ...
	@rm -rf "$(BUILD)" "$(TARGET).elf" "$(TARGET).dol" "$(TARGET).map"

install: all
	@echo Installing to $(INSTALL_DIR) ...
	@mkdir -p "$(INSTALL_DIR)"
	@cp -v "$(TARGET).dol" "$(INSTALL_DIR)/boot.dol"
	@echo Done.

dist: all
	@echo Creating release zip...
	@rm -rf "$(DIST_DIR)"
	@mkdir -p "$(DIST_DIR)/WiiMedic"
	@cp -v "$(TARGET).dol" "$(DIST_DIR)/WiiMedic/boot.dol"
	@cp -v "$(CURDIR)/../../addons/WiiMedic/manifest.json" "$(DIST_DIR)/WiiMedic/" 2>/dev/null || true
	@cd "$(DIST_DIR)" && zip -r "$(CURDIR)/WiiMedic-PinouDiag_v$(VERSION).zip" WiiMedic
	@rm -rf "$(DIST_DIR)"
	@echo Done. Release: WiiMedic-PinouDiag_v$(VERSION).zip
