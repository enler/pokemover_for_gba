.SECONDEXPANSION:

ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM)
endif

NAME	:= pokemover_for_gba
ELF		:= $(NAME).elf
ROM		:= $(NAME).gba
SYM		:= $(NAME).sym

PREFIX	:= $(DEVKITARM)/bin/arm-none-eabi-
CC		:= $(PREFIX)gcc
OBJCOPY	:= $(PREFIX)objcopy
OBJDUMP	:= $(PREFIX)objdump
AS		:= $(PREFIX)as
LD		:= $(PREFIX)ld
CC1		:= $(shell $(CC) --print-prog-name=cc1) -quiet
CPP		:= $(PREFIX)cpp
AR		:= $(PREFIX)ar

PREPROC	:= tools/preproc/preproc
SCANINC := tools/scaninc/scaninc
RAMSCRGEN := tools/ramscrgen/ramscrgen
GBAFIX	:= tools/gbafix/gbafix
GFX		:= tools/gbagfx/gbagfx
PERL	:= perl

OBJ_DIR := build
SUBDIRS := src data gflib
GFLIB	:= $(OBJ_DIR)/gflib.a

ASM_SRCS := $(wildcard src/*.s)
ASM_OBJS := $(ASM_SRCS:%.s=$(OBJ_DIR)/%.o)
C_SRCS := $(wildcard src/*.c)
C_OBJS := $(C_SRCS:%.c=$(OBJ_DIR)/%.o)
GFLIB_C_SRCS := $(wildcard gflib/*.c)
GFLIB_C_OBJS := $(GFLIB_C_SRCS:%.c=$(OBJ_DIR)/%.o)
DATA_ASM_SRCS := $(wildcard data/*.s)
DATA_ASM_OBJS := $(DATA_ASM_SRCS:%.s=$(OBJ_DIR)/%.o)


ALL_OBJS := $(ASM_OBJS) $(C_OBJS) $(DATA_ASM_OBJS)
OBJS_REL := $(patsubst $(OBJ_DIR)/%,%,$(ALL_OBJS))

ASFLAGS	:= -mcpu=arm7tdmi
CFLAGS	:= -Os -mthumb -mthumb-interwork -mabi=apcs-gnu -mtune=arm7tdmi -march=armv4t -fno-toplevel-reorder -Wno-pointer-to-int-cast -DMODERN=1 -Wno-multichar
CPPFLAGS:= -iquote include -iquote gflib -Wno-trigraphs -DMODERN=1

LIBPATH := -L "$(dir $(shell $(CC) -mthumb -print-file-name=libgcc.a))" -L "$(dir $(shell $(CC) -mthumb -print-file-name=libnosys.a))" -L "$(dir $(shell $(CC) -mthumb -print-file-name=libc.a))"
LIBS	:= gflib.a $(LIBPATH) -lc -lgcc

$(GFLIB_C_OBJS) : CFLAGS += -ffunction-sections -fdata-sections

$(OBJ_DIR)/src/math_fast.o: CFLAGS := -mthumb-interwork -O3 -mtune=arm7tdmi -march=armv4t

$(shell mkdir -p $(SUBDIRS:%=$(OBJ_DIR)/%))

.PHONY: all clean tools cleantools syms

ifeq ($(NODEP),1)
$(OBJ_DIR)/asm/%.o: asm_dep :=
$(OBJ_DIR)/src/%.o: c_dep :=
$(OBJ_DIR)/data/%.o: data_dep :=
else
$(OBJ_DIR)/src/%.o: asm_dep = $(shell $(SCANINC) -I include -I gflib $*.s)
$(OBJ_DIR)/src/%.o: c_dep = $(shell $(SCANINC) -I include -I gflib $*.c)
$(OBJ_DIR)/gflib/%.o: c_dep = $(shell $(SCANINC) -I include -I gflib $*.c)
$(OBJ_DIR)/data/%.o: data_dep = $(shell $(SCANINC) -I include -I gflib $*.s)
endif

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

$(ASM_OBJS): $(OBJ_DIR)/%.o: %.s $$(asm_dep)
	$(AS) $(ASFLAGS) -o $@ $<

$(C_OBJS): $(OBJ_DIR)/%.o: %.c $$(c_dep)
	$(CPP) $(CPPFLAGS) -o $(OBJ_DIR)/$*.i $<
	$(PREPROC) $(OBJ_DIR)/$*.i charmap.txt | $(CC1) $(CFLAGS) -o $(OBJ_DIR)/$*.s
	$(AS) $(ASFLAGS) -o $@ $(OBJ_DIR)/$*.s

$(GFLIB_C_OBJS): $(OBJ_DIR)/%.o: %.c $$(c_dep)
	$(CPP) $(CPPFLAGS) -o $(OBJ_DIR)/$*.i $<
	$(PREPROC) $(OBJ_DIR)/$*.i charmap.txt | $(CC1) $(CFLAGS) -o $(OBJ_DIR)/$*.s
	$(AS) $(ASFLAGS) -o $@ $(OBJ_DIR)/$*.s

$(DATA_ASM_OBJS): $(OBJ_DIR)/%.o: %.s $$(data_dep)
	$(PREPROC) $< charmap.txt | $(CPP) $(CPPFLAGS) | $(AS) $(ASFLAGS) -o $@

$(OBJ_DIR)/ld_script.ld: ld_script.txt
	cd $(OBJ_DIR) && sed "s#tools/#../tools/#g" ../$< > ld_script.ld

$(ELF): $(OBJ_DIR)/ld_script.ld $(ALL_OBJS) $(GFLIB)
	cd $(OBJ_DIR) && $(LD) -Map ../$(NAME).map -T ../$< $(OBJS_REL) -o ../$@ $(LIBS)
	$(GBAFIX) $@ -t"POKEMOVER" -cXXXX -m01 -r0 --silent

$(GFLIB): $(GFLIB_C_OBJS)
	$(AR) rcs $@ $(OBJ_DIR)/gflib/*.o

$(ROM): $(ELF)
	$(OBJCOPY) -O binary $< $@
	$(GBAFIX) $@ -p --silent

$(SYM): $(ELF)
	$(OBJDUMP) -t $< | sort -u | grep -E "^0[2389]" | $(PERL) -p -e 's/^(\w{8}) (\w).{6} \S+\t(\w{8}) (\S+)$$/\1 \2 \3 \4/g' > $@

clean:
	rm -rf build
	find . \( -iname '*.1bpp' -o -iname '*.4bpp' -o -iname '*.8bpp' -o -iname '*.gbapal' -o -iname '*.lz' -o -iname '*.rl' -o -iname '*.latfont' -o -iname '*.hwjpnfont' -o -iname '*.fwjpnfont' \) -exec rm {} +
