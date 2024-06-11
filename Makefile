.SECONDEXPANSION:

ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM)
endif

NAME    := pokemover_for_gba
ELF     := $(NAME).elf
ROM     := $(NAME).gba
SYM     := $(NAME).sym
MAJOR   := 1
MINOR   := 1

PREFIX  := $(DEVKITARM)/bin/arm-none-eabi-
CC      := $(PREFIX)gcc
OBJCOPY := $(PREFIX)objcopy
OBJDUMP := $(PREFIX)objdump
AS      := $(PREFIX)as
LD      := $(PREFIX)ld
CC1     := $(shell $(CC) --print-prog-name=cc1) -quiet
CPP     := $(PREFIX)cpp
AR      := $(PREFIX)ar

PREPROC := tools/preproc/preproc
SCANINC := tools/scaninc/scaninc
RAMSCRGEN := tools/ramscrgen/ramscrgen
GBAFIX  := tools/gbafix/gbafix
GFX     := tools/gbagfx/gbagfx
PERL    := perl
BLZ     := tools/blz/blz
BLZFIX  := tools/blz/fix.sh

OBJ_DIR  := build
SUB_REPO := pokeemerald
INC_DIRS := include gflib $(SUB_REPO) $(SUB_REPO)/include $(SUB_REPO)/gflib $(SUB_REPO)/src
SUBDIRS  := src data gflib
GFLIB    := $(OBJ_DIR)/gflib.a

ASM_SRCS := $(wildcard src/*.s)
ASM_OBJS := $(ASM_SRCS:%.s=$(OBJ_DIR)/%.o)
C_SRCS := $(wildcard src/*.c)
C_OBJS := $(C_SRCS:%.c=$(OBJ_DIR)/%.o)
SUB_REPO_SRCS_0 := bg.c blit.c gpu_regs.c io_reg.c malloc.c window.c 
SUB_REPO_SRCS_1 := agb_flash_le.c agb_flash_mx.c dynamic_placeholder_text_util.c international_string_util.c load_save.c menu_helpers.c random.c script_menu.c task.c util.c
GFLIB_C_SRCS := $(wildcard gflib/*.c) $(foreach file,$(SUB_REPO_SRCS_0), $(SUB_REPO)/gflib/$(file)) $(foreach file,$(SUB_REPO_SRCS_1), $(SUB_REPO)/src/$(file))
GFLIB_C_OBJS := $(GFLIB_C_SRCS:%.c=$(OBJ_DIR)/%.o)
DATA_ASM_SRCS := $(wildcard data/*.s)
DATA_ASM_OBJS := $(DATA_ASM_SRCS:%.s=$(OBJ_DIR)/%.o)


ALL_OBJS := $(ASM_OBJS) $(C_OBJS) $(DATA_ASM_OBJS)
OBJS_REL := $(patsubst $(OBJ_DIR)/%,%,$(ALL_OBJS))

TOOLDIRS := tools/gbafix tools/gbagfx tools/preproc tools/scaninc tools/blz

ASFLAGS	:= -mcpu=arm7tdmi $(foreach dir,$(INC_DIRS),-I $(dir))
CFLAGS	:= -g -Os -mthumb -mthumb-interwork -mabi=apcs-gnu -mtune=arm7tdmi -march=armv4t -fno-toplevel-reorder -Wno-pointer-to-int-cast -DMODERN=1 -Wno-multichar
CPPFLAGS:= $(foreach dir,$(INC_DIRS),-iquote $(dir))  -Wno-trigraphs -DMODERN=1

LIBPATH := -L "$(dir $(shell $(CC) -mthumb -print-file-name=libgcc.a))" -L "$(dir $(shell $(CC) -mthumb -print-file-name=libnosys.a))" -L "$(dir $(shell $(CC) -mthumb -print-file-name=libc.a))"
LIBS    := gflib.a $(LIBPATH) -lc -lgcc

$(GFLIB_C_OBJS) : CFLAGS += -ffunction-sections -fdata-sections

$(OBJ_DIR)/src/poke_mover_screen.o: CPPFLAGS += -DVER_MAJOR=$(MAJOR) -DVER_MINOR=$(MINOR)
$(OBJ_DIR)/src/math_fast.o: CFLAGS := -mthumb-interwork -O3 -mtune=arm7tdmi -march=armv4t

$(shell mkdir -p $(SUBDIRS:%=$(OBJ_DIR)/%) graphics/interface graphics/pokedex graphics/spinda_spots graphics/text_window graphics/pokemon_storage/wallpapers)

.PHONY: all clean tools cleantools syms

ifeq ($(NODEP),1)
$(OBJ_DIR)/asm/%.o: asm_dep :=
$(OBJ_DIR)/src/%.o: c_dep :=
$(OBJ_DIR)/data/%.o: data_dep :=
else
$(OBJ_DIR)/src/%.o: asm_dep = $(shell $(SCANINC) $(foreach dir,$(INC_DIRS),-I $(dir)) $*.s)
$(OBJ_DIR)/src/%.o: c_dep = $(shell $(SCANINC) $(foreach dir,$(INC_DIRS),-I $(dir)) $*.c)
$(OBJ_DIR)/gflib/%.o: c_dep = $(shell $(SCANINC) $(foreach dir,$(INC_DIRS),-I $(dir)) $*.c)
$(OBJ_DIR)/data/%.o: data_dep = $(shell $(SCANINC) $(foreach dir,$(INC_DIRS),-I $(dir)) $*.s)
$(OBJ_DIR)/$(SUB_REPO)/src/%.o: c_dep = $(shell $(SCANINC) $(foreach dir,$(INC_DIRS),-I $(dir)) $*.c)
endif

infoshell = $(foreach line, $(shell $1 | sed "s/ /__SPACE__/g"), $(info $(subst __SPACE__, ,$(line))))

$(call infoshell, $(MAKE) -f make_tools.mk)

all : $(ROM)
	@:

syms: $(SYM)

include graphics_file_rules.mk

%.1bpp: %.png  ; $(GFX) $< $@
%.4bpp: %.png  ; $(GFX) $< $@
%.8bpp: %.png  ; $(GFX) $< $@
%.gbapal: %.pal ; $(GFX) $< $@
%.gbapal: %.png ; $(GFX) $< $@
%.lz: % ; $(GFX) $< $@
%.rl: % ; $(GFX) $< $@
graphics/%.bin: $(SUB_REPO)/graphics/%.bin
	mkdir -p $(dir $@)
	cp -n $< $@
graphics/%.4bpp: $(SUB_REPO)/graphics/%.png
	mkdir -p $(dir $@)
	$(GFX) $< $@
graphics/%.gbapal: $(SUB_REPO)/graphics/%.pal
	mkdir -p $(dir $@)
	$(GFX) $< $@
graphics/%.gbapal: $(SUB_REPO)/graphics/%.png
	mkdir -p $(dir $@)
	$(GFX) $< $@


$(ASM_OBJS): $(OBJ_DIR)/%.o: %.s $$(asm_dep)
	$(AS) $(ASFLAGS) -o $@ $<

$(C_OBJS): $(OBJ_DIR)/%.o: %.c $$(c_dep)
	$(CPP) $(CPPFLAGS) -o $(OBJ_DIR)/$*.i $<
	$(PREPROC) $(OBJ_DIR)/$*.i charmap.txt | $(CC1) $(CFLAGS) -o $(OBJ_DIR)/$*.s
	$(AS) $(ASFLAGS) -o $@ $(OBJ_DIR)/$*.s

$(GFLIB_C_OBJS): $(OBJ_DIR)/%.o: %.c $$(c_dep)
	mkdir -p $(dir $(OBJ_DIR)/$*.i)
	$(CPP) $(CPPFLAGS) -o $(OBJ_DIR)/$*.i $<
	$(PREPROC) $(OBJ_DIR)/$*.i charmap.txt | $(CC1) $(CFLAGS) -o $(OBJ_DIR)/$*.s
	$(AS) $(ASFLAGS) -o $@ $(OBJ_DIR)/$*.s

$(DATA_ASM_OBJS): $(OBJ_DIR)/%.o: %.s $$(data_dep)
	$(PREPROC) $< charmap.txt | $(CPP) $(CPPFLAGS) | $(AS) $(ASFLAGS) -o $@

$(OBJ_DIR)/ld_script.ld: ld_script.txt
	cd $(OBJ_DIR) && sed "s#tools/#../tools/#g" ../$< > ld_script.ld

$(ELF): $(OBJ_DIR)/ld_script.ld $(GFLIB) $(ALL_OBJS)
	cd $(OBJ_DIR) && $(LD) -Map ../$(NAME).map -T ../$< $(OBJS_REL) -o ../$@ $(LIBS)
	$(GBAFIX) $@ -t"POKEMOVER" -cXXXX -m01 -r0 --silent

$(GFLIB): $(GFLIB_C_OBJS)
	$(AR) rcs $@ $(OBJ_DIR)/gflib/*.o $(OBJ_DIR)/$(SUB_REPO)/gflib/*.o $(OBJ_DIR)/$(SUB_REPO)/src/*.o

$(ROM): $(ELF)
	$(OBJCOPY) -O binary $< $@
	$(GBAFIX) $@ --silent
	@echo "compress with blz"
	@$(BLZ) -en9 $@ 1>/dev/null
	@bash $(BLZFIX) $@ $(shell $(OBJDUMP) -t $< | grep -w 'Bottom' | awk '{print $$1}')

$(SYM): $(ELF)
	$(OBJDUMP) -t $< | sort -u | grep -E "^0[2389]" | $(PERL) -p -e 's/^(\w{8}) (\w).{6} \S+\t(\w{8}) (\S+)$$/\1 \2 \3 \4/g' > $@

clean:
	@$(foreach tooldir,$(TOOLDIRS),$(MAKE) clean -C $(tooldir);)
	rm -rf build
	find . \( -iname '*.1bpp' -o -iname '*.4bpp' -o -iname '*.8bpp' -o -iname '*.gbapal' -o -iname '*.lz' -o -iname '*.rl' -o -iname '*.latfont' -o -iname '*.hwjpnfont' -o -iname '*.fwjpnfont' \) -exec rm {} +
	rmdir graphics/interface graphics/pokedex graphics/spinda_spots graphics/text_window
	rm -rf graphics/pokemon_storage/wallpapers
	rm graphics/pokemon_storage/close_box_button.bin graphics/pokemon_storage/party_slot_empty.bin graphics/pokemon_storage/party_slot_filled.bin graphics/pokemon_storage/pkmn_data.bin 