#---------------------------------------------------------------------------------
# PinouDiag Wii V1 - Makefile
# devkitPPC / libogc. Voir docs/build.md pour le statut de verification.
#---------------------------------------------------------------------------------

ifeq ($(strip $(DEVKITPPC)),)
$(error "Please set DEVKITPPC in your environment. export DEVKITPPC=<path to>devkitPPC")
endif

include $(DEVKITPPC)/wii_rules

#---------------------------------------------------------------------------------
TARGET		:=	boot
BUILD		:=	build
SOURCES		:=	src/main \
				src/core \
				src/core/bootstrap \
				src/core/storage \
				src/core/detection \
				src/core/identity \
				src/core/addon \
				src/core/workflow \
				src/core/result \
				src/core/report \
				src/core/session \
				src/core/shutdown \
				src/ui \
				src/platform/wii
DATA		:=	data
INCLUDES	:=	src

#---------------------------------------------------------------------------------
# [UNVERIFIED] LIBOGC_INC/LIBOGC_LIB sont normalement deja definis par
# $(DEVKITPPC)/wii_rules dans une installation devkitPro standard (motif
# des Makefiles gabarits devkitPro) ; le fallback ci-dessous vise le
# layout standard $(DEVKITPRO)/libogc si ce n'est pas le cas. WiiMedic
# (voir docs/wiimedic.md) code ces chemins en dur pour Windows
# (C:/devkitPro/libogc/...) - non repris ici pour rester portable Linux/Mac.
LIBOGC_INC ?= $(DEVKITPRO)/libogc/include
LIBOGC_LIB ?= $(DEVKITPRO)/libogc/lib/wii

INCLUDE	:=	$(foreach dir,$(INCLUDES),-I$(dir)) \
			-I$(BUILD) \
			-I$(LIBOGC_INC)

LIBPATHS :=	-L$(LIBOGC_LIB)

#---------------------------------------------------------------------------------
CFLAGS		:=	-g -O2 -Wall $(MACHDEP) $(INCLUDE)
CXXFLAGS	:=	$(CFLAGS)
LDFLAGS		:=	-g $(MACHDEP) -Wl,-Map,$(TARGET).map

LIBS	:=	-lfat -logc -lwiiuse -lbte -lm

#---------------------------------------------------------------------------------
CFILES		:=	$(foreach dir,$(SOURCES),$(wildcard $(dir)/*.c))
CPPFILES	:=	$(foreach dir,$(SOURCES),$(wildcard $(dir)/*.cpp))
sFILES		:=	$(foreach dir,$(SOURCES),$(wildcard $(dir)/*.s))
SFILES		:=	$(foreach dir,$(SOURCES),$(wildcard $(dir)/*.S))

OFILES		:=	$(CFILES:%.c=$(BUILD)/%.o) \
				$(CPPFILES:%.cpp=$(BUILD)/%.o) \
				$(sFILES:%.s=$(BUILD)/%.o) \
				$(SFILES:%.S=$(BUILD)/%.o)

VERSION		:=	1.0.0
DIST_DIR	:=	$(CURDIR)/dist

.PHONY: all clean dist sd-image

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

# Prepare une image de carte SD prete a l'emploi (structure section 5 du
# brief) : boot.dol a la racine (point d'entree LetterBomb exact, voir
# docs/letterbomb.md et docs/return_to_loader.md), PinouDiag/{workflows,
# addons,config} en sous-dossier. Les addons doivent avoir ete construits
# au prealable (voir docs/build.md, etape 3 : third_party/WiiMedic-PinouDiag).
sd-image: all
	@rm -rf "$(DIST_DIR)/sd"
	@mkdir -p "$(DIST_DIR)/sd/PinouDiag"
	@cp -v "$(TARGET).dol" "$(DIST_DIR)/sd/boot.dol"
	@cp -rv workflows "$(DIST_DIR)/sd/PinouDiag/workflows"
	@cp -rv addons "$(DIST_DIR)/sd/PinouDiag/addons"
	@if [ -d config ] && [ -n "$$(ls -A config 2>/dev/null)" ]; then cp -rv config "$(DIST_DIR)/sd/PinouDiag/config"; fi
	@echo "Image SD prete dans $(DIST_DIR)/sd (a copier a la racine de la carte)"
	@if [ ! -f "$(DIST_DIR)/sd/PinouDiag/addons/WiiMedic/boot.dol" ]; then \
		echo "ATTENTION: addons/WiiMedic/boot.dol absent - construire d'abord third_party/WiiMedic-PinouDiag (make install)"; \
	fi

dist: sd-image
	@cd "$(DIST_DIR)" && zip -r "$(CURDIR)/PinouDiag_v$(VERSION).zip" sd
	@echo Done. Release: PinouDiag_v$(VERSION).zip
